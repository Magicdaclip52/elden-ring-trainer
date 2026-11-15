#pragma once

#include "core/Types.h"

#include <d3d11.h>

#include <optional>
#include <span>
#include <vector>

namespace er {

struct GpuTexture {
    ID3D11ShaderResourceView* srv = nullptr;
    int width = 0;
    int height = 0;

    void release();
};

class TextureLoader {
public:
    explicit TextureLoader(ID3D11Device* device);
    ~TextureLoader();

    TextureLoader(const TextureLoader&) = delete;
    TextureLoader& operator=(const TextureLoader&) = delete;

    std::optional<GpuTexture> loadFromMemory(std::span<const u8> data);
    std::optional<GpuTexture> loadBmpFromBlob(std::span<const u8> blob);

private:
    std::optional<GpuTexture> createFromRgba(std::span<const u8> rgba, int w, int h);

    ID3D11Device* device_ = nullptr;
    std::vector<ID3D11ShaderResourceView*> owned_;
};

}  // namespace er
