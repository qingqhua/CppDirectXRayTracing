#pragma once
#include "15-Primitive.hpp"

void CppDirectXRayTracing15::Application::InitDXR(HWND winHandle, uint32_t winWidth, uint32_t winHeight)
{
    mContext = std::make_unique<D3D12GraphicsContext>();
    mAccelerateStruct = std::make_unique<D3D12AccelerationStructures>();
    mRtpipe = std::make_unique<D3D12RTPipeline>();

    for (uint32_t i = 0; i < mContext->kDefaultSwapChainBuffers; i++)
    {
        mFrameObjects.push_back(FrameObject());
    }

    mHwnd = winHandle;
    mSwapChainSize = uvec2(winWidth, winHeight);

    // Initialize the debug layer for debug builds
#ifdef _DEBUG
    ID3D12DebugPtr pDebug;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebug))))
    {
        pDebug->EnableDebugLayer();
    }
#endif
    // Create the DXGI factory
    IDXGIFactory4Ptr pDxgiFactory;
    d3d_call(CreateDXGIFactory1(IID_PPV_ARGS(&pDxgiFactory)));
    mpDevice = mContext->createDevice(pDxgiFactory);
    mpCmdQueue = mContext->createCommandQueue(mpDevice);
    mpSwapChain = mContext->createDxgiSwapChain(pDxgiFactory, mHwnd, winWidth, winHeight, DXGI_FORMAT_R8G8B8A8_UNORM, mpCmdQueue);

    // Create a RTV descriptor heap
    mRtvHeap.pHeap = mContext->createDescriptorHeap(mpDevice, kRtvHeapSize, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, false);

    // Create the per-frame objects
    for (uint32_t i = 0; i < mContext->kDefaultSwapChainBuffers; i++)
    {
        d3d_call(mpDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mFrameObjects[i].pCmdAllocator)));
        d3d_call(mpSwapChain->GetBuffer(i, IID_PPV_ARGS(&mFrameObjects[i].pSwapChainBuffer)));
        mFrameObjects[i].rtvHandle = mContext->createRTV(mpDevice, mFrameObjects[i].pSwapChainBuffer, mRtvHeap.pHeap, mRtvHeap.usedEntries, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
    }

    // Create the command-list
    d3d_call(mpDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mFrameObjects[0].pCmdAllocator, nullptr, IID_PPV_ARGS(&mpCmdList)));

    // Create a fence and the event
    d3d_call(mpDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mpFence)));
    mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void CppDirectXRayTracing15::Application::CreateAccelerationStructures()
{
    int vertexCount = 0;
    int indexCount = 0;

    // Create primitive vertex buffer and index buffer.
    mpVertexBuffer = mAccelerateStruct->CreatePrimitiveVB(mpDevice, vertexCount);
    mpIndexBuffer = mAccelerateStruct->CreatePrimitiveIB(mpDevice, indexCount);

    //mpVertexBuffer = mAccelerateStruct->createCubeVB(mpDevice, vertexCount);
    //mpIndexBuffer = mAccelerateStruct->createCubeIB(mpDevice, indexCount);


    // Bind primitive vertex buffer and index buffer to BLAS.
    CppDirectXRayTracing15::AccelerationStructureBuffers bottomLevelBuffers = mAccelerateStruct->createBottomLevelAS(mpDevice, mpCmdList, mpVertexBuffer, mpIndexBuffer,vertexCount,indexCount);

    AccelerationStructureBuffers topLevelBuffers = mAccelerateStruct->createTopLevelAS(mpDevice, mpCmdList, bottomLevelBuffers.pResult, mTlasSize);

    // The tutorial doesn't have any resource lifetime management, so we flush and sync here. This is not required by the DXR spec - you can submit the list whenever you like as long as you take care of the resources lifetime.
    mFenceValue = mContext->submitCommandList(mpCmdList, mpCmdQueue, mpFence, mFenceValue);
    mpFence->SetEventOnCompletion(mFenceValue, mFenceEvent);
    WaitForSingleObject(mFenceEvent, INFINITE);
    uint32_t bufferIndex = mpSwapChain->GetCurrentBackBufferIndex();
    mpCmdList->Reset(mFrameObjects[0].pCmdAllocator, nullptr);

    // Store the AS buffers. The rest of the buffers will be released once we exit the function
    mpTopLevelAS = topLevelBuffers.pResult;
    mpBottomLevelAS = bottomLevelBuffers.pResult;
}

