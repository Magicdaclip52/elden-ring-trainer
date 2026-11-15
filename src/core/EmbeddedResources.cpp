#include "core/EmbeddedResources.h"

#include "resource.h"

#include <Windows.h>

namespace er {

RcBlob loadResource(int id) {
    RcBlob out;
    HMODULE mod = GetModuleHandleW(nullptr);
    HRSRC hRes = FindResourceW(mod, MAKEINTRESOURCEW(id), RT_RCDATA);
    if (!hRes) {
        return out;
    }
    HGLOBAL hGlob = LoadResource(mod, hRes);
    if (!hGlob) {
        return out;
    }
    const void* p = LockResource(hGlob);
    const DWORD size = SizeofResource(mod, hRes);
    if (!p || size == 0) {
        return out;
    }
    out.data.assign(static_cast<const u8*>(p), static_cast<const u8*>(p) + size);
    out.ok = true;
    return out;
}

}  // namespace er
