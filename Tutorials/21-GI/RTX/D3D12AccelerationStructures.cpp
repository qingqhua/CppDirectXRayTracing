#pragma once
#include "D3D12AccelerationStructures.hpp"
#include <iostream>

ID3D12ResourcePtr CppDirectXRayTracing21::D3D12AccelerationStructures::createBuffer(ID3D12Device5Ptr pDevice, uint64_t size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState, const D3D12_HEAP_PROPERTIES& heapProps)
{
    D3D12_RESOURCE_DESC bufDesc = {};
    bufDesc.Alignment = 0;
    bufDesc.DepthOrArraySize = 1;
    bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufDesc.Flags = flags;
    bufDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufDesc.Height = 1;
    bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufDesc.MipLevels = 1;
    bufDesc.SampleDesc.Count = 1;
    bufDesc.SampleDesc.Quality = 0;
    bufDesc.Width = size;

    ID3D12ResourcePtr pBuffer;
    d3d_call(pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, initState, nullptr, IID_PPV_ARGS(&pBuffer)));
    return pBuffer;
}

CppDirectXRayTracing21::AccelerationStructureBuffers CppDirectXRayTracing21::D3D12AccelerationStructures::createPrimitiveBottomLevelAS(ID3D12Device5Ptr pDevice, ID3D12GraphicsCommandList4Ptr pCmdList)
{
    ID3D12ResourcePtr vb = CreateSphereVB(pDevice);
    ID3D12ResourcePtr ib = CreateSphereIB(pDevice);

    mSphereVertexBuffer = vb;
    mSphereIndexBuffer = ib;
    
    int vertexCount = static_cast<int>(mSphere.GetVertices().size());
    int indexCount = static_cast<int>(mSphere.GetIndices().size());
    return createBottomLevelAS(pDevice, pCmdList,vb,ib, vertexCount, indexCount);
}

CppDirectXRayTracing21::AccelerationStructureBuffers CppDirectXRayTracing21::D3D12AccelerationStructures::createCubeBottomLevelAS(ID3D12Device5Ptr pDevice, ID3D12GraphicsCommandList4Ptr pCmdList)
{
    auto& primitive = mQuad;
    auto& vb = CreateCubeVB(pDevice);
    auto& ib = CreateCubeIB(pDevice);

    mQuadVertexBuffer = vb;
    mQuadIndexBuffer = ib;
    
    int vertexCount = static_cast<int>(primitive.GetVertices().size());
    int indexCount = static_cast<int>(primitive.GetIndices().size());
    return createBottomLevelAS(pDevice, pCmdList, vb, ib, vertexCount, indexCount);
}