void CppDirectXRayTracing15::Application::CreateRtPipelineState()
{
    // Need 10 subobjects:
    //  1 for the DXIL library
    //  1 for hit-group
    //  2 for RayGen root-signature (root-signature and the subobject association)
    //  2 for the root-signature shared between miss and hit shaders (signature and association)
    //  2 for shader config (shared between all programs. 1 for the config, 1 for association)
    //  1 for pipeline config
    //  1 for the global root signature
    
    std::array<D3D12_STATE_SUBOBJECT, kNumSubobjects> subobjects;
    uint32_t index = 0;

    // Create the DXIL library
    DxilLibrary dxilLib = mRtpipe->createDxilLibrary();
    subobjects[index++] = dxilLib.stateSubobject; // 0 Library

    HitProgram hitProgram(nullptr, mRtpipe->kClosestHitShader, mRtpipe->kHitGroup);
    subobjects[index++] = hitProgram.subObject; // 1 Hit Group

    // Create the ray-gen root-signature and association
    LocalRootSignature rgsRootSignature(mpDevice, mRtpipe->createRayGenRootDesc().desc);
    subobjects[index] = rgsRootSignature.subobject; // 2 RayGen Root Sig

    uint32_t rgsRootIndex = index++; // 2
    ExportAssociation rgsRootAssociation(&mRtpipe->kRayGenShader, 1, &(subobjects[rgsRootIndex]));
    subobjects[index++] = rgsRootAssociation.subobject; // 3 Associate Root Sig to RGS

    // Create the miss- and hit-programs root-signature and association
    D3D12_ROOT_SIGNATURE_DESC emptyDesc = {};
    emptyDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
    LocalRootSignature hitMissRootSignature(mpDevice, emptyDesc);
    subobjects[index] = hitMissRootSignature.subobject; // 4 Root Sig to be shared between Miss and CHS

    uint32_t hitMissRootIndex = index++; // 4
    const WCHAR* missHitExportName[] = { mRtpipe->kMissShader,mRtpipe->kClosestHitShader };
    ExportAssociation missHitRootAssociation(missHitExportName, arraysize(missHitExportName), &(subobjects[hitMissRootIndex]));
    subobjects[index++] = missHitRootAssociation.subobject; // 5 Associate Root Sig to Miss and CHS

    // Bind the payload size to the programs
    ShaderConfig shaderConfig(sizeof(float) * 2, sizeof(float) * 3);
    subobjects[index] = shaderConfig.subobject; // 6 Shader Config

    uint32_t shaderConfigIndex = index++; // 6
    const WCHAR* shaderExports[] = { mRtpipe->kMissShader, mRtpipe->kClosestHitShader, mRtpipe->kRayGenShader };
    ExportAssociation configAssociation(shaderExports, arraysize(shaderExports), &(subobjects[shaderConfigIndex]));
    subobjects[index++] = configAssociation.subobject; // 7 Associate Shader Config to Miss, CHS, RGS

    // Create the pipeline config
    PipelineConfig config(kMaxTraceRecursionDepth);
    subobjects[index++] = config.subobject; // 8

    // Create the global root signature and store the empty signature
    GlobalRootSignature root(mpDevice, {});
    mpEmptyRootSig = root.pRootSig;
    subobjects[index++] = root.subobject; // 9

    // Create the state
    D3D12_STATE_OBJECT_DESC desc;
    desc.NumSubobjects = index; // 10
    desc.pSubobjects = subobjects.data();
    desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

    d3d_call(mpDevice->CreateStateObject(&desc, IID_PPV_ARGS(&mpPipelineState)));
}

