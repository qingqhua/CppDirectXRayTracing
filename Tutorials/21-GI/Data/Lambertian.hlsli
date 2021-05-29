
#include "Helpers.hlsli"

//------------------------------------------------------------------------------------------------------
float3 LambertianDirect(float3 position, float3 normal, float3 diffuse, inout uint seed)
{
    float sample_probability = 1.0f / float(1);

    float3 light_intensity = lightIntensity.xyz;
    float dist_to_light = length(lightPosition.xyz - position);
    float3 dir_to_light = normalize(lightPosition.xyz - position);

    float NdotL = saturate(dot(normal, dir_to_light));
    float is_lit = ShootShadowRay(position, dir_to_light, 0.001f, dist_to_light);
    float3 ray_color = is_lit * light_intensity;

    float ao = 1.0f;

    if (aoSamples > 0)
    {
        float ambient_occlusion = 0.0f;

        for (int i = 0; i < aoSamples; i++)
        {
            float3 ao_dir = CosineWeightedHemisphereSample(seed, normal);
            ambient_occlusion += ShootShadowRay(position, ao_dir, 0.001f, 100.0f);
        }

        ao = ambient_occlusion / float(aoSamples);
    }

    return ((NdotL * ray_color * (diffuse / 3.14f)) / sample_probability) * ao;
}

//------------------------------------------------------------------------------------------------------
float3 LambertianIndirect(float3 position, float3 normal, float3 diffuse, inout uint seed, uint depth)
{
    float3 dir = CosineWeightedHemisphereSample(seed, normal);
    float3 indirect = ShootIndirectRay(position, dir, gt_min, gt_max, seed, depth);
    return diffuse * indirect;
}