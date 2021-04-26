#pragma once

#include "RTX/FrameObject.hpp"
#include "RTX/HeapData.hpp"
#include "RTX/D3D12GraphicsContext.hpp"

namespace CppDirectXRayTracing02 {
    class Tutorial02 : public Tutorial
    {
    public:
        void onLoad(HWND winHandle, uint32_t winWidth, uint32_t winHeight) override;
        void onFrameRender() override;
        void onShutdown() override;

        void initDXR(HWND winHandle, uint32_t winWidth, uint32_t winHeight);
        uint32_t beginFrame();
        void endFrame(uint32_t rtvIndex);

    private:
        D3D12GraphicsContext mContext;
        static const uint32_t kRtvHeapSize = 3;
    
        std::vector<FrameObject> mFrameObjects;
        HeapData mRtvHeap;
        HWND mHwnd = nullptr;
        ID3D12Device5Ptr mpDevice;
        ID3D12CommandQueuePtr mpCmdQueue;
        IDXGISwapChain3Ptr mpSwapChain;
        uvec2 mSwapChainSize;
        ID3D12GraphicsCommandList4Ptr mpCmdList;
        ID3D12FencePtr mpFence;
        HANDLE mFenceEvent;
        uint64_t mFenceValue = 0;
    };
}

