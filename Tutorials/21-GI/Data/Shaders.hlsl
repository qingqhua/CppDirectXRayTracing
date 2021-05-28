#include "GGX.hlsli"
#include "Lambertian.hlsli"

[shader("raygeneration")]
void rayGen()
{
    uint3 launchIndex = DispatchRaysIndex();
    uint3 launchDim = DispatchRaysDimensions();

    float2 crd = float2(launchIndex.xy);
    float2 dims = float2(launchDim.xy);

    float2 d = ((crd/dims) * 2.f - 1.f);
    float aspectRatio = dims.x / dims.y;

	// Initialize random seed based on pixel and frame for random sample
	uint random_seed = initRand(DispatchRaysIndex().x * frameindex, DispatchRaysIndex().y * frameindex, 16);

    RayDesc ray;
    ray.Origin = cameraPosition;
    ray.Direction = normalize(float3(d.x * aspectRatio, -d.y, 1));

    ray.TMin = 0;
    ray.TMax = 100000;

    RayPayload payload;
	payload.recursionDepth = 0;
	payload.seed = random_seed;
	TraceRay(gRtScene,
		0 /*rayFlags*/,
		0xFF,
		0 /* ray index*/,
		0/* Multiplies */,
		0/* Miss index */,
		ray,
		payload);

	// The final output of each pixel.
    gOutput[launchIndex.xy] = payload.color;
}

[shader("miss")]
void miss(inout RayPayload payload)
{
	//payload.color = float4(backgroundColor, 1.0f);
	payload.color = float4(0,0,0, 1.0f);
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
	//-----------------------
	// Get geometry attribute
	//-----------------------
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
	float3 hitNormal = (InstanceID() == 0) ? float3(0, 1, 0) : HitAttribute(vertexNormals, attribs);

	float3 view_dir = normalize(cameraPosition - hitPosition);
	
	// Direct lighting
	//float3 color = LambertianDirect(hitPosition, hitNormal, matDiffuse, payload.seed);
	float3 color = ggxDirect(payload.seed, hitPosition, lightPosition, lightIntensity, hitNormal, view_dir, matDiffuse, matSpecular, matRoughness);
	//float3 color = float3(0, 0, 0);
	// Indirect lighting
	if (payload.recursionDepth < MaxRecursionDepth)
	{
		// GGX
		float3 indirect = ggxIndirect(payload.seed, hitPosition, lightPosition, lightIntensity, hitNormal, view_dir, matDiffuse, matSpecular, matRoughness, payload.recursionDepth);

		// Lambertian
		//float3 indirect = LambertianIndirect(hitPosition, hitNormal, matDiffuse, payload.seed, payload.recursionDepth);

		color += indirect;
		payload.recursionDepth++;
	}
	
	payload.color = float4(color, 1.0f);
}

[shader("miss")]
void shadowMiss(inout ShadowPayload payload)
{
	payload.hit = false;
}