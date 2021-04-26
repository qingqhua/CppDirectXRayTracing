#pragma once
#include "RTX/D3D12GraphicsContext.hpp"
#include "RTX/D3D12AccelerationStructures.hpp"
#include "RTX/D3D12RTPipeline.hpp"

#include "RTX/Structs/FrameObject.hpp"
#include "RTX/Structs/HitProgram.hpp"
#include "RTX/Structs/ExportAssociation.hpp"
#include "RTX/Structs/ShaderConfig.hpp"
#include "RTX/Structs/PipelineConfig.hpp"

namespace CppDirectXRayTracing04 {
    class Application : public Tutorial
    {
    public:
        void onLoad(HWND winHandle, uint32_t winWidth, uint32_t winHeight) override;
        void onFrameRender() override;
        void onShutdown() override;

        void InitDXR(HWND winHandle, uint32_t winWidth, uint32_t winHeight);
        void CreateAccelerationStructures();
        void CreateRtPipelineState();

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
        
        // Acceleration Structure
        ID3D12ResourcePtr mpVertexBuffer;
        ID3D12ResourcePtr mpTopLevelAS;
        ID3D12ResourcePtr mpBottomLevelAS;
        uint64_t mTlasSize = 0;

        // Pipeline state
        ID3D12StateObjectPtr mpPipelineState;
        ID3D12RootSignaturePtr mpEmptyRootSig;
    };
}

