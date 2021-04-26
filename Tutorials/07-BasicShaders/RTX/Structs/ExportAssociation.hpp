#pragma once
#include "Framework.h"

namespace CppDirectXRayTracing07
{
    // Binds a sub-object into shaders and hit-groups.
    struct ExportAssociation
    {
        ExportAssociation(const WCHAR* exportNames[], uint32_t exportCount, const D3D12_STATE_SUBOBJECT* pSubobjectToAssociate)
        {
            association.NumExports = exportCount;
            association.pExports = exportNames;
            association.pSubobjectToAssociate = pSubobjectToAssociate;

            subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
            subobject.pDesc = &association;
        }

        D3D12_STATE_SUBOBJECT subobject = {};
        D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION association = {};
    };
}