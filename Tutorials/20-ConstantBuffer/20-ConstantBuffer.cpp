#pragma once
#include "20-ConstantBuffer.hpp"

void CppDirectXRayTracing20::Application::InitDXR(HWND winHandle, uint32_t winWidth, uint32_t winHeight)
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

void CppDirectXRayTracing20::Application::CreateAccelerationStructures()
{
    AccelerationStructureBuffers blas[kDefaultNumDesc];

    blas[0] = mAccelerateStruct->createCubeBottomLevelAS(mpDevice, mpCmdList);
    blas[1] = mAccelerateStruct->createPrimitiveBottomLevelAS(mpDevice, mpCmdList);
   
    mBottomLevelAS[0] = blas[0].pResult;
    mBottomLevelAS[1] = blas[1].pResult;

    AccelerationStructureBuffers topLevelBuffers = mAccelerateStruct->createTopLevelAS(mpDevice, mpCmdList, mBottomLevelAS, mTlasSize);
    
    // The tutorial doesn't have any resource lifetime management, so we flush and sync here. This is not required by the DXR spec - you can submit the list whenever you like as long as you take care of the resources lifetime.
    mFenceValue = mContext->submitCommandList(mpCmdList, mpCmdQueue, mpFence, mFenceValue);
    mpFence->SetEventOnCompletion(mFenceValue, mFenceEvent);
    WaitForSingleObject(mFenceEvent, INFINITE);
    uint32_t bufferIndex = mpSwapChain->GetCurrentBackBufferIndex();
    mpCmdList->Reset(mFrameObjects[0].pCmdAllocator, nullptr);

    // Store the AS buffers. The rest of the buffers will be released once we exit the function
    mpTopLevelAS = topLevelBuffers.pResult;
}

void CppDirectXRayTracing20::Application::CreateRtPipelineState()
{
    // Need 10 subobjects:
    //  1 for the DXIL library
    //  1 for hit-group
    //  2 for RayGen root-signature (root-signature and the subobject association)
    //  New for tutorial 16: 2 for hit-program root signature and subobject association
    //                        2 for miss shader root-singature
    //  Deleted: 2 for the root-signature shared between miss and hit shaders (signature and association)
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

    // tutorial 16: 
    // Create the  hit root-signature and association
    LocalRootSignature hitRootSignature(mpDevice, mRtpipe->createHitRootDesc().desc);
    subobjects[index] = hitRootSignature.subobject;

    int hitRootIndex = index++; // 4
    ExportAssociation hitRootAssociation(&mRtpipe->kClosestHitShader, 1, &(subobjects[hitRootIndex]));// 5 Associate Hit Root Sig to Hit Group
    subobjects[index++] = hitRootAssociation.subobject; // 6 Associate Hit Root Sig to Hit Group

    // Create the miss root-signature and association
    //D3D12_ROOT_SIGNATURE_DESC emptyDesc = {};
    //emptyDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
    LocalRootSignature missRootSignature(mpDevice, mRtpipe->CreateMissRootDesc().desc);
    subobjects[index] = missRootSignature.subobject; // 6 Miss Root Sig

    int missRootIndex = index++;  // 6
    const WCHAR* missRootExport[] = { mRtpipe->kMissShader, mRtpipe->kShadowMiss };
    ExportAssociation missRootAssociation(missRootExport, arraysize(missRootExport), &(subobjects[missRootIndex]));
    //ExportAssociation missRootAssociation(&mRtpipe->kMissShader, 1, &(subobjects[missRootIndex]));
    subobjects[index++] = missRootAssociation.subobject; // 7 Associate Miss Root Sig to Miss Shader

    // Bind the payload size to the programs
    ShaderConfig shaderConfig(sizeof(float) * 2, sizeof(float) * (4+1));
    subobjects[index] = shaderConfig.subobject; // 8 Shader Config

    uint32_t shaderConfigIndex = index++; // 8
    const WCHAR* shaderExports[] = { mRtpipe->kMissShader, mRtpipe->kClosestHitShader, mRtpipe->kRayGenShader,mRtpipe->kShadowMiss };
    ExportAssociation configAssociation(shaderExports, arraysize(shaderExports), &(subobjects[shaderConfigIndex]));
    subobjects[index++] = configAssociation.subobject; // 9 Associate Shader Config to Miss, CHS, RGS

    // Create the pipeline config
    PipelineConfig config(kMaxTraceRecursionDepth);
    subobjects[index++] = config.subobject; // 10

    // Create the global root signature and store the empty signature
    GlobalRootSignature root(mpDevice, {});
    mpEmptyRootSig = root.pRootSig;
    subobjects[index++] = root.subobject; // 11

    // Create the state
    D3D12_STATE_OBJECT_DESC desc;
    desc.NumSubobjects = index; 
    desc.pSubobjects = subobjects.data();
    desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

    d3d_call(mpDevice->CreateStateObject(&desc, IID_PPV_ARGS(&mpPipelineState)));
}

