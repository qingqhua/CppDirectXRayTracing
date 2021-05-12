#pragma once
#include "D3D12AccelerationStructures.hpp"
#include <iostream>

ID3D12ResourcePtr CppDirectXRayTracing18::D3D12AccelerationStructures::createBuffer(ID3D12Device5Ptr pDevice, uint64_t size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState, const D3D12_HEAP_PROPERTIES& heapProps)
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

ID3D12ResourcePtr CppDirectXRayTracing18::D3D12AccelerationStructures::createCubeVB(ID3D12Device5Ptr pDevice)
{
    // Vertex buffer
    int vertexCount = static_cast<int>(mCube.GetVertices().size());

    ID3D12ResourcePtr vBuffer = createBuffer(pDevice, sizeof(Primitives::Vertex) * vertexCount, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
    uint8_t* vData;
    vBuffer->Map(0, nullptr, (void**)&vData);
    memcpy(vData, &mCube.GetVertices().data()[0], sizeof(Primitives::Vertex) * vertexCount);
    vBuffer->Unmap(0, nullptr);

    mVertexBuffer.push_back(vBuffer);
    mVertexCount.push_back(vertexCount);
    return vBuffer;
}

ID3D12ResourcePtr CppDirectXRayTracing18::D3D12AccelerationStructures::createCubeIB(ID3D12Device5Ptr pDevice)
{
    int indexCount = static_cast<int>(mCube.GetIndices().size());

    ID3D12ResourcePtr iBuffer = createBuffer(pDevice, sizeof(uint16_t) * indexCount, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
    uint8_t* iData;
    iBuffer->Map(0, nullptr, (void**)&iData);
    memcpy(iData, &mCube.GetIndices().data()[0], sizeof(uint16_t) * indexCount);
    iBuffer->Unmap(0, nullptr);

    mIndexBuffer.push_back(iBuffer);
    mIndexCount.push_back(indexCount);
    return iBuffer;
}

ID3D12ResourcePtr CppDirectXRayTracing18::D3D12AccelerationStructures::CreateSphereVB(ID3D12Device5Ptr pDevice)
{
    // Vertex buffer
    int vertexCount = static_cast<int>(mSphere.GetVertices().size());
    ID3D12ResourcePtr vBuffer = createBuffer(pDevice, sizeof(Primitives::Vertex) * vertexCount, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
    uint8_t* vData;
    vBuffer->Map(0, nullptr, (void**)&vData);
    memcpy(vData, &mSphere.GetVertices().data()[0], sizeof(Primitives::Vertex)* vertexCount);
    vBuffer->Unmap(0, nullptr);

    mVertexBuffer.push_back(vBuffer);
    mVertexCount.push_back(vertexCount);
    
    return vBuffer;
}

ID3D12ResourcePtr CppDirectXRayTracing18::D3D12AccelerationStructures::CreateSphereIB(ID3D12Device5Ptr pDevice)
{
    int indexCount = static_cast<int>(mSphere.GetIndices().size());

    ID3D12ResourcePtr iBuffer = createBuffer(pDevice, sizeof(uint16_t) * indexCount, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
    uint8_t* iData;
    iBuffer->Map(0, nullptr, (void**)&iData);
    memcpy(iData, &mSphere.GetIndices().data()[0], sizeof(uint16_t)* indexCount);
    iBuffer->Unmap(0, nullptr);

    mIndexBuffer.push_back(iBuffer);
    mIndexCount.push_back(indexCount);
    return iBuffer;
}

void CppDirectXRayTracing18::D3D12AccelerationStructures::CreateScenePrimitives(ID3D12Device5Ptr pDevice)
{
    mCube.Init(10.5f);
    mSphere.Init(1.0f, 32);

    // Transform the cube
    glm::mat4 t = glm::translate(glm::mat4(1.0), glm::vec3(-2.0f,-6.0f,-2.0f));
    mCube.Transform(t);

    CreateSphereVB(pDevice);
    CreateSphereIB(pDevice);

    createCubeVB(pDevice);
    createCubeIB(pDevice);
}

CppDirectXRayTracing18::AccelerationStructureBuffers CppDirectXRayTracing18::D3D12AccelerationStructures::createBottomLevelAS(ID3D12Device5Ptr pDevice, ID3D12GraphicsCommandList4Ptr pCmdList, std::vector<ID3D12ResourcePtr> vBuffer, std::vector<ID3D12ResourcePtr> iBuffer, const std::vector<int> vertexCount, const std::vector<int> indexCount, int geometryCount)
{
    std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geomDesc;
    geomDesc.resize(geometryCount);

    for (int i = 0; i < geometryCount; i++)
    {
        geomDesc[i].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
        geomDesc[i].Triangles.VertexBuffer.StartAddress = vBuffer[i]->GetGPUVirtualAddress();
        geomDesc[i].Triangles.VertexBuffer.StrideInBytes = sizeof(Primitives::Vertex);
        geomDesc[i].Triangles.VertexCount = vertexCount[i];
        geomDesc[i].Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;

        geomDesc[i].Triangles.IndexBuffer = iBuffer[i]->GetGPUVirtualAddress();
        geomDesc[i].Triangles.IndexCount = indexCount[i];
        geomDesc[i].Triangles.IndexFormat = DXGI_FORMAT_R16_UINT;
        geomDesc[i].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
    }

    // Get the size requirements for the scratch and AS buffers
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    inputs.NumDescs = geometryCount;
    inputs.pGeometryDescs = geomDesc.data();
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

CppDirectXRayTracing18::AccelerationStructureBuffers CppDirectXRayTracing18::D3D12AccelerationStructures::createTopLevelAS(ID3D12Device5Ptr pDevice, ID3D12GraphicsCommandList4Ptr pCmdList, ID3D12ResourcePtr pBottomLevelAS[], uint64_t& tlasSize)
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
    buffers.pScratch = createBuffer(pDevice, info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kDefaultHeapProps);
    buffers.pResult = createBuffer(pDevice, info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, kDefaultHeapProps);
    tlasSize = info.ResultDataMaxSizeInBytes;

    // The instance desc should be inside a buffer, create and map the buffer
    buffers.pInstanceDesc = createBuffer(pDevice, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * 3, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
    D3D12_RAYTRACING_INSTANCE_DESC* pInstanceDesc;
    buffers.pInstanceDesc->Map(0, nullptr, (void**)&pInstanceDesc);
    ZeroMemory(pInstanceDesc, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * 3);

    mat4 transformation[3];
    transformation[0] = glm::mat4(1.0); // Identity
    transformation[1] = translate(mat4(), vec3(-1.5, 0, 0));
    transformation[2] = translate(mat4(), vec3(1.5, 0, 0));

    // Create the desc for the primitives instance
    pInstanceDesc[0].InstanceID = 0;
    pInstanceDesc[0].InstanceContributionToHitGroupIndex = 0;
    pInstanceDesc[0].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
    mat4 m = transpose(transformation[0]);
    memcpy(pInstanceDesc[0].Transform, &m, sizeof(pInstanceDesc[0].Transform));
    pInstanceDesc[0].AccelerationStructure = pBottomLevelAS[0]->GetGPUVirtualAddress();
    pInstanceDesc[0].InstanceMask = 0xFF;

    // Initialize the instance desc.
    for (int i = 1; i < 3; i++)
    {
        pInstanceDesc[i].InstanceID = i;                            // This value will be exposed to the shader via InstanceID()
        pInstanceDesc[i].InstanceContributionToHitGroupIndex = (i * 2) + 2;  // The indices are relative to to the start of the hit-table entries specified in Raytrace(), so we need 4 and 6
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

std::vector<int> CppDirectXRayTracing18::D3D12AccelerationStructures::GetVertexCount()
{
    return mVertexCount;
}

std::vector<int> CppDirectXRayTracing18::D3D12AccelerationStructures::GetIndexCount()
{
    return mIndexCount;
}

std::vector<ID3D12ResourcePtr> CppDirectXRayTracing18::D3D12AccelerationStructures::GetIndexBuffer()
{
    return mIndexBuffer;
}

std::vector<ID3D12ResourcePtr> CppDirectXRayTracing18::D3D12AccelerationStructures::GetVertexBuffer()
{
    return mVertexBuffer;
}
