#pragma once
#include "Framework.h"

namespace CppDirectXRayTracing21
{
    // The constant buffer defined for each instance.
    struct PrimitiveCB
    {
        glm::vec3 matDiffuse;
        float matRoughness;
        glm::vec3 matSpecular;
        
    };
};