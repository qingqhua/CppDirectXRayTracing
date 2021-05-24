#pragma once
#include "Framework.h"

namespace CppDirectXRayTracing21
{
    // The constant buffer defined for each instance.
    struct PrimitiveCB
    {
        glm::vec4 diffuseColor;
        float inShadowRadiance;
        float diffuseCoef;
        float specularCoef;
        float specularPower;
        float reflectanceCoef;
    };
};