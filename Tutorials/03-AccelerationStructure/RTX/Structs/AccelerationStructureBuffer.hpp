#pragma once
#include "Framework.h"

namespace CppDirectXRayTracing03
{
    struct AccelerationStructureBuffer
    {
        ID3D12Resource pScratch;
        ID3D12Resource pResult;
        ID3D12Resource pInstanceDesc;  // Used only for top-level AS
    }
};