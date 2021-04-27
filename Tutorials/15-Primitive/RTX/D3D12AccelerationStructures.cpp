#pragma once
#include "D3D12AccelerationStructures.hpp"
#include <iostream>


ID3D12ResourcePtr CppDirectXRayTracing15::D3D12AccelerationStructures::createBuffer(ID3D12Device5Ptr pDevice, uint64_t size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState, const D3D12_HEAP_PROPERTIES& heapProps)
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

ID3D12ResourcePtr CppDirectXRayTracing15::D3D12AccelerationStructures::createCubeVB(ID3D12Device5Ptr pDevice, int& vertexCount)
{
    vec3 vertices[] =
    {
        vec3(-1.000000, 1.000000, -1.000000),
        vec3(1.000000, 1.000000, -1.000000),
        vec3(-1.000000, -1.000000, -1.000000),

        vec3(1.000000, -1.000000, -1.000000),
        vec3(-1.000000, 1.000000, 1.0),
        vec3(0.999999, 1.000000, 1.000001),
        vec3(-1.000000, -1.000000, 1.000000),
        vec3(1.000000, -1.000000, 1.000000)
    };
    vertexCount = sizeof(vertices) / sizeof(vertices[0]);

    // For simplicity, we create the vertex buffer on the upload heap, but that's not required
    ID3D12ResourcePtr pBuffer = createBuffer(pDevice, sizeof(vertices), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
    uint8_t* pData;
    pBuffer->Map(0, nullptr, (void**)&pData);
    memcpy(pData, vertices, sizeof(vertices));
    pBuffer->Unmap(0, nullptr);
    return pBuffer;
}

ID3D12ResourcePtr CppDirectXRayTracing15::D3D12AccelerationStructures::createCubeIB(ID3D12Device5Ptr pDevice, int& indexCount)
{
    uint16 indices[] =
    {
    0, 1, 2,    // side 1
    2, 1, 3,
    4, 0, 6,    // side 2
    6, 0, 2,
    7, 5, 6,    // side 3
    6, 5, 4,
    3, 1, 7,    // side 4
    7, 1, 5,
    4, 5, 0,    // side 5
    0, 5, 1,
    3, 7, 2,    // side 6
    2, 7, 6
    };
    indexCount = sizeof(indices) / sizeof(indices[0]);

    ID3D12ResourcePtr pBuffer = createBuffer(pDevice, sizeof(uint16) * indexCount, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
    uint8_t* pData;
    pBuffer->Map(0, nullptr, (void**)&pData);
    memcpy(pData, indices, sizeof(uint16) * indexCount);
    pBuffer->Unmap(0, nullptr);
    return pBuffer;
}

ID3D12ResourcePtr CppDirectXRayTracing15::D3D12AccelerationStructures::CreatePrimitiveVB(ID3D12Device5Ptr pDevice, int& vertexCount)
{
    // Vertex buffer
    vertexCount = static_cast<int>(mPrimitive.GetVertices().size());

    ID3D12ResourcePtr vBuffer = createBuffer(pDevice, sizeof(Primitives::Vertex) * vertexCount, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
    uint8_t* vData;
    vBuffer->Map(0, nullptr, (void**)&vData);
    memcpy(vData, &mPrimitive.GetVertices().data()[0], sizeof(Primitives::Vertex)* vertexCount);
    vBuffer->Unmap(0, nullptr);

    return vBuffer;
}

ID3D12ResourcePtr CppDirectXRayTracing15::D3D12AccelerationStructures::CreatePrimitiveIB(ID3D12Device5Ptr pDevice, int& indexCount)
{
    indexCount = static_cast<int>(mPrimitive.GetIndices().size());

    ID3D12ResourcePtr iBuffer = createBuffer(pDevice, sizeof(uint16_t) * indexCount, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
    uint8_t* iData;
    iBuffer->Map(0, nullptr, (void**)&iData);
    memcpy(iData, &mPrimitive.GetIndices().data()[0], sizeof(uint16_t)* indexCount);
    iBuffer->Unmap(0, nullptr);

    return iBuffer;
}

CppDirectXRayTracing15::AccelerationStructureBuffers CppDirectXRayTracing15::D3D12AccelerationStructures::createBottomLevelAS(ID3D12Device5Ptr pDevice, ID3D12GraphicsCommandList4Ptr pCmdList, ID3D12ResourcePtr vBuffer, ID3D12ResourcePtr iBuffer, int vertexCount, int indexCount)
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

CppDirectXRayTracing15::AccelerationStructureBuffers CppDirectXRayTracing15::D3D12AccelerationStructures::createTopLevelAS(ID3D12Device5Ptr pDevice, ID3D12GraphicsCommandList4Ptr pCmdList, ID3D12ResourcePtr pBottomLevelAS, uint64_t& tlasSize)
{
    // First, get the size of the TLAS buffers and create them
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    inputs.NumDescs = 1;
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
    pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

    // Create the buffers
    AccelerationStructureBuffers buffers;
    buffers.pScratch = createBuffer(pDevice, info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kDefaultHeapProps);
    buffers.pResult = createBuffer(pDevice, info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, kDefaultHeapProps);
    tlasSize = info.ResultDataMaxSizeInBytes;

    // The instance desc should be inside a buffer, create and map the buffer
    buffers.pInstanceDesc = createBuffer(pDevice, sizeof(D3D12_RAYTRACING_INSTANCE_DESC), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
    D3D12_RAYTRACING_INSTANCE_DESC* pInstanceDesc;
    buffers.pInstanceDesc->Map(0, nullptr, (void**)&pInstanceDesc);

    // Initialize the instance desc. We only have a single instance
    pInstanceDesc->InstanceID = 0;                            // This value will be exposed to the shader via InstanceID()
    pInstanceDesc->InstanceContributionToHitGroupIndex = 0;   // This is the offset inside the shader-table. We only have a single geometry, so the offset 0
    pInstanceDesc->Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
    mat4 m; // Identity matrix
    memcpy(pInstanceDesc->Transform, &m, sizeof(pInstanceDesc->Transform));
    pInstanceDesc->AccelerationStructure = pBottomLevelAS->GetGPUVirtualAddress();
    pInstanceDesc->InstanceMask = 0xFF;

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
