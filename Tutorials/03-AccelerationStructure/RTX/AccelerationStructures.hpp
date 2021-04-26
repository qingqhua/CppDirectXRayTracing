#pragma once
#include "Structs/AccelerationStructureBuffer.hpp"

namespace CppDirectXRayTracing03
{
	class D3D12GraphicsContext
	{
	public:
		D3D12GraphicsContext() = default;
		~D3D12GraphicsContext() = default;

		ID3D12ResourcePtr createBuffer(ID3D12Device5Ptr pDevice, uint64_t size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState, const D3D12_HEAP_PROPERTIES& heapProps);

		ID3D12ResourcePtr createTriangleVB(ID3D12Device5Ptr pDevice);

		AccelerationStructureBuffer createBottomLevelAS(ID3D12Device5Ptr pDevice, ID3D12GraphicsCommandList4Ptr pCmdList, ID3D12ResourcePtr pVB);


		AccelerationStructureBuffer createTopLevelAS(ID3D12Device5Ptr pDevice, ID3D12GraphicsCommandList4Ptr pCmdList, ID3D12ResourcePtr pBottomLevelAS, uint64_t& tlasSize);
	};

};
