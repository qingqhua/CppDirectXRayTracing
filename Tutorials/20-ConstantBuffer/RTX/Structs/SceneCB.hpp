#pragma once
#include "Framework.h"

namespace CppDirectXRayTracing20
{
    struct SceneCB
    {
        glm::mat4 projectionToWorld;
        glm::vec4 backgroundColor;

        // Camera
        glm::vec3 cameraPosition;

        // Ray recurion depth
        float MaxRecursionDepth;

        // Light
        glm::vec3 lightPosition;
        glm::vec4 lightAmbientColor;
        glm::vec4 lightDiffuseColor;
    };
};