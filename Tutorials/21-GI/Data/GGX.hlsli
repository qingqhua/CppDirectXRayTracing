//-------------------------------------
// Microfacet BRDF model introduced by Cook and Torrance
// Most of the code modified from http://intro-to-dxr.cwyman.org/
//-------------------------------------

#include "Helpers.hlsli"

// D term
float normalDistribution(float NdotH, float roughness)
{
	float a2 = roughness * roughness;
	float d = ((NdotH * a2 - NdotH) * NdotH + 1);
	return a2 / (d * d * M_PI);
}

// G term
float schlickMaskingTerm(float NdotL, float NdotV, float roughness)
{
	// Karis notes they use alpha / 2 (or roughness^2 / 2)
	float k = roughness * roughness / 2;

	// Compute G(v) and G(l).  These equations directly from Schlick 1994
	//     (Though note, Schlick's notation is cryptic and confusing.)
	float g_v = NdotV / (NdotV * (1 - k) + k);
	float g_l = NdotL / (NdotL * (1 - k) + k);
	return g_v * g_l;
}

// F term
float3 schlickFresnel(float3 f0, float lDotH)
{
	return f0 + (float3(1.0f, 1.0f, 1.0f) - f0) * pow(1.0f - lDotH, 5.0f);
}

// Gets a random microfacet orientation (i.e., a facet normal) 
// Randomly choose what direction a ray bounces when it leaves a specular surface
// When using this function to sample, the probability density is:
//      pdf = D * NdotH / (4 * HdotV)
float3 getGGXMicrofacet(inout uint randSeed, float roughness, float3 hitNorm)
{
	// Get our uniform random numbers
	float2 randVal = float2(nextRand(randSeed), nextRand(randSeed));

	// Get an orthonormal basis from the normal
	float3 B = GetPerpendicularVector(hitNorm);
	float3 T = cross(B, hitNorm);

	// GGX NDF sampling
	float a2 = roughness * roughness;
	float cosThetaH = sqrt(max(0.0f, (1.0 - randVal.x) / ((a2 - 1.0) * randVal.x + 1)));
	float sinThetaH = sqrt(max(0.0f, 1.0f - cosThetaH * cosThetaH));
	float phiH = randVal.y * M_PI * 2.0f;

	// Get our GGX NDF sample (i.e., the half vector)
	return T * (sinThetaH * cos(phiH)) +
		B * (sinThetaH * sin(phiH)) +
		hitNorm * cosThetaH;
}

float3 ggxDirect(inout uint rndSeed, float3 hitPosition, float3 lightPosition, float3 lightIntensity, float3 N, float3 V,
	float3 dif, float3 spec, float rough)
{
	// Pick a random light from our scene to shoot a shadow ray towards
	//int lightToSample = min(int(nextRand(rndSeed) * gLightsCount),
		//gLightsCount - 1);

	// Query the scene to find info about the randomly selected light
	float dist_to_light = length(lightPosition - hitPosition);
	float3 L = normalize(lightPosition - hitPosition);

	// Compute our lambertion term (N dot L)
	float NdotL = saturate(dot(N, L));

	// Shoot our shadow ray to our randomly selected light
	float is_lit = ShootShadowRay(hitPosition, L, 0.001f, dist_to_light);

	// Compute half vectors and additional dot products for GGX
	float3 H = normalize(V + L);
	float NdotH = saturate(dot(N, H));
	float LdotH = saturate(dot(L, H));
	float NdotV = saturate(dot(N, V));

	// Evaluate terms for our GGX BRDF model
	float  D = normalDistribution(NdotH, rough);
	float  G = schlickMaskingTerm(NdotL, NdotV, rough);
	float3 F = schlickFresnel(spec, LdotH);

	// Evaluate the Cook-Torrance Microfacet BRDF model
	//     Cancel NdotL here to avoid catastrophic numerical precision issues.
	float3 ggxTerm = D * G * F / (4 * NdotV /* * NdotL */);

	// Compute our final color (combining diffuse lobe plus specular GGX lobe)
	return is_lit * lightIntensity * ( /* NdotL * */ ggxTerm +
		NdotL * dif / M_PI);
}

