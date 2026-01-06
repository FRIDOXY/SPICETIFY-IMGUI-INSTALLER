// blur.cpp - Version DirectX 9
#include "blur.hpp"
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>

namespace blur {
    LPDIRECT3DDEVICE9 device = nullptr;

    static LPDIRECT3DTEXTURE9 blurTexture = nullptr;
    static LPDIRECT3DSURFACE9 blurSurface = nullptr;

    static void CreateBlurTexture(int width, int height)
    {
        if (!device) return;

        // Nettoyer l'ancienne texture
        if (blurTexture) { blurTexture->Release(); blurTexture = nullptr; }
        if (blurSurface) { blurSurface->Release(); blurSurface = nullptr; }

        // Créer la nouvelle texture
        device->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &blurTexture, nullptr);
        
        if (blurTexture)
        {
            blurTexture->GetSurfaceLevel(0, &blurSurface);
        }
    }

    static void begin_blur(const ImDrawList* parent_list, const ImDrawCmd* cmd) {
        if (!device) return;
        
        // Récupérer le backbuffer actuel
        LPDIRECT3DSURFACE9 backBuffer = nullptr;
        device->GetRenderTarget(0, &backBuffer);
        
        if (backBuffer)
        {
            D3DSURFACE_DESC desc;
            backBuffer->GetDesc(&desc);
            
            // Créer la texture de blur si nécessaire
            if (!blurTexture)
            {
                CreateBlurTexture(desc.Width, desc.Height);
            }
            
            // Copier le backbuffer dans la texture de blur
            if (blurSurface)
            {
                device->StretchRect(backBuffer, nullptr, blurSurface, nullptr, D3DTEXF_LINEAR);
            }
            
            backBuffer->Release();
        }
    }

    static void end_blur(const ImDrawList* parent_list, const ImDrawCmd* cmd) {
        if (!device) return;
        // Restaurer les états de rendu
    }

    void create_blur(ImDrawList* drawList, ImVec2 pos, ImVec2 size) {
        if (!device) return;
        if (!drawList) return;

        drawList->AddCallback(begin_blur, nullptr);
        
        // Afficher la texture floutée
        if (blurTexture)
            drawList->AddImage((ImTextureID)blurTexture, pos, pos + size);
            
        drawList->AddCallback(end_blur, nullptr);
        drawList->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
    }

    void cleanup()
    {
        if (blurTexture) { blurTexture->Release(); blurTexture = nullptr; }
        if (blurSurface) { blurSurface->Release(); blurSurface = nullptr; }
    }
}

void draw_blur(ImDrawList* drawList) {
    if (!drawList) return;
    if (!blur::device) return;

    ImVec2 pos = drawList->GetClipRectMin();
    ImVec2 size = drawList->GetClipRectMax();
    blur::create_blur(drawList, pos, size - pos);
}
