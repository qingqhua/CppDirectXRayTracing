#pragma once
#include "Framework.h"

namespace CppDirectXRayTracing07
{
    struct AccelerationStructureBuffers
    {
        ID3D12ResourcePtr pScratch;
        ID3D12ResourcePtr pResult;
        ID3D12ResourcePtr pInstanceDesc;  // Used only for top-level AS
    };
};