float luminance(float3 rgb)
{
	float red = rgb.x;
	float green = rgb.y;
	float blue = rgb.z;
	return (red / 255.0) * 0.3 + (green / 255.0) * 0.59 + (blue / 255.0) * 0.11;
}

float probabilityToSampleDiffuse(float3 difColor, float3 specColor)
{
	float lumDiffuse = max(0.01f, luminance(difColor.rgb));
	float lumSpecular = max(0.01f, luminance(specColor.rgb));
	return lumDiffuse / (lumDiffuse + lumSpecular);
}

float3 ggxIndirect(inout uint rndSeed, float3 hit, float3 lightPosition, float3 lightIntensity, float3 N, float3 V,
	float3 dif, float3 spec, float rough, uint rayDepth)
{
	// We have to decide whether we sample our diffuse or specular/ggx lobe.
	float probDiffuse = probabilityToSampleDiffuse(dif, spec);
	float chooseDiffuse = (nextRand(rndSeed) < probDiffuse);

	// We'll need NdotV for both diffuse and specular...
	float NdotV = saturate(dot(N, V));

	// If we randomly selected to sample our diffuse lobe...
	if (chooseDiffuse)
	{
		// Shoot a randomly selected cosine-sampled diffuse ray.
		float3 L = CosineWeightedHemisphereSample(rndSeed, N);
		float3 bounceColor = ShootIndirectRay(hit, L, 0.01f, 100000.0f, rndSeed, rayDepth);

		// Check to make sure our randomly selected, normal mapped diffuse ray didn't go below the surface.
		//if (dot(N, L) <= 0.0f) bounceColor = float3(0, 0, 0);

		// Accumulate the color: (NdotL * incomingLight * dif / pi) 
		// Probability of sampling:  (NdotL / pi) * probDiffuse
		return bounceColor * dif / probDiffuse;
	}
	// Otherwise we randomly selected to sample our GGX lobe
	else
	{
		// Randomly sample the NDF to get a microfacet in our BRDF to reflect off of
		float3 H = getGGXMicrofacet(rndSeed, rough, N);

		// Compute the outgoing direction based on this (perfectly reflective) microfacet
		float3 L = normalize(2.f * dot(V, H) * H - V);

		// Compute our color by tracing a ray in this direction
		float3 bounceColor = ShootIndirectRay(hit, L, 0.01f, 100000.0f, rndSeed, rayDepth);

		// Check to make sure our randomly selected, normal mapped diffuse ray didn't go below the surface.
		//if (dot(N, L) <= 0.0f) bounceColor = float3(0, 0, 0);

		// Compute some dot products needed for shading
		float  NdotL = saturate(dot(N, L));
		float  NdotH = saturate(dot(N, H));
		float  LdotH = saturate(dot(L, H));

		// Evaluate our BRDF using a microfacet BRDF model
		float  D = normalDistribution(NdotH, rough);          // The GGX normal distribution
		float  G = schlickMaskingTerm(NdotL, NdotV, rough);   // Use Schlick's masking term approx
		float3 F = schlickFresnel(spec, LdotH);                  // Use Schlick's approx to Fresnel
		float3 ggxTerm = D * G * F / (4 * NdotL * NdotV);        // The Cook-Torrance microfacet BRDF

		// What's the probability of sampling vector H from getGGXMicrofacet()?
		float  ggxProb = D * NdotH / (4 * LdotH);

		// Accumulate the color:  ggx-BRDF * incomingLight * NdotL / probability-of-sampling
		//    -> Should really simplify the math above.
		return NdotL * bounceColor * ggxTerm / (ggxProb * (1.0f - probDiffuse));
	}
}