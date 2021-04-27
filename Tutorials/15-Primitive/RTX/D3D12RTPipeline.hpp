#pragma once
#include "Structs/DxilLibrary.hpp"
#include "Structs/RootSignature.hpp"

namespace CppDirectXRayTracing15
{
	class D3D12RTPipeline
	{
	public:
		D3D12RTPipeline() = default;
		~D3D12RTPipeline() = default;

		ID3DBlobPtr compileLibrary(const WCHAR* filename, const WCHAR* targetString);
		RootSignatureDesc createRayGenRootDesc();
		DxilLibrary createDxilLibrary();

		const WCHAR* kShaderName = L"Data/15-Shaders.hlsl";
		const WCHAR* kRayGenShader = L"rayGen";
		const WCHAR* kMissShader = L"miss";
		const WCHAR* kClosestHitShader = L"chs";
		const WCHAR* kHitGroup = L"HitGroup";
		
	};

};
