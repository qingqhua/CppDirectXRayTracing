//#include "Helper.hlsl"

//------------------------------------
// Helper functions
//------------------------------------
static const float3 cameraPosition = float3(0, 0, -5);
static const float4 backgroundColor = float4(1.0, 0.6, 0.2, 1.0);
static const float4 lightAmbientColor = float4(0.2, 0.2, 0.2, 1.0);
static const float3 lightPosition = float3(2.0, 2.0, -2.0);
static const float4 lightDiffuseColor = float4(0.7, 0.2, 0.2, 1.0);
static const float4 lightSpecularColor = float4(1, 1, 1, 1);
static const float4 primitiveAlbedo = float4(1.0, 0.8, 0.5, 1.0);
static const float diffuseCoef = 0.9;
static const float specularCoef = 0.7;
static const float specularPower = 50;

// Pack vertex buffer to shader
struct Vertex
{
	float3 Position;
	float3 Normal;
	float3 Tangent;
	float2 TexCoord;
};

float4 linearToSrgb(float4 c)
{
	// Based on http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
	float4 sq1 = sqrt(c);
	float4 sq2 = sqrt(sq1);
	float4 sq3 = sqrt(sq2);
	float4 srgb = 0.662002687 * sq1 + 0.684122060 * sq2 - 0.323583601 * sq3 - 0.0225411470 * c;
	return srgb;
}

// Retrieve hit world position.
float3 HitWorldPosition()
{
	return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

// Diffuse lighting calculation.
float CalculateDiffuseCoefficient(in float3 hitPosition, in float3 incidentLightRay, in float3 normal)
{
	float fNDotL = saturate(dot(-incidentLightRay, normal));
	return fNDotL;
}

// Phong lighting specular component
float4 CalculateSpecularCoefficient(in float3 hitPosition, in float3 incidentLightRay, in float3 normal, in float specularPower)
{
	float3 reflectedLightRay = normalize(reflect(incidentLightRay, normal));
	return pow(saturate(dot(reflectedLightRay, normalize(-WorldRayDirection()))), specularPower);
}


// Phong lighting model = ambient + diffuse + specular components.
float4 CalculatePhongLighting(in float4 albedo, in float3 normal, in float diffuseCoef = 1.0, in float specularCoef = 1.0, in float specularPower = 50)
{
	float3 hitPosition = HitWorldPosition();
	float3 incidentLightRay = normalize(hitPosition - lightPosition);

	// Diffuse component.
	float Kd = CalculateDiffuseCoefficient(hitPosition, incidentLightRay, normal);
	float4 diffuseColor = diffuseCoef * Kd * lightDiffuseColor * albedo;

	// Specular component.
	float4 specularColor = float4(0, 0, 0, 0);
	float4 Ks = CalculateSpecularCoefficient(hitPosition, incidentLightRay, normal, specularPower);
	specularColor = specularCoef * Ks * lightSpecularColor;

	// Ambient component.
	// Fake AO: Darken faces with normal facing downwards/away from the sky a little bit.
	float4 ambientColorMin = lightAmbientColor - 0.15;
	float4 ambientColorMax = lightAmbientColor;
	float fNDotL = saturate(dot(-incidentLightRay, normal));
	float4 ambientColor = albedo * lerp(ambientColorMin, ambientColorMax, fNDotL);

	return ambientColor + diffuseColor + specularColor;
}

// Load three 16 bit indices from a byte addressed buffer.
uint3 Load3x16BitIndices(ByteAddressBuffer Indices, uint offsetBytes)
{
	uint3 indices;

	// ByteAdressBuffer loads must be aligned at a 4 byte boundary.
	// Since we need to read three 16 bit indices: { 0, 1, 2 } 
	// aligned at a 4 byte boundary as: { 0 1 } { 2 0 } { 1 2 } { 0 1 } ...
	// we will load 8 bytes (~ 4 indices { a b | c d }) to handle two possible index triplet layouts,
	// based on first index's offsetBytes being aligned at the 4 byte boundary or not:
	//  Aligned:     { 0 1 | 2 - }
	//  Not aligned: { - 0 | 1 2 }
	const uint dwordAlignedOffset = offsetBytes & ~3;
	const uint2 four16BitIndices = Indices.Load2(dwordAlignedOffset);

	// Aligned: { 0 1 | 2 - } => retrieve first three 16bit indices
	if (dwordAlignedOffset == offsetBytes)
	{
		indices.x = four16BitIndices.x & 0xffff;
		indices.y = (four16BitIndices.x >> 16) & 0xffff;
		indices.z = four16BitIndices.y & 0xffff;
	}
	else // Not aligned: { - 0 | 1 2 } => retrieve last three 16bit indices
	{
		indices.x = (four16BitIndices.x >> 16) & 0xffff;
		indices.y = four16BitIndices.y & 0xffff;
		indices.z = (four16BitIndices.y >> 16) & 0xffff;
	}

	return indices;
}


//----------------------------
// Real shader calculation
//--------------------------------------
RaytracingAccelerationStructure gRtScene : register(t0);
RWTexture2D<float4> gOutput : register(u0);
ByteAddressBuffer Indices : register(t1);
StructuredBuffer<Vertex> Vertices : register(t2);

struct RayPayload
{
    float4 color;
};

[shader("raygeneration")]
void rayGen()
{
    uint3 launchIndex = DispatchRaysIndex();
    uint3 launchDim = DispatchRaysDimensions();

    float2 crd = float2(launchIndex.xy);
    float2 dims = float2(launchDim.xy);

    float2 d = ((crd/dims) * 2.f - 1.f);
    float aspectRatio = dims.x / dims.y;

    RayDesc ray;
    ray.Origin = cameraPosition;
    ray.Direction = normalize(float3(d.x * aspectRatio, -d.y, 1));

    ray.TMin = 0;
    ray.TMax = 100000;

    RayPayload payload;
    TraceRay( gRtScene,
        0 /*rayFlags*/,
        0xFF, 
        0 /* ray index*/,
        0/* Multiplies */,
        0/* Miss index */,
        ray,
        payload );

    float4 col = 
    gOutput[launchIndex.xy] = linearToSrgb(payload.color);
}

[shader("miss")]
void miss(inout RayPayload payload)
{
    payload.color = backgroundColor;
}

float3 HitAttribute(float3 vertexAttribute[3], BuiltInTriangleIntersectionAttributes attr)
{
	return vertexAttribute[0] +
		attr.barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]) +
		attr.barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
}

[shader("closesthit")]
void chs(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    float3 hitPosition = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();

    // Get the base index of the triangle's first 16 bit index.
    uint indexSizeInBytes = 2;
    uint indicesPerTriangle = 3;
    uint triangleIndexStride = indicesPerTriangle * indexSizeInBytes;
    uint baseIndex = PrimitiveIndex() * triangleIndexStride;

    // Load up 3 16 bit indices for the triangle.
    const uint3 indices = Load3x16BitIndices(Indices, baseIndex);

    // Retrieve corresponding vertex normals for the triangle vertices.
    float3 vertexNormals[3] = {
        Vertices[indices[0]].Normal,
        Vertices[indices[1]].Normal,
        Vertices[indices[2]].Normal
    };

    float3 hitNormal = HitAttribute(vertexNormals, attribs);

    // Calculate final color.
    float4 phongColor = CalculatePhongLighting(primitiveAlbedo, hitNormal, diffuseCoef, specularCoef, specularPower);
    payload.color = phongColor;
}
