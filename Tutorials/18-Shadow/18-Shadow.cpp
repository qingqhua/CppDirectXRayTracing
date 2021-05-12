#pragma once
#include "18-Shadow.hpp"

#include <numeric>

void CppDirectXRayTracing18::Application::InitDXR(HWND winHandle, uint32_t winWidth, uint32_t winHeight)
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

// The original tutorial use triangles (3 instances) and one plane.
// Here we use the spheres to inplace triangles, the name is stay as in the original tutorial
void CppDirectXRayTracing18::Application::CreateAccelerationStructures()
{
    // Create vertex buffer and index buffer of the scene.
    mAccelerateStruct->CreateScenePrimitives(mpDevice);

    // The first bottom-level buffer is for the ground and the triangle
    int indexCountTriPlane = mAccelerateStruct->GetIndexCount()[0] + mAccelerateStruct->GetIndexCount()[1];
    int vertexCountTriPlane = mAccelerateStruct->GetVertexCount()[0] + mAccelerateStruct->GetVertexCount()[1];

    mBottomLevelBuffers[0] = mAccelerateStruct->createBottomLevelAS(mpDevice, mpCmdList, mAccelerateStruct->GetVertexBuffer(), mAccelerateStruct->GetIndexBuffer(), mAccelerateStruct->GetVertexCount(), mAccelerateStruct->GetIndexCount(), 2);
    mBottomLevelAS[0] = mBottomLevelBuffers[0].pResult;

    // The second bottom-level buffer is for the triangle only
    std::vector<ID3D12ResourcePtr> trivb; trivb.push_back(mAccelerateStruct->GetVertexBuffer()[0]);
    std::vector<ID3D12ResourcePtr> triib; triib.push_back(mAccelerateStruct->GetIndexBuffer()[0]);
    mBottomLevelBuffers[1] = mAccelerateStruct->createBottomLevelAS(mpDevice, mpCmdList, trivb, triib, mAccelerateStruct->GetVertexCount(), mAccelerateStruct->GetIndexCount(), 1);
    mBottomLevelAS[1] = mBottomLevelBuffers[1].pResult;

    AccelerationStructureBuffers topLevelBuffers = mAccelerateStruct->createTopLevelAS(mpDevice, mpCmdList, mBottomLevelAS, mTlasSize);
    
    // The tutorial doesn't have any resource lifetime management, so we flush and sync here. This is not required by the DXR spec - you can submit the list whenever you like as long as you take care of the resources lifetime.
    mFenceValue = mContext->submitCommandList(mpCmdList, mpCmdQueue, mpFence, mFenceValue);
    mpFence->SetEventOnCompletion(mFenceValue, mFenceEvent);
    WaitForSingleObject(mFenceEvent, INFINITE);
    uint32_t bufferIndex = mpSwapChain->GetCurrentBackBufferIndex();
    mpCmdList->Reset(mFrameObjects[0].pCmdAllocator, nullptr);

    // Store the AS buffers. The rest of the buffers will be released once we exit the function
    mTopLevelAS = topLevelBuffers.pResult;
    
}

