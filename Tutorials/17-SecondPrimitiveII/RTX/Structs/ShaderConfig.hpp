#pragma once
#include "Framework.h"

namespace CppDirectXRayTracing17II
{
    struct ShaderConfig
    {
        ShaderConfig(uint32_t maxAttributeSizeInBytes, uint32_t maxPayloadSizeInBytes)
        {
            shaderConfig.MaxAttributeSizeInBytes = maxAttributeSizeInBytes;
            shaderConfig.MaxPayloadSizeInBytes = maxPayloadSizeInBytes;

            subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
            subobject.pDesc = &shaderConfig;
        }

        D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
        D3D12_STATE_SUBOBJECT subobject = {};
    };
}