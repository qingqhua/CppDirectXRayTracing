#include "Framework.h"

namespace CppDirectXRayTracing02
{
    struct FrameObject
    {
        ID3D12CommandAllocatorPtr pCmdAllocator;
        ID3D12ResourcePtr pSwapChainBuffer;
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
    }; //mFrameObjects[kDefaultSwapChainBuffers];
};