void CppDirectXRayTracing15::Application::CreateShaderTable()
{
    /** The shader-table layout is as follows:
        Entry 0 - Ray-gen program
        Entry 1 - Miss program
        Entry 2 - Hit program
        All entries in the shader-table must have the same size, so we will choose it base on the largest required entry.
        The ray-gen program requires the largest entry - sizeof(program identifier) + 8 bytes for a descriptor-table.
        The entry size must be aligned up to D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT
    */

    // Calculate the size and create the buffer
    mShaderTableEntrySize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    mShaderTableEntrySize += 8; // The ray-gen's descriptor table
    mShaderTableEntrySize = align_to(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT, mShaderTableEntrySize);
    uint32_t shaderTableSize = mShaderTableEntrySize * 3;

    // For simplicity, we create the shader-table on the upload heap. You can also create it on the default heap
    mpShaderTable = mAccelerateStruct->createBuffer(mpDevice, shaderTableSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);

    // Map the buffer
    uint8_t* pData;
    d3d_call(mpShaderTable->Map(0, nullptr, (void**)&pData));

    MAKE_SMART_COM_PTR(ID3D12StateObjectProperties);
    ID3D12StateObjectPropertiesPtr pRtsoProps;
    mpPipelineState->QueryInterface(IID_PPV_ARGS(&pRtsoProps));

    // Entry 0 - ray-gen program ID and descriptor data
    memcpy(pData, pRtsoProps->GetShaderIdentifier(mRtpipe->kRayGenShader), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    uint64_t heapStart = mpSrvUavHeap->GetGPUDescriptorHandleForHeapStart().ptr;
    *(uint64_t*)(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = heapStart;

    // Entry 1 - miss program
    memcpy(pData + mShaderTableEntrySize, pRtsoProps->GetShaderIdentifier(mRtpipe->kMissShader), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    // Entry 2 - hit program
    uint8_t* pHitEntry = pData + mShaderTableEntrySize * 2; // +2 skips the ray-gen and miss entries
    memcpy(pHitEntry, pRtsoProps->GetShaderIdentifier(mRtpipe->kHitGroup), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    // Unmap
    mpShaderTable->Unmap(0, nullptr);
}

void CppDirectXRayTracing15::Application::createShaderResources()
{
    // Create the output resource. The dimensions and format should match the swap-chain
    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.DepthOrArraySize = 1;
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // The backbuffer is actually DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, but sRGB formats can't be used with UAVs. We will convert to sRGB ourselves in the shader
    resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    resDesc.Height = mSwapChainSize.y;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resDesc.MipLevels = 1;
    resDesc.SampleDesc.Count = 1;
    resDesc.Width = mSwapChainSize.x;
    d3d_call(mpDevice->CreateCommittedResource(&kDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&mpOutputResource))); // Starting as copy-source to simplify onFrameRender()

    // Create an SRV/UAV descriptor heap. Need 2 entries - 1 SRV for the scene and 1 UAV for the output
    mpSrvUavHeap = mContext->createDescriptorHeap(mpDevice, 2, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

    // Create the UAV. Based on the root signature we created it should be the first entry
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    mpDevice->CreateUnorderedAccessView(mpOutputResource, nullptr, &uavDesc, mpSrvUavHeap->GetCPUDescriptorHandleForHeapStart());

    // Create the TLAS SRV right after the UAV. Note that we are using a different SRV desc here
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.RaytracingAccelerationStructure.Location = mpTopLevelAS->GetGPUVirtualAddress();
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = mpSrvUavHeap->GetCPUDescriptorHandleForHeapStart();
    srvHandle.ptr += mpDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    mpDevice->CreateShaderResourceView(nullptr, &srvDesc, srvHandle);
}

uint32_t CppDirectXRayTracing15::Application::beginFrame()
{
    // Bind the descriptor heaps
    ID3D12DescriptorHeap* heaps[] = { mpSrvUavHeap };
    mpCmdList->SetDescriptorHeaps(arraysize(heaps), heaps);
    return mpSwapChain->GetCurrentBackBufferIndex();
}

void CppDirectXRayTracing15::Application::endFrame(uint32_t rtvIndex)
{
    mContext->resourceBarrier(mpCmdList, mFrameObjects[rtvIndex].pSwapChainBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    mFenceValue = mContext->submitCommandList(mpCmdList, mpCmdQueue, mpFence, mFenceValue);
    mpSwapChain->Present(0, 0);

    // Prepare the command list for the next frame
    uint32_t bufferIndex = mpSwapChain->GetCurrentBackBufferIndex();

    // Make sure we have the new back-buffer is ready
    if (mFenceValue > mContext->kDefaultSwapChainBuffers)
    {
        mpFence->SetEventOnCompletion(mFenceValue - mContext->kDefaultSwapChainBuffers + 1, mFenceEvent);
        WaitForSingleObject(mFenceEvent, INFINITE);
    }

    mFrameObjects[bufferIndex].pCmdAllocator->Reset();
    mpCmdList->Reset(mFrameObjects[bufferIndex].pCmdAllocator, nullptr);
}


//////////////////////////////////////////////////////////////////////////
// Callbacks
//////////////////////////////////////////////////////////////////////////
void CppDirectXRayTracing15::Application::onLoad(HWND winHandle, uint32_t winWidth, uint32_t winHeight)
{
    InitDXR(winHandle, winWidth, winHeight); 
    CreateAccelerationStructures();
    CreateRtPipelineState();
    createShaderResources();
    CreateShaderTable();
}

void CppDirectXRayTracing15::Application::onFrameRender()
{
    uint32_t rtvIndex = beginFrame();

    // Let's raytrace
    mContext->resourceBarrier(mpCmdList, mpOutputResource, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    D3D12_DISPATCH_RAYS_DESC raytraceDesc = {};
    raytraceDesc.Width = mSwapChainSize.x;
    raytraceDesc.Height = mSwapChainSize.y;
    raytraceDesc.Depth = 1;

    // RayGen is the first entry in the shader-table
    raytraceDesc.RayGenerationShaderRecord.StartAddress = mpShaderTable->GetGPUVirtualAddress() + 0 * mShaderTableEntrySize;
    raytraceDesc.RayGenerationShaderRecord.SizeInBytes = mShaderTableEntrySize;

    // Miss is the second entry in the shader-table
    size_t missOffset = 1 * mShaderTableEntrySize;
    raytraceDesc.MissShaderTable.StartAddress = mpShaderTable->GetGPUVirtualAddress() + missOffset;
    raytraceDesc.MissShaderTable.StrideInBytes = mShaderTableEntrySize;
    raytraceDesc.MissShaderTable.SizeInBytes = mShaderTableEntrySize;   // Only a s single miss-entry

    // Hit is the third entry in the shader-table
    size_t hitOffset = 2 * mShaderTableEntrySize;
    raytraceDesc.HitGroupTable.StartAddress = mpShaderTable->GetGPUVirtualAddress() + hitOffset;
    raytraceDesc.HitGroupTable.StrideInBytes = mShaderTableEntrySize;
    raytraceDesc.HitGroupTable.SizeInBytes = mShaderTableEntrySize;

    // Bind the empty root signature
    mpCmdList->SetComputeRootSignature(mpEmptyRootSig);

    // Dispatch
    mpCmdList->SetPipelineState1(mpPipelineState.GetInterfacePtr());
    mpCmdList->DispatchRays(&raytraceDesc);

    // Copy the results to the back-buffer
    mContext->resourceBarrier(mpCmdList, mpOutputResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    mContext->resourceBarrier(mpCmdList, mFrameObjects[rtvIndex].pSwapChainBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);
    mpCmdList->CopyResource(mFrameObjects[rtvIndex].pSwapChainBuffer, mpOutputResource);

    endFrame(rtvIndex);
}

void CppDirectXRayTracing15::Application::onShutdown()
{
    // Wait for the command queue to finish execution
    mFenceValue++;
    mpCmdQueue->Signal(mpFence, mFenceValue);
    mpFence->SetEventOnCompletion(mFenceValue, mFenceEvent);
    WaitForSingleObject(mFenceEvent, INFINITE);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    Framework::run(CppDirectXRayTracing15::Application(), "Tutorial 15");
}
