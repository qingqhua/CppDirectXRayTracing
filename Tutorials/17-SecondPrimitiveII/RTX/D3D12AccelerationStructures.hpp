#pragma once
#include "Structs/AccelerationStructureBuffer.hpp"
#include "Structs/HeapData.hpp"
#include "../Primitives/Sphere.hpp" 
#include "../Primitives/Cube.hpp" 
#include "../Primitives/Quad.hpp" 

namespace CppDirectXRayTracing17II
{
	class D3D12AccelerationStructures
	{
	public:
		D3D12AccelerationStructures() 
		{
			mCube.Init(10.5f);
			mSphere.Init(2.1f,32);

			mSceneIndices = mSphere.GetIndices();
			mSceneVertices = mSphere.GetVertices();
		};

		~D3D12AccelerationStructures() = default;

		ID3D12ResourcePtr createBuffer(ID3D12Device5Ptr pDevice, uint64_t size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState, const D3D12_HEAP_PROPERTIES& heapProps);

		AccelerationStructureBuffers createCubeBottomLevelAS(ID3D12Device5Ptr pDevice, ID3D12GraphicsCommandList4Ptr pCmdList);

		AccelerationStructureBuffers createPrimitiveBottomLevelAS(ID3D12Device5Ptr pDevice, ID3D12GraphicsCommandList4Ptr pCmdList);

		AccelerationStructureBuffers createTopLevelAS(ID3D12Device5Ptr pDevice, ID3D12GraphicsCommandList4Ptr pCmdList, ID3D12ResourcePtr pBottomLevelAS[], uint64_t& tlasSize);

		int GetIndexCount();
		int GetVertexCount();

		ID3D12ResourcePtr GetSphereIndexBuffer();
		ID3D12ResourcePtr GetSphereVertexBuffer();

		static const int kDefaultNumDesc = 2;

	private:

		// Create cube vertex and index buffer
		ID3D12ResourcePtr CreateCubeVB(ID3D12Device5Ptr pDevice);
		ID3D12ResourcePtr CreateCubeIB(ID3D12Device5Ptr pDevice);

		// Create primitive vertex and index buffer
		ID3D12ResourcePtr CreateSphereVB(ID3D12Device5Ptr pDevice);
		ID3D12ResourcePtr CreateSphereIB(ID3D12Device5Ptr pDevice);


		AccelerationStructureBuffers createBottomLevelAS(ID3D12Device5Ptr pDevice, ID3D12GraphicsCommandList4Ptr pCmdList, ID3D12ResourcePtr vBuffer, ID3D12ResourcePtr iBuffer, int vertexCount, int indexCount);

		Primitives::Cube mCube;
		Primitives::Sphere mSphere;

		std::vector<Primitives::Vertex> mSceneVertices;
		std::vector<uint16_t> mSceneIndices;

		ID3D12ResourcePtr mCubeIndexBuffer;
		ID3D12ResourcePtr mCubeVertexBuffer;

		ID3D12ResourcePtr mSphereIndexBuffer;
		ID3D12ResourcePtr mSphereVertexBuffer;
	};

};