void CppDirectXRayTracing20::Application::CreateShaderTable()
{
    /** The shader-table layout is as follows:
        Entry 0 - Ray-gen program
        Entry 1 - Miss program
        Entry 2 - Hit program
        Entry 3 - Hit program
        Entry 4 - Hit program
        All entries in the shader-table must have the same size, so we will choose it base on the largest required entry.
        The ray-gen program requires the largest entry - sizeof(program identifier) + 8 bytes for a descriptor-table.
        The entry size must be aligned up to D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT
    */

    // Calculate the size and create the buffer
    mShaderTableEntrySize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    mShaderTableEntrySize += 8; // The ray-gen's descriptor table
    mShaderTableEntrySize = align_to(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT, mShaderTableEntrySize);
    uint32_t shaderTableSize = mShaderTableEntrySize * (3+4);

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
    // Entry 2 - miss program
    memcpy(pData + mShaderTableEntrySize*2, pRtsoProps->GetShaderIdentifier(mRtpipe->kShadowMiss), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    // Entry 3-5 - hit program
    //uint8_t* pHitEntry = pData + mShaderTableEntrySize * 3; // +3 skips the ray-gen and 2 miss entries
    //memcpy(pHitEntry, pRtsoProps->GetShaderIdentifier(mRtpipe->kHitGroup), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
   // heapStart = mpSrvUavHeap->GetGPUDescriptorHandleForHeapStart().ptr;
    //*(uint64_t*)(pHitEntry + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = heapStart;
    /*
    for (uint32_t i = 0; i < kInstancesNum; i++)
    {
        uint8_t* pHitEntry = pData + mShaderTableEntrySize * (i + 3); 
        memcpy(pHitEntry, pRtsoProps->GetShaderIdentifier(mRtpipe->kHitGroup), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

         heapStart = mpSrvUavHeap->GetGPUDescriptorHandleForHeapStart().ptr;
        *(uint64_t*)(pHitEntry + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = heapStart;

        * (uint64_t*)(pHitEntry + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES*2) = mPrimitiveCB[i]->GetGPUVirtualAddress();
    }
    */
    pData += mShaderTableEntrySize * 2;
    for (uint32_t i = 0; i < kDefaultNumDesc; i++)
    {
        pData += mShaderTableEntrySize; 
        memcpy(pData, pRtsoProps->GetShaderIdentifier(mRtpipe->kHitGroup), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

        heapStart = mpSrvUavHeap->GetGPUDescriptorHandleForHeapStart().ptr;
        *(uint64_t*)(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = heapStart;

        uint8_t* pCbDesc = pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + sizeof(uint64_t);
        assert(((uint64_t)pCbDesc % 8) == 0); // Root descriptor must be stored at an 8-byte aligned address
        *(D3D12_GPU_VIRTUAL_ADDRESS*)pCbDesc = mPrimitiveCB[2]->GetGPUVirtualAddress();
    }

    // Unmap
    mpShaderTable->Unmap(0, nullptr);
}

void CppDirectXRayTracing20::Application::CreateGeometryBuffers(D3D12_CPU_DESCRIPTOR_HANDLE& srvHandle)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC indexsrvDesc = {};
    D3D12_SHADER_RESOURCE_VIEW_DESC vertexsrvDesc = {};

    indexsrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    indexsrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    indexsrvDesc.Format = DXGI_FORMAT_R32_TYPELESS; //todo: check r32 typeless
    indexsrvDesc.Buffer.NumElements = static_cast<int>(mAccelerateStruct->GetIndexCount() * 2 / 4);
    indexsrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
    indexsrvDesc.Buffer.StructureByteStride = 0;

    srvHandle.ptr += mpDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE indexsrvHandle = srvHandle;
    mpDevice->CreateShaderResourceView(mAccelerateStruct->GetSphereIndexBuffer(), &indexsrvDesc, indexsrvHandle);

    //Create vertex srv
    vertexsrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    vertexsrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    vertexsrvDesc.Format = DXGI_FORMAT_UNKNOWN;
    vertexsrvDesc.Buffer.NumElements = mAccelerateStruct->GetVertexCount();
    vertexsrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    vertexsrvDesc.Buffer.StructureByteStride = sizeof(Primitives::Vertex);

    srvHandle.ptr += mpDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE vertexsrvHandle = srvHandle;
    mpDevice->CreateShaderResourceView(mAccelerateStruct->GetSphereVertexBuffer(), &vertexsrvDesc, vertexsrvHandle);
}

void CppDirectXRayTracing20::Application::CreateSceneConstantBuffers(D3D12_CPU_DESCRIPTOR_HANDLE& srvHandle)
{
    SceneCB scenecb;
    {
        scenecb.projectionToWorld = glm::mat4(); //todo
        scenecb.cameraPosition = glm::vec3(0, 0, -7);
        scenecb.lightPosition = glm::vec3(2.0, 2.0, -2.0);
        scenecb.lightDiffuseColor = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
        scenecb.lightAmbientColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
        scenecb.backgroundColor = glm::vec4(0.2f, 0.21f, 0.9f, 1.0f);
        scenecb.MaxRecursionDepth = kMaxTraceRecursionDepth - 2;
    }

    mSceneCB = mAccelerateStruct->createBuffer(mpDevice, sizeof(SceneCB), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
    uint8_t* pData;
    d3d_call(mSceneCB->Map(0, nullptr, (void**)&pData));
    memcpy(pData, &scenecb, sizeof(scenecb));
    mSceneCB->Unmap(0, nullptr);

    D3D12_CONSTANT_BUFFER_VIEW_DESC scenecbsrvDesc = {};
    scenecbsrvDesc.BufferLocation = mSceneCB->GetGPUVirtualAddress();
    scenecbsrvDesc.SizeInBytes = (sizeof(SceneCB) + 255) & ~255;

    srvHandle.ptr += mpDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE scenecbsrvHandle = srvHandle;
    mpDevice->CreateConstantBufferView(&scenecbsrvDesc, scenecbsrvHandle);
}

void CppDirectXRayTracing20::Application::CreatePrimitiveConstantBuffers()
{
    // Primitive buffer per instance
    PrimitiveCB pcb[kInstancesNum];
    {
        pcb[0].diffuseColor = glm::vec4(0.9f, 0.9f, 0.9f, 1.0f);
        pcb[0].inShadowRadiance = 0.35f;
        pcb[0].diffuseCoef = 0.9f;
        pcb[0].specularCoef = 0.7f;
        pcb[0].specularPower = 50;
        pcb[0].reflectanceCoef = 0.7f;
    }

    {
        pcb[1].diffuseColor = glm::vec4(0.9f, 0.9f, 0.0f, 1.0f);
        pcb[1].inShadowRadiance = 0.35f;
        pcb[1].diffuseCoef = 0.9f;
        pcb[1].specularCoef = 0.7f;
        pcb[1].specularPower = 50;
        pcb[1].reflectanceCoef = 0.7f;
    }

    {
        pcb[2].diffuseColor = glm::vec4(0.0f, 0.9f, 0.9f, 1.0f);
        pcb[2].inShadowRadiance = 0.35f;
        pcb[2].diffuseCoef = 0.9f;
        pcb[2].specularCoef = 0.7f;
        pcb[2].specularPower = 50;
        pcb[2].reflectanceCoef = 0.7f;
    }

    {
        pcb[3].diffuseColor = glm::vec4(0.9f, 0.0f, 0.9f, 1.0f);
        pcb[3].inShadowRadiance = 0.35f;
        pcb[3].diffuseCoef = 0.9f;
        pcb[3].specularCoef = 0.7f;
        pcb[3].specularPower = 50;
        pcb[3].reflectanceCoef = 0.7f;
    }

    for (int i = 0; i < kInstancesNum; i++)
    {
        const uint32_t bufferSize = sizeof(PrimitiveCB);
        mPrimitiveCB[i] = mAccelerateStruct->createBuffer(mpDevice, bufferSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
        uint8_t* pData;
        d3d_call(mPrimitiveCB[i]->Map(0, nullptr, (void**)&pData));
        memcpy(pData, &pcb[i], sizeof(pcb));
        mPrimitiveCB[i]->Unmap(0, nullptr);
    }
}

void CppDirectXRayTracing20::Application::CreateShaderResources()
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

    // Create an SRV/UAV/VertexSRV/IndexSRV descriptor heap. 
    // Need 5 entries - 1 SRV for the scene, 1 UAV for the output, 1 SRV for VertexBuffer, 1 SRV for IndexBuffer, 1 constant buffer
    mpSrvUavHeap = mContext->createDescriptorHeap(mpDevice, 5, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

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
   
    // Create index and vertex srv
    CreateGeometryBuffers(srvHandle);

    // Create primitive cb and scene cb
    CreateSceneConstantBuffers(srvHandle);
}

uint32_t CppDirectXRayTracing20::Application::beginFrame()
{
    // Bind the descriptor heaps
    ID3D12DescriptorHeap* heaps[] = { mpSrvUavHeap };
    mpCmdList->SetDescriptorHeaps(arraysize(heaps), heaps);
    return mpSwapChain->GetCurrentBackBufferIndex();
}

void CppDirectXRayTracing20::Application::endFrame(uint32_t rtvIndex)
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
void CppDirectXRayTracing20::Application::onLoad(HWND winHandle, uint32_t winWidth, uint32_t winHeight)
{
    InitDXR(winHandle, winWidth, winHeight); 

    // Create geometry bottom/top level structure.
    CreateAccelerationStructures();

    // Create pipeline
    CreateRtPipelineState();

    // Create shader buffers
    CreateShaderResources();

    CreatePrimitiveConstantBuffers();

    CreateShaderTable();
}

void CppDirectXRayTracing20::Application::onFrameRender()
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
    raytraceDesc.MissShaderTable.SizeInBytes = mShaderTableEntrySize * 2;   // Only a s single miss-entry

    // Hit is the third entry in the shader-table
    size_t hitOffset = 3 * mShaderTableEntrySize;
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

void CppDirectXRayTracing20::Application::onShutdown()
{
    // Wait for the command queue to finish execution
    mFenceValue++;
    mpCmdQueue->Signal(mpFence, mFenceValue);
    mpFence->SetEventOnCompletion(mFenceValue, mFenceEvent);
    WaitForSingleObject(mFenceEvent, INFINITE);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    Framework::run(CppDirectXRayTracing20::Application(), "Tutorial 17");
}
