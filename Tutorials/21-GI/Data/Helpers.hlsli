/*
 * ----------------------------------------
 * HELPER FUNCTIONS
 * ----------------------------------------
 */
#ifndef __HELPER_HLSL__
#define __HELPER_HLSL__

static float M_PI = 3.1415f;
static float gt_min = 0.01f;
static float gt_max = 1000.0f;

struct RayPayload
{
    float4 color;
    uint recursionDepth;
    uint seed;
};

struct ShadowPayload
{
    bool hit;
};

struct Vertex
{
    float3 Position;
    float3 Normal;
    float3 Tangent;
    float2 TexCoord;
};

cbuffer SceneCB : register(b0)
{
    float3 backgroundColor			: packoffset(c0);
    float  MaxRecursionDepth        : packoffset(c0.w);
    float3 cameraPosition			: packoffset(c1);
    float frameindex                : packoffset(c1.w);
    float3 lightPosition			: packoffset(c2);
    uint aoSamples                  : packoffset(c2.w);
    float3 lightIntensity		    : packoffset(c3);
    bool ggxshadingMode             : packoffset(c3.w);
};

cbuffer PrimitiveCB : register(b1)
{
    float3 matDiffuse		: packoffset(c0);
    float  matRoughness    : packoffset(c0.w);
    float3 matSpecular		: packoffset(c1);
}

RaytracingAccelerationStructure gRtScene : register(t0);
RWTexture2D<float4>             gOutput	 : register(u0);
ByteAddressBuffer               Indices	 : register(t1);
StructuredBuffer<Vertex>        Vertices : register(t2);

// Retrieve hit world position.
float3 HitWorldPosition()
{
    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

// Load three 16 bit indices from a byte addressed buffer.
// From: https://github.com/Jorgemagic/CSharpDirectXRaytracing
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

// Cosine weighted hemisphere sampling
// From: http://intro-to-dxr.cwyman.org/
float3 GetPerpendicularVector(float3 u)
{
    float3 a = abs(u);
    uint xm = ((a.x - a.y) < 0 && (a.x - a.z) < 0) ? 1 : 0;
    uint ym = (a.y - a.z) < 0 ? (1 ^ xm) : 0;
    uint zm = 1 ^ (xm | ym);
    return cross(u, float3(xm, ym, zm));
}

// Generate a pseudorandom float in [0..1] from random seed.
// From: http://intro-to-dxr.cwyman.org/
float nextRand(inout uint s)
{
    s = (1664525u * s + 1013904223u);
    return float(s & 0x00FFFFFF) / float(0x01000000);
}

// From: http://intro-to-dxr.cwyman.org/
uint initRand(uint val0, uint val1, uint backoff = 16)
{
    uint v0 = val0, v1 = val1, s0 = 0;

    [unroll]
    for (uint n = 0; n < backoff; n++)
    {
        s0 += 0x9e3779b9;
        v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
        v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
    }
    return v0;
}

// From: http://intro-to-dxr.cwyman.org/
float3 CosineWeightedHemisphereSample(inout uint seed, float3 normal)
{
    float2 random = float2(nextRand(seed), nextRand(seed));

    float3 bitangent = GetPerpendicularVector(normal);
    float3 tangent = cross(bitangent, normal);
    float r = sqrt(random.x);
    float phi = 2.0f * 3.14159265f * random.y;

    return tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + normal.xyz * sqrt(1 - random.x);
}

//------------------------------------------------------------------------------------------------------
inline float3 ShootIndirectRay(float3 origin, float3 direction, float tmin, float tmax, uint seed, uint depth)
{
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = tmin;
    ray.TMax = tmax;

    RayPayload pay;
    pay.color = float4(0.0f, 0.0f, 0.0f, 1.0f);
    pay.recursionDepth = depth + 1;
    pay.seed = seed;

    TraceRay(
        gRtScene,
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,
        0xFF, // instance mask
        0, // hitgroup index
        0, // geom multiplier
        0, // miss index
        ray,
        pay
    );

    return pay.color.xyz;
}


//------------------------------------------------------------------------------------------------------
inline float ShootShadowRay(float3 origin, float3 direction, float tmin, float tmax)
{
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = tmin;
    ray.TMax = tmax;

    ShadowPayload pay;
    pay.hit = true;

    TraceRay(
        gRtScene,
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,
        0xFF,
        4,
        0,
        1,
        ray,
        pay
    );

    return (pay.hit == false) ? 1.0f : 0.0f;
}

#endif