void CppDirectXRayTracing18::Application::testCreateAccelerationStructures()
{
    mAccelerateStruct->CreateScenePrimitives(mpDevice);
    ID3D12ResourcePtr mpVertexBuffer[2];
    //std::vector<ID3D12ResourcePtr> mpVertexBuffer;
    mpVertexBuffer[0] = createTriangleVB(mpDevice);
    mpVertexBuffer[1] = createPlaneVB(mpDevice);
    AccelerationStructureBuffers bottomLevelBuffers[2];

    // The first bottom-level buffer is for the plane and the triangle
    const uint32_t vertexCount[] = { 3, 6 }; // Triangle has 3 vertices, plane has 6
    //std::vector<uint32_t> vertexCount;
    //vertexCount.p

    //bottomLevelBuffers[0] =  mAccelerateStruct->createBottomLevelAS(mpDevice, mpCmdList, mAccelerateStruct->GetVertexBuffer(), vertexCount, 2);
    mBottomLevelAS[0] = bottomLevelBuffers[0].pResult;

    // The second bottom-level buffer is for the triangle only
    bottomLevelBuffers[1] = createBottomLevelAS(mpDevice, mpCmdList, mpVertexBuffer, vertexCount, 1);
    mBottomLevelAS[1] = bottomLevelBuffers[1].pResult;

    // Create the TLAS
    AccelerationStructureBuffers topLevelBuffers = createTopLevelAS(mpDevice, mpCmdList, mBottomLevelAS, mTlasSize);

    // The tutorial doesn't have any resource lifetime management, so we flush and sync here. This is not required by the DXR spec - you can submit the list whenever you like as long as you take care of the resources lifetime.
    mFenceValue = mContext->submitCommandList(mpCmdList, mpCmdQueue, mpFence, mFenceValue);
    mpFence->SetEventOnCompletion(mFenceValue, mFenceEvent);
    WaitForSingleObject(mFenceEvent, INFINITE);
    uint32_t bufferIndex = mpSwapChain->GetCurrentBackBufferIndex();
    mpCmdList->Reset(mFrameObjects[0].pCmdAllocator, nullptr);

    // Store the AS buffers. The rest of the buffers will be released once we exit the function
    mTopLevelAS = topLevelBuffers.pResult;
}

