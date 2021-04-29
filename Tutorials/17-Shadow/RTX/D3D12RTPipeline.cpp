#pragma once
#include "D3D12RTPipeline.hpp"
#include <sstream>

#define DXC

#ifdef DXC
    static dxc::DxcDllSupport gDxcDllHelper;
    MAKE_SMART_COM_PTR(IDxcCompiler);
    MAKE_SMART_COM_PTR(IDxcLibrary);
    MAKE_SMART_COM_PTR(IDxcBlobEncoding);
    MAKE_SMART_COM_PTR(IDxcOperationResult);
#endif // DXC

ID3DBlobPtr CppDirectXRayTracing17::D3D12RTPipeline::compileLibrary(const WCHAR* filename, const WCHAR* targetString)
{
    // Initialize the helper
    d3d_call(gDxcDllHelper.Initialize());
    IDxcCompilerPtr pCompiler;
    IDxcLibraryPtr pLibrary;
    d3d_call(gDxcDllHelper.CreateInstance(CLSID_DxcCompiler, &pCompiler));
    d3d_call(gDxcDllHelper.CreateInstance(CLSID_DxcLibrary, &pLibrary));

    // Open and read the file
    std::ifstream shaderFile(filename);
    if (shaderFile.good() == false)
    {
        msgBox("Can't open file " + wstring_2_string(std::wstring(filename)));
        return nullptr;
    }
    std::stringstream strStream;
    strStream << shaderFile.rdbuf();
    std::string shader = strStream.str();

    // Create blob from the string
    IDxcBlobEncodingPtr pTextBlob;
    d3d_call(pLibrary->CreateBlobWithEncodingFromPinned((LPBYTE)shader.c_str(), (uint32_t)shader.size(), 0, &pTextBlob));

    // Compile
    IDxcOperationResultPtr pResult;
    d3d_call(pCompiler->Compile(pTextBlob, filename, L"", targetString, nullptr, 0, nullptr, 0, nullptr, &pResult));

    // Verify the result
    HRESULT resultCode;
    d3d_call(pResult->GetStatus(&resultCode));
    if (FAILED(resultCode))
    {
        IDxcBlobEncodingPtr pError;
        d3d_call(pResult->GetErrorBuffer(&pError));
        std::string log = convertBlobToString(pError.GetInterfacePtr());
        msgBox("Compiler error:\n" + log);
        return nullptr;
    }

    MAKE_SMART_COM_PTR(IDxcBlob);
    IDxcBlobPtr pBlob;
    d3d_call(pResult->GetResult(&pBlob));
    return pBlob;
}

CppDirectXRayTracing17::RootSignatureDesc CppDirectXRayTracing17::D3D12RTPipeline::createRayGenRootDesc()
{
    // Create the root-signature
    CppDirectXRayTracing17::RootSignatureDesc desc;
    desc.range.resize(2);
    // gOutput
    desc.range[0].BaseShaderRegister = 0;
    desc.range[0].NumDescriptors = 1;
    desc.range[0].RegisterSpace = 0;
    desc.range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    desc.range[0].OffsetInDescriptorsFromTableStart = 0;

    // gRtScene
    desc.range[1].BaseShaderRegister = 0;
    desc.range[1].NumDescriptors = 1;
    desc.range[1].RegisterSpace = 0;
    desc.range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    desc.range[1].OffsetInDescriptorsFromTableStart = 1;

    desc.rootParams.resize(1);
    desc.rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    desc.rootParams[0].DescriptorTable.NumDescriptorRanges = 2;
    desc.rootParams[0].DescriptorTable.pDescriptorRanges = desc.range.data();

    // Create the desc
    desc.desc.NumParameters = 1;
    desc.desc.pParameters = desc.rootParams.data();
    desc.desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

    return desc;
}

CppDirectXRayTracing17::RootSignatureDesc CppDirectXRayTracing17::D3D12RTPipeline::createHitRootDesc()
{
    // Create the root-signature
    CppDirectXRayTracing17::RootSignatureDesc desc;
    desc.range.resize(2);
    // indices
    desc.range[0].NumDescriptors = 1;
    desc.range[0].BaseShaderRegister = 1;
    desc.range[0].RegisterSpace = 0;
    desc.range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    desc.range[0].OffsetInDescriptorsFromTableStart = 2;

    // vertices
    desc.range[1].NumDescriptors = 1;
    desc.range[1].BaseShaderRegister = 2;
    desc.range[1].RegisterSpace = 0;
    desc.range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    desc.range[1].OffsetInDescriptorsFromTableStart = 3;

    desc.rootParams.resize(1);
    desc.rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    desc.rootParams[0].DescriptorTable.NumDescriptorRanges = 2;
    desc.rootParams[0].DescriptorTable.pDescriptorRanges = desc.range.data();

    // Create the desc
    desc.desc.NumParameters = 1;
    desc.desc.pParameters = desc.rootParams.data();
    desc.desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

    return desc;
}

CppDirectXRayTracing17::DxilLibrary CppDirectXRayTracing17::D3D12RTPipeline::createDxilLibrary()
{
    // Compile the shader
    ID3DBlobPtr pDxilLib = compileLibrary(kShaderName, L"lib_6_3");
    const WCHAR* entryPoints[] = { kRayGenShader, kMissShader, kClosestHitShader };
    return DxilLibrary(pDxilLib, entryPoints, arraysize(entryPoints));
}
