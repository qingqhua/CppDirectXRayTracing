#pragma once
#include "Structs/AccelerationStructureBuffer.hpp"
#include "Structs/HeapData.hpp"
#include "../Primitives/Sphere.hpp" 
#include "../Primitives/Cube.hpp" 
#include "../Primitives/Quad.hpp" 

namespace CppDirectXRayTracing17
{
	class D3D12AccelerationStructures
	{
	public:
		D3D12AccelerationStructures() 
		{
			CreateScenePrimitives();
		};

		~D3D12AccelerationStructures() = default;

		ID3D12ResourcePtr createBuffer(ID3D12Device5Ptr pDevice, uint64_t size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState, const D3D12_HEAP_PROPERTIES& heapProps);

		void CreateSceneVBIB(ID3D12Device5Ptr pDevice);
 
		AccelerationStructureBuffers createBottomLevelAS(ID3D12Device5Ptr pDevice, ID3D12GraphicsCommandList4Ptr pCmdList, ID3D12ResourcePtr vBuffer, ID3D12ResourcePtr iBuffer, int vertexCount, int indexCount);

		AccelerationStructureBuffers createTopLevelAS(ID3D12Device5Ptr pDevice, ID3D12GraphicsCommandList4Ptr pCmdList, ID3D12ResourcePtr pBottomLevelAS[], uint64_t& tlasSize);

		int GetIndexCount();
		int GetVertexCount();

		ID3D12ResourcePtr GetIndexBuffer();
		ID3D12ResourcePtr GetVertexBuffer();

		static const int kDefaultNumDesc = 3;

	private:
		ID3D12ResourcePtr CreateSceneVB(ID3D12Device5Ptr pDevice);
		ID3D12ResourcePtr CreateSceneIB(ID3D12Device5Ptr pDevice);

		// Create cube vertex and index buffer
		ID3D12ResourcePtr createCubeVB(ID3D12Device5Ptr pDevice);
		ID3D12ResourcePtr createCubeIB(ID3D12Device5Ptr pDevice);

		// Create primitive vertex and index buffer
		ID3D12ResourcePtr CreateSphereVB(ID3D12Device5Ptr pDevice);
		ID3D12ResourcePtr CreateSphereIB(ID3D12Device5Ptr pDevice);

		void CreateScenePrimitives();

		Primitives::Cube mCube;
		Primitives::Sphere mSphere;

		std::vector<Primitives::Vertex> mSceneVertices;
		std::vector<uint16_t> mSceneIndices;

		ID3D12ResourcePtr mIndexBuffer;
		ID3D12ResourcePtr mVertexBuffer;
	};

};
