#pragma once
#include "Structs/AccelerationStructureBuffer.hpp"
#include "Structs/HeapData.hpp"
#include "../Primitives/Sphere.hpp" 
#include "../Primitives/Cube.hpp" 
#include "../Primitives/Quad.hpp" 

namespace CppDirectXRayTracing18
{
	class D3D12AccelerationStructures
	{
	public:
		D3D12AccelerationStructures() = default;
		~D3D12AccelerationStructures() = default;


		void CreateScenePrimitives(ID3D12Device5Ptr pDevice);

		AccelerationStructureBuffers createBottomLevelAS(ID3D12Device5Ptr pDevice, ID3D12GraphicsCommandList4Ptr pCmdList, std::vector<ID3D12ResourcePtr> vBuffer, std::vector<ID3D12ResourcePtr> iBuffer, const std::vector<int> vertexCount, const std::vector<int> indexCount, int geometryCount);

		ID3D12ResourcePtr createBuffer(ID3D12Device5Ptr pDevice, uint64_t size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState, const D3D12_HEAP_PROPERTIES& heapProps);

		AccelerationStructureBuffers createTopLevelAS(ID3D12Device5Ptr pDevice, ID3D12GraphicsCommandList4Ptr pCmdList, ID3D12ResourcePtr pBottomLevelAS[], uint64_t& tlasSize);

		std::vector<int> GetIndexCount();
		std::vector<int> GetVertexCount();

		std::vector <ID3D12ResourcePtr> GetIndexBuffer();
		std::vector <ID3D12ResourcePtr> GetVertexBuffer();

	private:
		// Create cube vertex and index buffer
		ID3D12ResourcePtr createCubeVB(ID3D12Device5Ptr pDevice);
		ID3D12ResourcePtr createCubeIB(ID3D12Device5Ptr pDevice);

		// Create primitive vertex and index buffer
		ID3D12ResourcePtr CreateSphereVB(ID3D12Device5Ptr pDevice);
		ID3D12ResourcePtr CreateSphereIB(ID3D12Device5Ptr pDevice);

		Primitives::Cube mCube;
		Primitives::Sphere mSphere;

		std::vector<ID3D12ResourcePtr> mIndexBuffer;
		std::vector<ID3D12ResourcePtr> mVertexBuffer;

		std::vector<int> mIndexCount;
		std::vector<int> mVertexCount;
	};

};
