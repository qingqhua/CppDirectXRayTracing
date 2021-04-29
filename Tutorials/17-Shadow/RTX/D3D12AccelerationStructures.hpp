#pragma once
#include "Structs/AccelerationStructureBuffer.hpp"
#include "Structs/HeapData.hpp"
#include "../Primitives/Sphere.hpp" 
#include "../Primitives/Cube.hpp" 

namespace CppDirectXRayTracing17
{
	class D3D12AccelerationStructures
	{
	public:
		D3D12AccelerationStructures()
		{
			mSphere.Init(2.5f,32);
			mCube.Init(1.0f);
		};

		~D3D12AccelerationStructures() = default;

		ID3D12ResourcePtr createBuffer(ID3D12Device5Ptr pDevice, uint64_t size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState, const D3D12_HEAP_PROPERTIES& heapProps);

		// Create cube vertex and index buffer
		ID3D12ResourcePtr createCubeVB(ID3D12Device5Ptr pDevice);
		ID3D12ResourcePtr createCubeIB(ID3D12Device5Ptr pDevice);

		// Create primitive vertex and index buffer
		ID3D12ResourcePtr CreateSphereVB(ID3D12Device5Ptr pDevice);
		ID3D12ResourcePtr CreateSphereIB(ID3D12Device5Ptr pDevice);

		AccelerationStructureBuffers createBottomLevelAS(ID3D12Device5Ptr pDevice, ID3D12GraphicsCommandList4Ptr pCmdList, ID3D12ResourcePtr vBuffer, ID3D12ResourcePtr iBuffer, int vertexCount, int indexCount);

		AccelerationStructureBuffers createTopLevelAS(ID3D12Device5Ptr pDevice, ID3D12GraphicsCommandList4Ptr pCmdList, ID3D12ResourcePtr pBottomLevelAS[], uint64_t& tlasSize);

		std::vector<int> GetIndexCount();
		std::vector<int> GetVertexCount();

		std::vector<ID3D12ResourcePtr> GetIndexBuffer();
		std::vector<ID3D12ResourcePtr> GetVertexBuffer();

		static const int kDefaultNumDesc = 2;

	private:
		Primitives::Cube mCube;
		Primitives::Sphere mSphere;

		std::vector<int> mIndexCount;
		std::vector<int> mVertexCount;

		std::vector<ID3D12ResourcePtr> mIndexBuffer;
		std::vector<ID3D12ResourcePtr> mVertexBuffer;
	};

};
