#pragma once
#include "RTX/D3D12GraphicsContext.hpp"
#include "RTX/D3D12AccelerationStructures.hpp"
#include "RTX/D3D12RTPipeline.hpp"

#include "RTX/Structs/FrameObject.hpp"
#include "RTX/Structs/HitProgram.hpp"
#include "RTX/Structs/ExportAssociation.hpp"
#include "RTX/Structs/ShaderConfig.hpp"
#include "RTX/Structs/PipelineConfig.hpp"

namespace CppDirectXRayTracing18 {
    class Application : public Tutorial
    {
    public:
        void onLoad(HWND winHandle, uint32_t winWidth, uint32_t winHeight) override;
        void onFrameRender() override;
        void onShutdown() override;

        void InitDXR(HWND winHandle, uint32_t winWidth, uint32_t winHeight);
        void CreateAccelerationStructures();

        void testCreateAccelerationStructures();
        AccelerationStructureBuffers createTopLevelAS(ID3D12Device5Ptr pDevice, ID3D12GraphicsCommandList4Ptr pCmdList, ID3D12ResourcePtr pBottomLevelAS[2], uint64_t& tlasSize);
        AccelerationStructureBuffers createBottomLevelAS(ID3D12Device5Ptr pDevice, ID3D12GraphicsCommandList4Ptr pCmdList, ID3D12ResourcePtr pVB[], const uint32_t vertexCount[], uint32_t geometryCount);
        ID3D12ResourcePtr createPlaneVB(ID3D12Device5Ptr pDevice);
        ID3D12ResourcePtr createTriangleVB(ID3D12Device5Ptr pDevice);


        void CreateRtPipelineState();
        void CreateShaderTable();
        void createShaderResources();

        uint32_t beginFrame();
        void endFrame(uint32_t rtvIndex);

    private:
        static const uint32_t kRtvHeapSize = 3;
        static const uint32_t kSrvUavHeapSize = 2;
        static const uint32_t kNumSubobjects = 12;
        static const uint32_t kMaxTraceRecursionDepth = 5;

        std::unique_ptr<D3D12GraphicsContext> mContext;
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
        std::unique_ptr<D3D12AccelerationStructures> mAccelerateStruct;
        AccelerationStructureBuffers mBottomLevelBuffers[2];
        ID3D12ResourcePtr mTopLevelAS, mBottomLevelAS[2];
   
        uint64_t mTlasSize = 0;

        // Pipeline state
        std::unique_ptr<D3D12RTPipeline> mRtpipe;
        ID3D12StateObjectPtr mpPipelineState;
        ID3D12RootSignaturePtr mpEmptyRootSig;

        // Shader table
        ID3D12ResourcePtr mpShaderTable;
        uint32_t mShaderTableEntrySize = 0;

        // Shader Resource
        ID3D12ResourcePtr mpOutputResource;
        ID3D12DescriptorHeapPtr mpSrvUavHeap;

    };
}

