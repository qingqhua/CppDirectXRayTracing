#pragma once
#include "Framework.h"

namespace CppDirectXRayTracing15
{
    struct AccelerationStructureBuffers
    {
        ID3D12ResourcePtr pScratch;
        ID3D12ResourcePtr pResult;
        ID3D12ResourcePtr pInstanceDesc;  // Used only for top-level AS
    };
};