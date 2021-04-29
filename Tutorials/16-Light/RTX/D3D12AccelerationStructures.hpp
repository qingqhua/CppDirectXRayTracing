#pragma once
#include "Structs/AccelerationStructureBuffer.hpp"
#include "Structs/HeapData.hpp"
#include "../Primitives/Sphere.hpp" 
#include "../Primitives/Cube.hpp" 

namespace CppDirectXRayTracing16
{
	class D3D12AccelerationStructures
	{
	public:
		D3D12AccelerationStructures()
		{
			mPrimitive.Init(2.0f,64);
		};

		~D3D12AccelerationStructures() = default;

		ID3D12ResourcePtr createBuffer(ID3D12Device5Ptr pDevice, uint64_t size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState, const D3D12_HEAP_PROPERTIES& heapProps);

		// Create cube vertex and index buffer
		ID3D12ResourcePtr createCubeVB(ID3D12Device5Ptr pDevice, int& vertexCount);
		ID3D12ResourcePtr createCubeIB(ID3D12Device5Ptr pDevice, int& indexCount);

		// Create primitive vertex and index buffer
		ID3D12ResourcePtr CreatePrimitiveVB(ID3D12Device5Ptr pDevice, int& vertexCount);
		ID3D12ResourcePtr CreatePrimitiveIB(ID3D12Device5Ptr pDevice, int& indexCount);

		AccelerationStructureBuffers createBottomLevelAS(ID3D12Device5Ptr pDevice, ID3D12GraphicsCommandList4Ptr pCmdList, ID3D12ResourcePtr vBuffer, ID3D12ResourcePtr iBuffer, int vertexCount, int indexCount);

		AccelerationStructureBuffers createTopLevelAS(ID3D12Device5Ptr pDevice, ID3D12GraphicsCommandList4Ptr pCmdList, ID3D12ResourcePtr pBottomLevelAS, uint64_t& tlasSize);

		int GetIndexCount();
		int GetVertexCount();
	private:
		//Primitives::Cube mPrimitive;
		Primitives::Sphere mPrimitive;
	};

};
