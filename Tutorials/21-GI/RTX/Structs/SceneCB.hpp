#pragma once
#include "Framework.h"

namespace CppDirectXRayTracing21
{
    // Note that the data need to be aligned in shader code.
    struct SceneCB
    {
        glm::vec3 backgroundColor;

        // Ray recurion depth
        float MaxRecursionDepth;

        // Camera
        glm::vec3 cameraPosition;

        // frame index used to generate random seed
        float frameindex;

        // Light
        glm::vec3 lightPosition;
        uint aoSamples;
        glm::vec3 lightIntensity;

        bool ggxshadingMode;
    };
};