CppDirectXRayTracing18::AccelerationStructureBuffers CppDirectXRayTracing18::Application::createTopLevelAS(ID3D12Device5Ptr pDevice, ID3D12GraphicsCommandList4Ptr pCmdList, ID3D12ResourcePtr pBottomLevelAS[2], uint64_t& tlasSize)
{
    // First, get the size of the TLAS buffers and create them
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    inputs.NumDescs = 3;
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
    pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

    // Create the buffers
    AccelerationStructureBuffers buffers;
    buffers.pScratch = mAccelerateStruct->createBuffer(pDevice, info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kDefaultHeapProps);
    buffers.pResult = mAccelerateStruct->createBuffer(pDevice, info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, kDefaultHeapProps);
    tlasSize = info.ResultDataMaxSizeInBytes;

    // The instance desc should be inside a buffer, create and map the buffer
    buffers.pInstanceDesc = mAccelerateStruct->createBuffer(pDevice, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * 3, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
    D3D12_RAYTRACING_INSTANCE_DESC* instanceDescs;
    buffers.pInstanceDesc->Map(0, nullptr, (void**)&instanceDescs);
    ZeroMemory(instanceDescs, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * 3);

    // The transformation matrices for the instances
    mat4 transformation[3];
    transformation[0] = mat4(); // Identity
    transformation[1] = translate(mat4(), vec3(-2, 0, 0));
    transformation[2] = translate(mat4(), vec3(2, 0, 0));

    // Create the desc for the triangle/plane instance
    instanceDescs[0].InstanceID = 0;
    instanceDescs[0].InstanceContributionToHitGroupIndex = 0;
    instanceDescs[0].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
    memcpy(instanceDescs[0].Transform, &transformation[0], sizeof(instanceDescs[0].Transform));
    instanceDescs[0].AccelerationStructure = pBottomLevelAS[0]->GetGPUVirtualAddress();
    instanceDescs[0].InstanceMask = 0xFF;

    for (uint32_t i = 1; i < 3; i++)
    {
        instanceDescs[i].InstanceID = i; // This value will be exposed to the shader via InstanceID()
        instanceDescs[i].InstanceContributionToHitGroupIndex = (i * 2) + 2;  // The indices are relative to to the start of the hit-table entries specified in Raytrace(), so we need 4 and 6
        instanceDescs[i].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        mat4 m = transpose(transformation[i]); // GLM is column major, the INSTANCE_DESC is row major
        memcpy(instanceDescs[i].Transform, &m, sizeof(instanceDescs[i].Transform));
        instanceDescs[i].AccelerationStructure = pBottomLevelAS[1]->GetGPUVirtualAddress();
        instanceDescs[i].InstanceMask = 0xFF;
    }

    // Unmap
    buffers.pInstanceDesc->Unmap(0, nullptr);

    // Create the TLAS
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
    asDesc.Inputs = inputs;
    asDesc.Inputs.InstanceDescs = buffers.pInstanceDesc->GetGPUVirtualAddress();
    asDesc.DestAccelerationStructureData = buffers.pResult->GetGPUVirtualAddress();
    asDesc.ScratchAccelerationStructureData = buffers.pScratch->GetGPUVirtualAddress();

    pCmdList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

    // We need to insert a UAV barrier before using the acceleration structures in a raytracing operation
    D3D12_RESOURCE_BARRIER uavBarrier = {};
    uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    uavBarrier.UAV.pResource = buffers.pResult;
    pCmdList->ResourceBarrier(1, &uavBarrier);

    return buffers;
}

CppDirectXRayTracing18::AccelerationStructureBuffers CppDirectXRayTracing18::Application::createBottomLevelAS(ID3D12Device5Ptr pDevice, ID3D12GraphicsCommandList4Ptr pCmdList, ID3D12ResourcePtr pVB[], const uint32_t vertexCount[], uint32_t geometryCount)
{
    std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geomDesc;
    geomDesc.resize(geometryCount);

    for (uint32_t i = 0; i < geometryCount; i++)
    {
        geomDesc[i].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
        geomDesc[i].Triangles.VertexBuffer.StartAddress = pVB[i]->GetGPUVirtualAddress();
        geomDesc[i].Triangles.VertexBuffer.StrideInBytes = sizeof(vec3);
        geomDesc[i].Triangles.VertexCount = vertexCount[i];
        geomDesc[i].Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
        geomDesc[i].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
    }

    // Get the size requirements for the scratch and AS buffers
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    inputs.NumDescs = geometryCount;
    inputs.pGeometryDescs = geomDesc.data();
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
    pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

    // Create the buffers. They need to support UAV, and since we are going to immediately use them, we create them with an unordered-access state
    AccelerationStructureBuffers buffers;
    buffers.pScratch = mAccelerateStruct->createBuffer(pDevice, info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, kDefaultHeapProps);
    buffers.pResult = mAccelerateStruct->createBuffer(pDevice, info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, kDefaultHeapProps);

    // Create the bottom-level AS
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
    asDesc.Inputs = inputs;
    asDesc.DestAccelerationStructureData = buffers.pResult->GetGPUVirtualAddress();
    asDesc.ScratchAccelerationStructureData = buffers.pScratch->GetGPUVirtualAddress();

    pCmdList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

    // We need to insert a UAV barrier before using the acceleration structures in a raytracing operation
    D3D12_RESOURCE_BARRIER uavBarrier = {};
    uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    uavBarrier.UAV.pResource = buffers.pResult;
    pCmdList->ResourceBarrier(1, &uavBarrier);

    return buffers;
}

ID3D12ResourcePtr CppDirectXRayTracing18::Application::createPlaneVB(ID3D12Device5Ptr pDevice)
{
    const vec3 vertices[] =
    {
        vec3(-100, -1,  -2),
        vec3(100, -1,  100),
        vec3(-100, -1,  100),

        vec3(-100, -1,  -2),
        vec3(100, -1,  -2),
        vec3(100, -1,  100),
    };

    // For simplicity, we create the vertex buffer on the upload heap, but that's not required
    ID3D12ResourcePtr pBuffer = mAccelerateStruct->createBuffer(pDevice, sizeof(vertices), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
    uint8_t* pData;
    pBuffer->Map(0, nullptr, (void**)&pData);
    memcpy(pData, vertices, sizeof(vertices));
    pBuffer->Unmap(0, nullptr);
    return pBuffer;
}

ID3D12ResourcePtr CppDirectXRayTracing18::Application::createTriangleVB(ID3D12Device5Ptr pDevice)
{
    const vec3 vertices[] =
    {
        vec3(0,          1,  0),
        vec3(0.866f,  -0.5f, 0),
        vec3(-0.866f, -0.5f, 0),
    };

    // For simplicity, we create the vertex buffer on the upload heap, but that's not required
    ID3D12ResourcePtr pBuffer = mAccelerateStruct->createBuffer(pDevice, sizeof(vertices), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
    uint8_t* pData;
    pBuffer->Map(0, nullptr, (void**)&pData);
    memcpy(pData, vertices, sizeof(vertices));
    pBuffer->Unmap(0, nullptr);
    return pBuffer;
}

void CppDirectXRayTracing18::Application::CreateRtPipelineState()
{
    // Need 16 subobjects:
     //  1 for DXIL library    
     //  3 for the hit-groups (triangle hit group, plane hit-group, shadow-hit group)
     //  2 for RayGen root-signature (root-signature and the subobject association)
     //  2 for triangle hit-program root-signature (root-signature and the subobject association)
     //  2 for the plane-hit root-signature (root-signature and the subobject association)
     //  2 for shadow-program and miss root-signature (root-signature and the subobject association)
     //  2 for shader config (shared between all programs. 1 for the config, 1 for association)
     //  1 for pipeline config
     //  1 for the global root signature
    std::array<D3D12_STATE_SUBOBJECT, 16> subobjects;
    uint32_t index = 0;

    // Create the DXIL library
    DxilLibrary dxilLib = mRtpipe->createDxilLibrary();
    subobjects[index++] = dxilLib.stateSubobject; // 0 Library

    // Create the triangle HitProgram
    HitProgram triHitProgram(nullptr,  mRtpipe->kTriangleChs, mRtpipe->kTriHitGroup);
    subobjects[index++] = triHitProgram.subObject; // 1 Triangle Hit Group

    // Create the plane HitProgram
    HitProgram planeHitProgram(nullptr, mRtpipe->kPlaneChs, mRtpipe->kPlaneHitGroup);
    subobjects[index++] = planeHitProgram.subObject; // 2 Plant Hit Group

    // Create the shadow-ray hit group
    HitProgram shadowHitProgram(nullptr, mRtpipe->kShadowChs, mRtpipe->kShadowHitGroup);
    subobjects[index++] = shadowHitProgram.subObject; // 3 Shadow Hit Group

    // Create the ray-gen root-signature and association
    LocalRootSignature rgsRootSignature(mpDevice, mRtpipe->createRayGenRootDesc().desc);
    subobjects[index] = rgsRootSignature.subobject; // 4 Ray Gen Root Sig

    uint32_t rgsRootIndex = index++; // 4
    ExportAssociation rgsRootAssociation(&mRtpipe->kRayGenShader, 1, &(subobjects[rgsRootIndex]));
    subobjects[index++] = rgsRootAssociation.subobject; // 5 Associate Root Sig to RGS

    // Create the tri hit root-signature and association
    LocalRootSignature triHitRootSignature(mpDevice, mRtpipe->createTriangleHitRootDesc().desc);
    subobjects[index] = triHitRootSignature.subobject; // 6 Triangle Hit Root Sig

    uint32_t triHitRootIndex = index++; // 6
    ExportAssociation triHitRootAssociation(&mRtpipe->kTriangleChs, 1, &(subobjects[triHitRootIndex]));
    subobjects[index++] = triHitRootAssociation.subobject; // 7 Associate Triangle Root Sig to Triangle Hit Group

    // Create the plane hit root-signature and association
    LocalRootSignature planeHitRootSignature(mpDevice, mRtpipe->createPlaneHitRootDesc().desc);
    subobjects[index] = planeHitRootSignature.subobject; // 8 Plane Hit Root Sig

    uint32_t planeHitRootIndex = index++; // 8
    ExportAssociation planeHitRootAssociation(&mRtpipe->kPlaneHitGroup, 1, &(subobjects[planeHitRootIndex]));
    subobjects[index++] = planeHitRootAssociation.subobject; // 9 Associate Plane Hit Root Sig to Plane Hit Group

    // Create the empty root-signature and associate it with the primary miss-shader and the shadow programs
    D3D12_ROOT_SIGNATURE_DESC emptyDesc = {};
    emptyDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
    LocalRootSignature emptyRootSignature(mpDevice, emptyDesc);
    subobjects[index] = emptyRootSignature.subobject; // 10 Empty Root Sig for Plane Hit Group and Miss

    uint32_t emptyRootIndex = index++; // 10
    const WCHAR* emptyRootExport[] = {mRtpipe->kMissShader, mRtpipe->kShadowChs, mRtpipe->kShadowMiss };
    ExportAssociation emptyRootAssociation(emptyRootExport, arraysize(emptyRootExport), &(subobjects[emptyRootIndex]));
    subobjects[index++] = emptyRootAssociation.subobject; // 11 Associate empty root sig to Plane Hit Group and Miss shader

    // Bind the payload size to all programs
    ShaderConfig primaryShaderConfig(sizeof(float) * 2, sizeof(float) * 4);
    subobjects[index] = primaryShaderConfig.subobject; // 12

    uint32_t primaryShaderConfigIndex = index++;
    const WCHAR* primaryShaderExports[] = { mRtpipe->kRayGenShader, mRtpipe->kMissShader, mRtpipe->kTriangleChs, mRtpipe->kPlaneChs, mRtpipe->kShadowMiss, mRtpipe->kShadowChs };
    ExportAssociation primaryConfigAssociation(primaryShaderExports, arraysize(primaryShaderExports), &(subobjects[primaryShaderConfigIndex]));
    subobjects[index++] = primaryConfigAssociation.subobject; // 13 Associate shader config to all programs

    // Create the pipeline config
    PipelineConfig config(2); // maxRecursionDepth - 1 TraceRay() from the ray-gen, 1 TraceRay() from the primary hit-shader
    subobjects[index++] = config.subobject; // 14

    // Create the global root signature and store the empty signature
    GlobalRootSignature root(mpDevice, {});
    mpEmptyRootSig = root.pRootSig;
    subobjects[index++] = root.subobject; // 15

    // Create the state
    D3D12_STATE_OBJECT_DESC desc;
    desc.NumSubobjects = index; // 16
    desc.pSubobjects = subobjects.data();
    desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

    d3d_call(mpDevice->CreateStateObject(&desc, IID_PPV_ARGS(&mpPipelineState)));
}

void CppDirectXRayTracing18::Application::CreateShaderTable()
{
    /** The shader-table layout is as follows:
        Entry 0 - Ray-gen program
        Entry 1 - Miss program for the primary ray
        Entry 2 - Miss program for the shadow ray
        Entries 3,4 - Hit programs for triangle 0 (primary followed by shadow)
        Entries 5,6 - Hit programs for the plane (primary followed by shadow)
        Entries 7,8 - Hit programs for triangle 1 (primary followed by shadow)
        Entries 9,10 - Hit programs for triangle 2 (primary followed by shadow)
        All entries in the shader-table must have the same size, so we will choose it base on the largest required entry.
        The triangle primary-ray hit program requires the largest entry - sizeof(program identifier) + 8 bytes for the constant-buffer root descriptor.
        The entry size must be aligned up to D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT
    */

    // Calculate the size and create the buffer
    mShaderTableEntrySize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    mShaderTableEntrySize += 8; // The hit shader constant-buffer descriptor
    mShaderTableEntrySize = align_to(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT, mShaderTableEntrySize);
    uint32_t shaderTableSize = mShaderTableEntrySize * 11;

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

    // Entry 1 - primary ray miss
    memcpy(pData + mShaderTableEntrySize, pRtsoProps->GetShaderIdentifier(mRtpipe->kMissShader), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    // Entry 2 - shadow-ray miss
    memcpy(pData + mShaderTableEntrySize * 2, pRtsoProps->GetShaderIdentifier(mRtpipe->kShadowMiss), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    // Entry 3 - Triangle 0, primary ray. ProgramID and constant-buffer data
    uint8_t* pEntry3 = pData + mShaderTableEntrySize * 3;
    memcpy(pEntry3, pRtsoProps->GetShaderIdentifier(mRtpipe->kTriHitGroup), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    assert(((uint64_t)(pEntry3 + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) % 8) == 0); // Root descriptor must be stored at an 8-byte aligned address
    //*(D3D12_GPU_VIRTUAL_ADDRESS*)(pEntry3 + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = mpConstantBuffer[0]->GetGPUVirtualAddress();

    // Entry 4 - Triangle 0, shadow ray. ProgramID only
    uint8_t* pEntry4 = pData + mShaderTableEntrySize * 4;
    memcpy(pEntry4, pRtsoProps->GetShaderIdentifier(mRtpipe->kShadowHitGroup), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    // Entry 5 - Plane, primary ray. ProgramID only and the TLAS SRV
    uint8_t* pEntry5 = pData + mShaderTableEntrySize * 5;
    memcpy(pEntry5, pRtsoProps->GetShaderIdentifier(mRtpipe->kPlaneHitGroup), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    *(uint64_t*)(pEntry5 + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = heapStart + mpDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); // The SRV comes directly after the program id

    // Entry 6 - Plane, shadow ray
    uint8_t* pEntry6 = pData + mShaderTableEntrySize * 6;
    memcpy(pEntry6, pRtsoProps->GetShaderIdentifier(mRtpipe->kShadowHitGroup), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    // Entry 7 - Triangle 1, primary ray. ProgramID and constant-buffer data
    uint8_t* pEntry7 = pData + mShaderTableEntrySize * 7;
    memcpy(pEntry7, pRtsoProps->GetShaderIdentifier(mRtpipe->kTriHitGroup), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    assert(((uint64_t)(pEntry7 + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) % 8) == 0); // Root descriptor must be stored at an 8-byte aligned address
    //*(D3D12_GPU_VIRTUAL_ADDRESS*)(pEntry7 + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = mpConstantBuffer[1]->GetGPUVirtualAddress();

    // Entry 8 - Triangle 1, shadow ray. ProgramID only
    uint8_t* pEntry8 = pData + mShaderTableEntrySize * 8;
    memcpy(pEntry8, pRtsoProps->GetShaderIdentifier(mRtpipe->kShadowHitGroup), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    // Entry 9 - Triangle 2, primary ray. ProgramID and constant-buffer data
    uint8_t* pEntry9 = pData + mShaderTableEntrySize * 9;
    memcpy(pEntry9, pRtsoProps->GetShaderIdentifier(mRtpipe->kTriHitGroup), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    assert(((uint64_t)(pEntry9 + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) % 8) == 0); // Root descriptor must be stored at an 8-byte aligned address
    //*(D3D12_GPU_VIRTUAL_ADDRESS*)(pEntry9 + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = mpConstantBuffer[2]->GetGPUVirtualAddress();

    // Entry 10 - Triangle 2, shadow ray. ProgramID only
    uint8_t* pEntry10 = pData + mShaderTableEntrySize * 10;
    memcpy(pEntry10, pRtsoProps->GetShaderIdentifier(mRtpipe->kShadowHitGroup), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    // Unmap
    mpShaderTable->Unmap(0, nullptr);
}

void CppDirectXRayTracing18::Application::createShaderResources()
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

    // tutorial 17
    // Create an SRV/UAV/VertexSRV/IndexSRV descriptor heap. Need 6 entries - 1 SRV for the scene, 1 UAV for the output, 2 SRV for VertexBuffer, 2 SRV for IndexBuffer
    mpSrvUavHeap = mContext->createDescriptorHeap(mpDevice, 4, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

    // Create the UAV. Based on the root signature we created it should be the first entry
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    mpDevice->CreateUnorderedAccessView(mpOutputResource, nullptr, &uavDesc, mpSrvUavHeap->GetCPUDescriptorHandleForHeapStart());

    // Create the TLAS SRV right after the UAV. Note that we are using a different SRV desc here
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.RaytracingAccelerationStructure.Location = mTopLevelAS->GetGPUVirtualAddress();
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = mpSrvUavHeap->GetCPUDescriptorHandleForHeapStart();
    srvHandle.ptr += mpDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    mpDevice->CreateShaderResourceView(nullptr, &srvDesc, srvHandle);
   
    // tutorial 16
    //Create index srv
    D3D12_SHADER_RESOURCE_VIEW_DESC indexsrvDesc = {};
    D3D12_SHADER_RESOURCE_VIEW_DESC vertexsrvDesc = {};

    indexsrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    indexsrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    indexsrvDesc.Format = DXGI_FORMAT_R32_TYPELESS; //todo: check r32 typeless
    indexsrvDesc.Buffer.NumElements = static_cast<int>(mAccelerateStruct->GetIndexCount()[0] * 2 / 4);
    indexsrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
    indexsrvDesc.Buffer.StructureByteStride = 0;

    srvHandle.ptr += mpDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE indexsrvHandle = srvHandle;
    mpDevice->CreateShaderResourceView(mAccelerateStruct->GetIndexBuffer()[0], &indexsrvDesc, indexsrvHandle);

    //Create vertex srv
    vertexsrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    vertexsrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    vertexsrvDesc.Format = DXGI_FORMAT_UNKNOWN;
    vertexsrvDesc.Buffer.NumElements = mAccelerateStruct->GetVertexCount()[0];
    vertexsrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    vertexsrvDesc.Buffer.StructureByteStride = sizeof(Primitives::Vertex);

    srvHandle.ptr += mpDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE vertexsrvHandle = srvHandle;
    mpDevice->CreateShaderResourceView(mAccelerateStruct->GetVertexBuffer()[0], &vertexsrvDesc, vertexsrvHandle);
}

uint32_t CppDirectXRayTracing18::Application::beginFrame()
{
    // Bind the descriptor heaps
    ID3D12DescriptorHeap* heaps[] = { mpSrvUavHeap };
    mpCmdList->SetDescriptorHeaps(arraysize(heaps), heaps);
    return mpSwapChain->GetCurrentBackBufferIndex();
}

void CppDirectXRayTracing18::Application::endFrame(uint32_t rtvIndex)
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
void CppDirectXRayTracing18::Application::onLoad(HWND winHandle, uint32_t winWidth, uint32_t winHeight)
{
    InitDXR(winHandle, winWidth, winHeight); 
    CreateAccelerationStructures();
    //testCreateAccelerationStructures();
    CreateRtPipelineState();
    createShaderResources();
    CreateShaderTable();
}

void CppDirectXRayTracing18::Application::onFrameRender()
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
    raytraceDesc.MissShaderTable.SizeInBytes = mShaderTableEntrySize * 2;   // 2 miss-entries

    // Hit is the third entry in the shader-table
    size_t hitOffset = 3 * mShaderTableEntrySize;
    raytraceDesc.HitGroupTable.StartAddress = mpShaderTable->GetGPUVirtualAddress() + hitOffset;
    raytraceDesc.HitGroupTable.StrideInBytes = mShaderTableEntrySize;
    raytraceDesc.HitGroupTable.SizeInBytes = mShaderTableEntrySize * 8;

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

void CppDirectXRayTracing18::Application::onShutdown()
{
    // Wait for the command queue to finish execution
    mFenceValue++;
    mpCmdQueue->Signal(mpFence, mFenceValue);
    mpFence->SetEventOnCompletion(mFenceValue, mFenceEvent);
    WaitForSingleObject(mFenceEvent, INFINITE);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    Framework::run(CppDirectXRayTracing18::Application(), "Tutorial 18");
}
