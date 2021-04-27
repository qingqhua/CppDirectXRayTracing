#pragma once
#include "Structs/AccelerationStructureBuffer.hpp"
#include "Structs/HeapData.hpp"
#include "../Primitives/Sphere.hpp" 

namespace CppDirectXRayTracing15
{
	class D3D12AccelerationStructures
	{
	public:
		D3D12AccelerationStructures() = default;
		~D3D12AccelerationStructures() = default;

		ID3D12ResourcePtr createBuffer(ID3D12Device5Ptr pDevice, uint64_t size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState, const D3D12_HEAP_PROPERTIES& heapProps);

		ID3D12ResourcePtr createTriangleVB(ID3D12Device5Ptr pDevice);

		void CreatePrimitiveVBIB(ID3D12Device5Ptr pDevice, ID3D12ResourcePtr& vb, size_t& vertexCount, ID3D12ResourcePtr& ib, size_t& indexCount);

		AccelerationStructureBuffers createBottomLevelAS(ID3D12Device5Ptr pDevice, ID3D12GraphicsCommandList4Ptr pCmdList, ID3D12ResourcePtr pVB, int vertexCount, ID3D12ResourcePtr pIB, int indexCount);

		// Used for triangle vb only
		//AccelerationStructureBuffers createBottomLevelAS(ID3D12Device5Ptr pDevice, ID3D12GraphicsCommandList4Ptr pCmdList, ID3D12ResourcePtr pVB);

		AccelerationStructureBuffers createTopLevelAS(ID3D12Device5Ptr pDevice, ID3D12GraphicsCommandList4Ptr pCmdList, ID3D12ResourcePtr pBottomLevelAS, uint64_t& tlasSize);
	};

};
