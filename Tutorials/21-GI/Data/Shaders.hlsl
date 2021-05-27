#include "Helpers.hlsli"


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
	payload.recursionDepth = 0;
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
	uint random_seed = initRand(DispatchRaysIndex().x * frameindex, DispatchRaysIndex().y * frameindex, 16);

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

	float4 color;

	// Direct lighting
	color = float4(DiffuseShade(hitPosition, hitNormal, diffuseColor, random_seed), 1.0f);
	payload.seed = random_seed;
	payload.recursionDepth++;

	// Indirect lighting
	if (payload.recursionDepth < MaxRecursionDepth)
	{
		float3 dir = CosineWeightedHemisphereSample(payload.seed, hitNormal);
		float3 indirectcolor = ShootIndirectRay(hitPosition, dir, 0.001f, 100000.0f, payload.seed, payload.recursionDepth);
		float4 direct = float4(DiffuseShade(hitPosition, hitNormal, diffuseColor, payload.seed), 1.0f);

		color += diffuseColor * float4(indirectcolor, 1.0f);
	}
	//	// Shadow
	//	RayDesc shadowRay;
	//	shadowRay.Origin = hitPosition;
	//	shadowRay.Direction = normalize(lightPosition - shadowRay.Origin);
	//	shadowRay.TMin = 0.01;
	//	shadowRay.TMax = 100000;
	//	ShadowPayload shadowPayload;
	//	TraceRay(gRtScene,
	//		0  /*rayFlags*/,
	//		0xFF,
	//		4 /* ray index*/,
	//		0 /* Multiplies */,
	//		1 /* Miss index (shadow) */,
	//		shadowRay,
	//		shadowPayload);

	//	// Reflection    
	//	RayDesc reflectionRay;
	//	reflectionRay.Origin = hitPosition;
	//	reflectionRay.Direction = reflect(WorldRayDirection(), hitNormal);
	//	reflectionRay.TMin = 0.01;
	//	reflectionRay.TMax = 100000;
	//	RayPayload reflectionPayload;
	//	reflectionPayload.recursionDepth = payload.recursionDepth + 1;
	//	TraceRay(gRtScene,
	//		0  /*rayFlags*/,
	//		0xFF,
	//		0 /* ray index*/,
	//		0 /* Multiplies */,
	//		0 /* Miss index (raytrace) */,
	//		reflectionRay,
	//		reflectionPayload);
	//	float4 reflectionColor = reflectionPayload.color;

	//	float3 fresnelR = FresnelReflectanceSchlick(WorldRayDirection(), hitNormal, diffuseColor);
	//	float4 reflectedColor = reflectanceCoef * float4(fresnelR, 1) * reflectionColor;

	//	// Calculate final color.
	//	float4 phongColor = CalculatePhongLighting(diffuseColor, hitNormal, shadowPayload.hit, diffuseCoef, specularCoef, specularPower);
	//	color = phongColor + reflectedColor;
	//}
	//else
	//{
	//	color = CalculatePhongLighting(diffuseColor, hitNormal, false, diffuseCoef, specularCoef, specularPower);
	//}

	// Apply visibility falloff.
	float t = RayTCurrent();
	//color = lerp(color, backgroundColor, 1.0 - exp(-0.000002 * t * t * t));

	payload.color = color;
}

[shader("miss")]
void shadowMiss(inout ShadowPayload payload)
{
	payload.hit = false;
}