ID3D12ResourcePtr CppDirectXRayTracing21::D3D12AccelerationStructures::CreateCubeVB(ID3D12Device5Ptr pDevice)
{
    // Vertex buffer
    auto& primitive = mQuad;
    int vertexCount = static_cast<int>(primitive.GetVertices().size());
    ID3D12ResourcePtr vBuffer = createBuffer(pDevice, sizeof(Primitives::Vertex) * vertexCount, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
    uint8_t* vData;
    vBuffer->Map(0, nullptr, (void**)&vData);
    memcpy(vData, &primitive.GetVertices().data()[0], sizeof(Primitives::Vertex) * vertexCount);
    vBuffer->Unmap(0, nullptr);

    return vBuffer;
}

ID3D12ResourcePtr CppDirectXRayTracing21::D3D12AccelerationStructures::CreateCubeIB(ID3D12Device5Ptr pDevice)
{
    auto& primitive = mQuad;
    int indexCount = static_cast<int>(primitive.GetIndices().size());

    ID3D12ResourcePtr iBuffer = createBuffer(pDevice, sizeof(uint16_t) * indexCount, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
    uint8_t* iData;
    iBuffer->Map(0, nullptr, (void**)&iData);
    memcpy(iData, &primitive.GetIndices().data()[0], sizeof(uint16_t) * indexCount);
    iBuffer->Unmap(0, nullptr);

    return iBuffer;
}

ID3D12ResourcePtr CppDirectXRayTracing21::D3D12AccelerationStructures::CreateSphereVB(ID3D12Device5Ptr pDevice)
{
    // Vertex buffer

    int vertexCount = static_cast<int>(mSphere.GetVertices().size());
    ID3D12ResourcePtr vBuffer = createBuffer(pDevice, sizeof(Primitives::Vertex) * vertexCount, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
    uint8_t* vData;
    vBuffer->Map(0, nullptr, (void**)&vData);
    memcpy(vData, &mSphere.GetVertices().data()[0], sizeof(Primitives::Vertex) * vertexCount);
    vBuffer->Unmap(0, nullptr);

    return vBuffer;
}

ID3D12ResourcePtr CppDirectXRayTracing21::D3D12AccelerationStructures::CreateSphereIB(ID3D12Device5Ptr pDevice)
{
    int indexCount = static_cast<int>(mSphere.GetIndices().size());

    ID3D12ResourcePtr iBuffer = createBuffer(pDevice, sizeof(uint16_t) * indexCount, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
    uint8_t* iData;
    iBuffer->Map(0, nullptr, (void**)&iData);
    memcpy(iData, &mSphere.GetIndices().data()[0], sizeof(uint16_t)* indexCount);
    iBuffer->Unmap(0, nullptr);

    return iBuffer;
}

CppDirectXRayTracing21::AccelerationStructureBuffers CppDirectXRayTracing21::D3D12AccelerationStructures::createBottomLevelAS(ID3D12Device5Ptr pDevice, ID3D12GraphicsCommandList4Ptr pCmdList, ID3D12ResourcePtr vBuffer, ID3D12ResourcePtr iBuffer, int vertexCount, int indexCount)
{
    D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
    geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geomDesc.Triangles.VertexBuffer.StartAddress = vBuffer->GetGPUVirtualAddress();
    geomDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Primitives::Vertex); // change to vec3 if use hand-made cube buffer
    geomDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
    geomDesc.Triangles.VertexCount = vertexCount;
    geomDesc.Triangles.IndexBuffer = iBuffer->GetGPUVirtualAddress();
    geomDesc.Triangles.IndexCount = indexCount;
    geomDesc.Triangles.IndexFormat = DXGI_FORMAT_R16_UINT;

    geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
    
    // Get the size requirements for the scratch and AS buffers
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    inputs.NumDescs = 1;
    inputs.pGeometryDescs = &geomDesc;
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
    pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

    // Create the buffers. They need to support UAV, and since we are going to immediately use them, we create them with an unordered-access state
    AccelerationStructureBuffers buffers;
    buffers.pScratch = createBuffer(pDevice, info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kDefaultHeapProps);
    buffers.pResult = createBuffer(pDevice, info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, kDefaultHeapProps);

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

CppDirectXRayTracing21::AccelerationStructureBuffers CppDirectXRayTracing21::D3D12AccelerationStructures::createTopLevelAS(ID3D12Device5Ptr pDevice, ID3D12GraphicsCommandList4Ptr pCmdList, ID3D12ResourcePtr pBottomLevelAS[], uint64_t& tlasSize)
{
    // First, get the size of the TLAS buffers and create them
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    inputs.NumDescs = kInstancesNum;
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
    pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

    // Create the buffers
    AccelerationStructureBuffers buffers;
    buffers.pScratch = createBuffer(pDevice, info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kDefaultHeapProps);
    buffers.pResult = createBuffer(pDevice, info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, kDefaultHeapProps);
    tlasSize = info.ResultDataMaxSizeInBytes;

    // The instance desc should be inside a buffer, create and map the buffer
    buffers.pInstanceDesc = createBuffer(pDevice, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * kInstancesNum, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
    D3D12_RAYTRACING_INSTANCE_DESC* pInstanceDesc;
    buffers.pInstanceDesc->Map(0, nullptr, (void**)&pInstanceDesc);
    ZeroMemory(pInstanceDesc, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * kInstancesNum);

    mat4 transformation[kInstancesNum];
    transformation[0] = glm::translate(glm::mat4(1.0), glm::vec3(0.0f, -0.5f, 0.0f)); // Identity
    transformation[1] = translate(mat4(), vec3(0, 0.0, 0));
    transformation[2] = translate(mat4(), vec3(0.7, 0.0, -3));
    transformation[3] = translate(mat4(), vec3(2, 0.0, -3));

    // Plane
    {
        pInstanceDesc[0].InstanceID = 0;                            // This value will be exposed to the shader via InstanceID()
        pInstanceDesc[0].InstanceContributionToHitGroupIndex = 0;   // This is the offset inside the shader-table. We only have a single geometry, so the offset 0
        pInstanceDesc[0].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        mat4 m = transpose(transformation[0]);
        memcpy(pInstanceDesc[0].Transform, &m, sizeof(pInstanceDesc[0].Transform));
        pInstanceDesc[0].AccelerationStructure = pBottomLevelAS[0]->GetGPUVirtualAddress();
        pInstanceDesc[0].InstanceMask = 0xFF;
    }
    // Initialize the instance desc.
    for (int i = 1; i < kInstancesNum; i++)
    {
        pInstanceDesc[i].InstanceID = i;                            // This value will be exposed to the shader via InstanceID()
        pInstanceDesc[i].InstanceContributionToHitGroupIndex = i;   // This is the offset inside the shader-table. Since we have unique constant-buffer for each instance, we need a different offset
        pInstanceDesc[i].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        mat4 m = transpose(transformation[i]);
        memcpy(pInstanceDesc[i].Transform, &m, sizeof(pInstanceDesc[i].Transform));
        pInstanceDesc[i].AccelerationStructure = pBottomLevelAS[1]->GetGPUVirtualAddress();
        pInstanceDesc[i].InstanceMask = 0xFF;
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

int CppDirectXRayTracing21::D3D12AccelerationStructures::GetVertexCount()
{
    return static_cast<int>(mSceneVertices.size());
}

int CppDirectXRayTracing21::D3D12AccelerationStructures::GetIndexCount()
{
    return static_cast<int>(mSceneIndices.size());
}


ID3D12ResourcePtr CppDirectXRayTracing21::D3D12AccelerationStructures::GetSphereIndexBuffer()
{
    return mSphereIndexBuffer;
}

ID3D12ResourcePtr CppDirectXRayTracing21::D3D12AccelerationStructures::GetSphereVertexBuffer()
{
    return mSphereVertexBuffer;
}
