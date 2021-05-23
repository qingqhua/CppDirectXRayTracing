#pragma once
#include "Framework.h"

namespace CppDirectXRayTracing20
{
    struct HeapData
    {
        ID3D12DescriptorHeapPtr pHeap;
        uint32_t usedEntries = 0;
    };

    static const D3D12_HEAP_PROPERTIES kUploadHeapProps =
    {
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        D3D12_MEMORY_POOL_UNKNOWN,
        0,
        0,
    };

    static const D3D12_HEAP_PROPERTIES kDefaultHeapProps =
    {
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        D3D12_MEMORY_POOL_UNKNOWN,
        0,
        0
    };
}