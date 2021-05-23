
//------------------------------------
// Helper functions
//------------------------------------
static const float3 A = float3(1, 0, 0);
static const float3 B = float3(0, 1, 0);
static const float3 C = float3(0, 0, 1);
static const float3 cameraPosition = float3(0, 0, -3);
static const float4 backgroundColor = float4(1.0, 0.6, 0.2, 1.0);
static const float4 lightAmbientColor = float4(0.2, 0.2, 0.2, 1.0);
static const float3 lightPosition = float3(2.0, 2.0, -4.0);
static const float4 lightDiffuseColor = float4(0.2, 1.0, 0.2, 1.0);
static const float4 lightSpecularColor = float4(1, 1, 1, 1);
static const float4 primitiveAlbedo = float4(0.1, 0.7, 0.6, 1.0);
static const float4 groundAlbedo = float4(1.0, 0.0, 0.0, 1.0);
static const float InShadowRadiance = 0.35f;
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
float4 CalculatePhongLighting(in float4 albedo, in float3 normal, in bool isInShadow, in float diffuseCoef = 1.0, in float specularCoef = 1.0, in float specularPower = 50)
{
	float3 hitPosition = HitWorldPosition();
	float shadowFactor = isInShadow ? InShadowRadiance : 1.0;
	float3 incidentLightRay = normalize(hitPosition - lightPosition);

	// Diffuse component.
	float Kd = CalculateDiffuseCoefficient(hitPosition, incidentLightRay, normal);
	float4 diffuseColor = shadowFactor * diffuseCoef * Kd * lightDiffuseColor * albedo;

	// Specular component.
	float4 specularColor = float4(0, 0, 0, 0);
	if (!isInShadow)
	{
		float4 lightSpecularColor = float4(1, 1, 1, 1);
		float4 Ks = CalculateSpecularCoefficient(hitPosition, incidentLightRay, normal, specularPower);
		specularColor = specularCoef * Ks * lightSpecularColor;
	}

	// Ambient component.
	// Fake AO: Darken faces with normal facing downwards/away from the sky a little bit.
	float4 ambientColor = lightAmbientColor;
	float4 ambientColorMin = lightAmbientColor - 0.15;
	float4 ambientColorMax = lightAmbientColor;
	float a = 1 - saturate(dot(normal, float3(0, -1, 0)));
	ambientColor = albedo * lerp(ambientColorMin, ambientColorMax, a);

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
//ByteAddressBuffer IndicesPlane : register(t3);
//StructuredBuffer<Vertex> VerticesPlane : register(t4);

struct RayPayload
{
	float4 color;
	uint recursionDepth;
};

struct ShadowPayload
{
	bool hit;
};

[shader("raygeneration")]
void rayGen()
{
    uint3 launchIndex = DispatchRaysIndex();
    uint3 launchDim = DispatchRaysDimensions();

    float2 crd = float2(launchIndex.xy);
    float2 dims = float2(launchDim.xy);

    float2 d = ((crd / dims) * 2.f - 1.f);
    float aspectRatio = dims.x / dims.y;

    RayDesc ray;
	ray.Origin = cameraPosition;
    ray.Direction = normalize(float3(d.x * aspectRatio, -d.y, 1));

    ray.TMin = 0;
    ray.TMax = 100000;

    RayPayload payload;
    TraceRay(gRtScene, 0 /*rayFlags*/, 0xFF, 0 /* ray index*/, 2, 0, ray, payload);

    gOutput[launchIndex.xy] = linearToSrgb(payload.color);
}

// miss primary 
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

// Hit primary - triangle
[shader("closesthit")]
void triangleChs(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
	float hitT = RayTCurrent();
	float3 rayDirW = WorldRayDirection();
	float3 rayOriginW = WorldRayOrigin();

	// Find the world-space hit position
	float3 posW = rayOriginW + hitT * rayDirW;

	// Fire a shadow ray. The direction is hard-coded here, but can be fetched from a constant-buffer
	RayDesc ray;
	ray.Origin = posW;
	ray.Direction = normalize(float3(0.5, 0.5, -0.5));
	ray.TMin = 0.01;
	ray.TMax = 100000;
	ShadowPayload shadowPayload;
	TraceRay(gRtScene, 0  /*rayFlags*/, 0xFF, 1 /* ray index*/, 0, 1, ray, shadowPayload);

	// Lighting of the objects.
	float3 hitPosition = HitWorldPosition();

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

	//float3 hitNormal = (InstanceID() == 0) ? float3(0, 1, 0) : HitAttribute(vertexNormals, attribs);
	float3 hitNormal = HitAttribute(vertexNormals, attribs);


	// Calculate final color.
	float4 diffuseColor = (InstanceID() == 0) ? groundAlbedo : primitiveAlbedo;
	float4 phongColor = CalculatePhongLighting(diffuseColor, hitNormal, shadowPayload.hit, diffuseCoef, specularCoef, specularPower);
	float4 color = phongColor;

	float t = RayTCurrent();
	color = lerp(color, backgroundColor, 1.0 - exp(-0.000002 * t * t * t));
	
	payload.color = color;
}

// Hit primary - plane
[shader("closesthit")]
void planeChs(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    float hitT = RayTCurrent();
    float3 rayDirW = WorldRayDirection();
    float3 rayOriginW = WorldRayOrigin();

    // Find the world-space hit position
    float3 posW = rayOriginW + hitT * rayDirW;

    // Fire a shadow ray. The direction is hard-coded here, but can be fetched from a constant-buffer
    RayDesc ray;
    ray.Origin = posW;
    ray.Direction = normalize(float3(0.5, 0.5, -0.5));
    ray.TMin = 0.01;
    ray.TMax = 100000;
    ShadowPayload shadowPayload;
    TraceRay(gRtScene, 0  /*rayFlags*/, 0xFF, 1 /* ray index*/, 0, 1, ray, shadowPayload);

	// Lighting of the objects.
	float3 hitPosition = HitWorldPosition();


	// Retrieve corresponding vertex normals for the triangle vertices.
	float3 vertexNormals[3] = {
		float3(0,1,0),
		float3(0,1,0),
		float3(0,1,0)
	};

	//float3 hitNormal = (InstanceID() == 0) ? float3(0, 1, 0) : HitAttribute(vertexNormals, attribs);
	float3 hitNormal = HitAttribute(vertexNormals, attribs);
	// Calculate final color.
	float4 diffuseColor = primitiveAlbedo;
	float4 phongColor = CalculatePhongLighting(diffuseColor, hitNormal, shadowPayload.hit, diffuseCoef, specularCoef, specularPower);
	float4 color = phongColor;
	float t = RayTCurrent();
	color = lerp(color, backgroundColor, 1.0 - exp(-0.000002 * t * t * t));

    float factor = shadowPayload.hit ? 0.1 : 1.0;
    payload.color = color * factor;
}

// Hit shadow
[shader("closesthit")]
void shadowChs(inout ShadowPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    payload.hit = true;
}

// Miss shadow
[shader("miss")]
void shadowMiss(inout ShadowPayload payload)
{
    payload.hit = false;
}

