#pragma once
#include "Framework.h"

namespace CppDirectXRayTracing03
{
    struct HeapData
    {
        ID3D12DescriptorHeapPtr pHeap;
        uint32_t usedEntries = 0;
    };
}