#include "Helpers.hlsli"

//------------------------------------------------------------------------------------------------------
inline float3 ShootIndirectRay(float3 origin, float3 direction, float tmin, float tmax, uint depth = 0)
{
	RayDesc indirectray;
	indirectray.Origin = origin;
	indirectray.Direction = direction;
	indirectray.TMin = tmin;
	indirectray.TMax = tmax;

	RayPayload indirectpayload;
	indirectpayload.color = float4(0.0f, 0.0f, 0.0f,1.0f);
	indirectpayload.recursionDepth = depth + 1;

	TraceRay(
		gRtScene,
		RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,
		~0, // instance mask
		0, // hitgroup index
		0, // geom multiplier
		0, // miss index
		indirectray,
		indirectpayload
	);

	return indirectpayload.color.xyz;
}

//------------------------------------------------------------------------------------------------------
inline float ShootShadowRay(float3 origin, float3 direction, float tmin, float tmax)
{
	RayDesc shadowray;
	shadowray.Origin = origin;
	shadowray.Direction = direction;
	shadowray.TMin = tmin;
	shadowray.TMax = tmax;

	ShadowPayload shadowpayload;
	//shadowpayload.hit = true;

	TraceRay(
		gRtScene,
		0,
		0xFF, // instance mask
		4, // hitgroup index
		0, // geom multiplier
		1, // miss index
		shadowray,
		shadowpayload
	);

	return shadowpayload.hit ? 0.0f : 1.0f;
}

//------------------------------------------------------------------------------------------------------
float3 DiffuseShade(float3 position, float3 normal, float3 diffuse, inout uint seed)
{
	float sample_probability = 1.0f / float(1);

	float3 light_position = lightPosition;
	float3 light_intensity = lightDiffuseColor.xyz;
	float dist_to_light = length(light_position - position);
	float3 dir_to_light = normalize(light_position - position);

	float NdotL = saturate(dot(normal, dir_to_light));
	float is_lit = ShootShadowRay(position, dir_to_light, 0.001f, dist_to_light);
	float3 ray_color = is_lit * light_intensity;

	float ao = 1.0f;
	float ao_samples = 4;
	if (ao_samples > 0)
	{
		float ambient_occlusion = 0.0f;

		for (int i = 0; i < ao_samples; i++)
		{
			float3 ao_dir = CosineWeightedHemisphereSample(seed, normal);
			ambient_occlusion += ShootShadowRay(position, ao_dir, 0.001f, 100.0f);
		}

		ao = ambient_occlusion / float(scene_constants.ao_samples);
	}

	return ((NdotL * ray_color * (diffuse / 3.14f)) / sample_probability) * ao;
}