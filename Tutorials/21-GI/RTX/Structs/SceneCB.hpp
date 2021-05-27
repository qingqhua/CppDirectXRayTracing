#pragma once
#include "Framework.h"

namespace CppDirectXRayTracing21
{
    // Note that the data need to be aligned in shader code.
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

        // frame index used to generate random seed
        float frameindex;

        glm::vec4 lightAmbientColor;
        glm::vec4 lightDiffuseColor;


    };
};