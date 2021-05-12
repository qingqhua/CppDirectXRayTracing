#pragma once
#include "Structs/DxilLibrary.hpp"
#include "Structs/RootSignature.hpp"

namespace CppDirectXRayTracing18
{
	class D3D12RTPipeline
	{
	public:
		D3D12RTPipeline() = default;
		~D3D12RTPipeline() = default;

		ID3DBlobPtr compileLibrary(const WCHAR* filename, const WCHAR* targetString);
		RootSignatureDesc createRayGenRootDesc();
		RootSignatureDesc createTriangleHitRootDesc();
		RootSignatureDesc createPlaneHitRootDesc();

		DxilLibrary createDxilLibrary();

		const WCHAR* kShaderName = L"Data/Shaders-18.hlsl";//L"Data/Shaders.hlsl";

		const WCHAR* kRayGenShader = L"rayGen";
		const WCHAR* kMissShader = L"miss";
		const WCHAR* kTriangleChs = L"triangleChs";
		const WCHAR* kPlaneChs = L"planeChs";
		const WCHAR* kTriHitGroup = L"TriHitGroup";
		const WCHAR* kPlaneHitGroup = L"PlaneHitGroup";
		const WCHAR* kShadowChs = L"shadowChs";
		const WCHAR* kShadowMiss = L"shadowMiss";
		const WCHAR* kShadowHitGroup = L"ShadowHitGroup";

	};

};
