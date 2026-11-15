#include "ui/TextureLoader.h"

#include <wincodec.h>

#include <cstring>

namespace er {

void GpuTexture::release() {
    if (srv) {
        srv->Release();
        srv = nullptr;
    }
}

TextureLoader::TextureLoader(ID3D11Device* device) : device_(device) {}

TextureLoader::~TextureLoader() {
    for (auto* s : owned_) {
        if (s) {
            s->Release();
        }
    }
    owned_.clear();
}

std::optional<GpuTexture> TextureLoader::createFromRgba(std::span<const u8> rgba, int w, int h) {
    if (!device_ || rgba.empty() || w <= 0 || h <= 0) {
        return std::nullopt;
    }
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = static_cast<UINT>(w);
    desc.Height = static_cast<UINT>(h);
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA sub{};
    sub.pSysMem = rgba.data();
    sub.SysMemPitch = static_cast<UINT>(w) * 4;

    ID3D11Texture2D* tex = nullptr;
    if (FAILED(device_->CreateTexture2D(&desc, &sub, &tex)) || !tex) {
        return std::nullopt;
    }

    ID3D11ShaderResourceView* srv = nullptr;
    const HRESULT hr = device_->CreateShaderResourceView(tex, nullptr, &srv);
    tex->Release();
    if (FAILED(hr) || !srv) {
        return std::nullopt;
    }
    owned_.push_back(srv);
    return GpuTexture{srv, w, h};
}

std::optional<GpuTexture> TextureLoader::loadFromMemory(std::span<const u8> data) {
    if (!device_ || data.empty()) {
        return std::nullopt;
    }

    IWICImagingFactory* factory = nullptr;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&factory)))) {
        return std::nullopt;
    }

    IWICStream* stream = nullptr;
    if (FAILED(factory->CreateStream(&stream))) {
        factory->Release();
        return std::nullopt;
    }
    if (FAILED(stream->InitializeFromMemory(const_cast<BYTE*>(data.data()), static_cast<DWORD>(data.size())))) {
        stream->Release();
        factory->Release();
        return std::nullopt;
    }

    IWICBitmapDecoder* decoder = nullptr;
    if (FAILED(factory->CreateDecoderFromStream(stream, nullptr, WICDecodeMetadataCacheOnLoad, &decoder))) {
        stream->Release();
        factory->Release();
        return std::nullopt;
    }

    IWICBitmapFrameDecode* frame = nullptr;
    if (FAILED(decoder->GetFrame(0, &frame))) {
        decoder->Release();
        stream->Release();
        factory->Release();
        return std::nullopt;
    }

    IWICFormatConverter* converter = nullptr;
    if (FAILED(factory->CreateFormatConverter(&converter))) {
        frame->Release();
        decoder->Release();
        stream->Release();
        factory->Release();
        return std::nullopt;
    }
    if (FAILED(converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0,
                                     WICBitmapPaletteTypeCustom))) {
        converter->Release();
        frame->Release();
        decoder->Release();
        stream->Release();
        factory->Release();
        return std::nullopt;
    }

    UINT w = 0, h = 0;
    converter->GetSize(&w, &h);
    std::vector<u8> rgba(static_cast<usize>(w) * static_cast<usize>(h) * 4);
    if (FAILED(converter->CopyPixels(nullptr, w * 4, static_cast<UINT>(rgba.size()), rgba.data()))) {
        converter->Release();
        frame->Release();
        decoder->Release();
        stream->Release();
        factory->Release();
        return std::nullopt;
    }

    converter->Release();
    frame->Release();
    decoder->Release();
    stream->Release();
    factory->Release();

    return createFromRgba(rgba, static_cast<int>(w), static_cast<int>(h));
}

std::optional<GpuTexture> TextureLoader::loadBmpFromBlob(std::span<const u8> blob) {
    for (usize i = 0; i + 54 < blob.size(); ++i) {
        if (blob[i] != 'B' || blob[i + 1] != 'M') {
            continue;
        }
        const i32 fileSize = *reinterpret_cast<const i32*>(&blob[i + 2]);
        const i32 dataOff = *reinterpret_cast<const i32*>(&blob[i + 10]);
        const i32 dibSize = *reinterpret_cast<const i32*>(&blob[i + 14]);
        if (dibSize < 40 || dataOff < 14 || fileSize < 1024 || static_cast<usize>(i + fileSize) > blob.size()) {
            continue;
        }
        const int w = *reinterpret_cast<const i32*>(&blob[i + 18]);
        int h = *reinterpret_cast<const i32*>(&blob[i + 22]);
        const u16 bpp = *reinterpret_cast<const u16*>(&blob[i + 28]);
        if (w <= 0 || w > 2048 || bpp != 24 && bpp != 32) {
            continue;
        }
        const bool flip = h > 0;
        const int ah = flip ? h : -h;
        if (ah < 32 || ah > 2048) {
            continue;
        }
        const usize rowStride = ((static_cast<usize>(w) * bpp + 31) / 32) * 4;
        const usize pixelBase = i + static_cast<usize>(dataOff);
        if (pixelBase + rowStride * static_cast<usize>(ah) > blob.size()) {
            continue;
        }
        std::vector<u8> rgba(static_cast<usize>(w) * static_cast<usize>(ah) * 4);
        for (int y = 0; y < ah; ++y) {
            const int srcY = flip ? (ah - 1 - y) : y;
            const u8* row = blob.data() + pixelBase + static_cast<usize>(srcY) * rowStride;
            u8* dst = rgba.data() + static_cast<usize>(y) * static_cast<usize>(w) * 4;
            for (int x = 0; x < w; ++x) {
                if (bpp == 24) {
                    dst[x * 4 + 0] = row[x * 3 + 2];
                    dst[x * 4 + 1] = row[x * 3 + 1];
                    dst[x * 4 + 2] = row[x * 3 + 0];
                    dst[x * 4 + 3] = 255;
                } else {
                    dst[x * 4 + 0] = row[x * 4 + 2];
                    dst[x * 4 + 1] = row[x * 4 + 1];
                    dst[x * 4 + 2] = row[x * 4 + 0];
                    dst[x * 4 + 3] = row[x * 4 + 3];
                }
            }
        }
        if (w > 64 && ah > 64) {
            return createFromRgba(rgba, w, ah);
        }
    }
    return std::nullopt;
}

}  // namespace er
