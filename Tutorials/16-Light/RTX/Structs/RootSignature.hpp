#pragma once
#include "Framework.h"

namespace CppDirectXRayTracing16
{
    struct RootSignatureDesc
    {
        D3D12_ROOT_SIGNATURE_DESC desc = {};
        std::vector<D3D12_DESCRIPTOR_RANGE> range;
        std::vector<D3D12_ROOT_PARAMETER> rootParams;
    };

    struct LocalRootSignature
    {
        LocalRootSignature(ID3D12Device5Ptr pDevice, const D3D12_ROOT_SIGNATURE_DESC& desc)
        {
            pRootSig = createRootSignature(pDevice, desc);
            pInterface = pRootSig.GetInterfacePtr();
            subobject.pDesc = &pInterface;
            subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
        }
        ID3D12RootSignaturePtr pRootSig;
        ID3D12RootSignature* pInterface = nullptr;
        D3D12_STATE_SUBOBJECT subobject = {};

        ID3D12RootSignaturePtr createRootSignature(ID3D12Device5Ptr pDevice, const D3D12_ROOT_SIGNATURE_DESC& desc)
        {
            ID3DBlobPtr pSigBlob;
            ID3DBlobPtr pErrorBlob;
            HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &pSigBlob, &pErrorBlob);
            if (FAILED(hr))
            {
                std::string msg = convertBlobToString(pErrorBlob.GetInterfacePtr());
                msgBox(msg);
                return nullptr;
            }
            ID3D12RootSignaturePtr pRootSig;
            d3d_call(pDevice->CreateRootSignature(0, pSigBlob->GetBufferPointer(), pSigBlob->GetBufferSize(), IID_PPV_ARGS(&pRootSig)));
            return pRootSig;
        }
    };

    struct GlobalRootSignature
    {
        GlobalRootSignature(ID3D12Device5Ptr pDevice, const D3D12_ROOT_SIGNATURE_DESC& desc)
        {
            pRootSig = createRootSignature(pDevice, desc);
            pInterface = pRootSig.GetInterfacePtr();
            subobject.pDesc = &pInterface;
            subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
        }
        ID3D12RootSignaturePtr pRootSig;
        ID3D12RootSignature* pInterface = nullptr;
        D3D12_STATE_SUBOBJECT subobject = {};

        ID3D12RootSignaturePtr createRootSignature(ID3D12Device5Ptr pDevice, const D3D12_ROOT_SIGNATURE_DESC& desc)
        {
            ID3DBlobPtr pSigBlob;
            ID3DBlobPtr pErrorBlob;
            HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &pSigBlob, &pErrorBlob);
            if (FAILED(hr))
            {
                std::string msg = convertBlobToString(pErrorBlob.GetInterfacePtr());
                msgBox(msg);
                return nullptr;
            }
            ID3D12RootSignaturePtr pRootSig;
            d3d_call(pDevice->CreateRootSignature(0, pSigBlob->GetBufferPointer(), pSigBlob->GetBufferSize(), IID_PPV_ARGS(&pRootSig)));
            return pRootSig;
        }
    };


}