#pragma once
#include "Structs/DxilLibrary.hpp"
#include "Structs/RootSignature.hpp"

namespace CppDirectXRayTracing16
{
	class D3D12RTPipeline
	{
	public:
		D3D12RTPipeline() = default;
		~D3D12RTPipeline() = default;

		ID3DBlobPtr compileLibrary(const WCHAR* filename, const WCHAR* targetString);
		RootSignatureDesc createRayGenRootDesc();
		RootSignatureDesc createHitRootDesc();
		DxilLibrary createDxilLibrary();

		const WCHAR* kShaderName = L"Data/Shaders.hlsl";
		const WCHAR* kRayGenShader = L"rayGen";
		const WCHAR* kMissShader = L"miss";
		const WCHAR* kClosestHitShader = L"chs";
		const WCHAR* kHitGroup = L"HitGroup";
		
	};

};
