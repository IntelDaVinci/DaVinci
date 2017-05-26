#ifndef _D3D_RENDER_H_
#define _D3D_RENDER_H_

#include <d3d9.h>
#include <dxva2api.h>
#include <dxva.h>
#include <windef.h>
#include <windows.h>

enum {
    MFX_HANDLE_GFXS3DCONTROL = 0x100 /* A handle to the IGFXS3DControl instance */
}; //mfxHandleType

class CD3DRender
{
public:
    CD3DRender(void);
    ~CD3DRender(void);

    int InitD3D(HWND hWindow, int nWidth, int nHeight);
    int RenderFrame(unsigned char *data, unsigned int data_len);

private:
    void Close();

private:
    IDirect3D9 * m_pD3D;
    IDirect3DDevice9 * m_pd3dDevice;
    IDirect3DSurface9 * m_pd3dSurface;
    RECT                        m_recClient;
    int                         m_numWidth;
    int                         m_numHeight;
};

#endif
