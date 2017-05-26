#include "d3d_render.h"

#include "atlbase.h"
#include <fstream>

CD3DRender::CD3DRender(void)
{
    m_pD3D = NULL;
    m_pd3dDevice = NULL;
    m_pd3dSurface = NULL;
}


CD3DRender::~CD3DRender(void)
{
    Close();
}

int CD3DRender::InitD3D(HWND hWindow, int nWidth, int nHeight)
{
    int ret = 0;

    m_pD3D = Direct3DCreate9( D3D_SDK_VERSION);
    if( m_pD3D == NULL )
        return -1;

    D3DPRESENT_PARAMETERS d3dpp; 
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_A8B8G8R8;

    double    dbAspect    = (double)nWidth / nHeight;
    RECT    rtClient;

    GetClientRect(hWindow, &rtClient);
    m_recClient        = rtClient;
    if((rtClient.right - rtClient.left) > (rtClient.bottom - rtClient.top) * dbAspect)
    {
        //width lager than height,adjust the width
        int                    nValidW(static_cast<int>((rtClient.bottom - rtClient.top) * dbAspect));
        int                    nLost((rtClient.right - rtClient.left) - nValidW);
        m_recClient.left    += nLost / 2;
        m_recClient.right    = m_recClient.left + nValidW;
    }
    else
    {
        //height lager than width,adjust the height
        int                    nValidH(static_cast<int>((rtClient.right - rtClient.left) / dbAspect));
        int                    nLost((rtClient.bottom - rtClient.top) - nValidH);
        m_recClient.top    += nLost / 2;
        m_recClient.bottom    = m_recClient.top + nValidH;
    }

    HRESULT hr =m_pD3D->CreateDevice(D3DADAPTER_DEFAULT,
                                      D3DDEVTYPE_HAL,
                                      hWindow,
                                      D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE,
                                      &d3dpp,
                                      &m_pd3dDevice);
    if( FAILED(hr) )
        return -1;

    hr = m_pd3dDevice->CreateOffscreenPlainSurface(
        nWidth,nHeight,
        D3DFMT_X8B8G8R8,
        D3DPOOL_DEFAULT,
        &m_pd3dSurface,
        NULL);
    if( FAILED(hr)) {
        return -1;
    }

    m_numWidth = nWidth;
    m_numHeight = nHeight;
    return 0;
}

int CD3DRender::RenderFrame(unsigned char *data, unsigned int data_len)
{
    HRESULT hr = S_OK;

    int len = m_numWidth * m_numHeight;

    D3DLOCKED_RECT d3d_rect;
    if( FAILED(m_pd3dSurface->LockRect(&d3d_rect,NULL,D3DLOCK_DONOTWAIT)))
        return -1;

    const int w = m_numWidth,h = m_numHeight;
    BYTE * const p = (BYTE *)d3d_rect.pBits;
    const int stride = d3d_rect.Pitch;

    //memcpy(p, data, data_len);
    int i = 0;
    for(i = 0;i < h; i ++)
    {
        memcpy(p + i * stride, data + i * w, w);
    }

    //for (int i = 0; i < w * h * 4; i++) {
    //    memcpy();
    //}

/*
    int i = 0;
    for(i = 0;i < h;i ++)
    {
        memcpy(p + i * stride, data + i * w, w);
    }
    for(i = 0;i < h/2;i ++)
    {
        memcpy(p + stride * h + i * stride / 2, data + w * h + w * h / 4 + i * w / 2, w / 2);
    }
    for(i = 0;i < h/2;i ++)
    {
        memcpy(p + stride * h + stride * h / 4 + i * stride / 2, data + w * h + i * w / 2, w / 2);
    }
*/
    if( FAILED(m_pd3dSurface->UnlockRect()))
    {
        return -1;
    }

#if 0
    std::ofstream of("m1.yuv", std::ofstream::out | std::ofstream::binary);
    of.write((char *)p, len * 3 / 2);
    of.close();
#endif

    m_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0,0,0), 1.0f, 0 );
    m_pd3dDevice->BeginScene();
    IDirect3DSurface9 * pBackBuffer = NULL;
    
    m_pd3dDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
    m_pd3dDevice->StretchRect(m_pd3dSurface, NULL, pBackBuffer, &m_recClient, D3DTEXF_LINEAR);
    m_pd3dDevice->EndScene();
    m_pd3dDevice->Present( NULL, NULL, NULL, NULL );

    return 0;
}

void CD3DRender::Close()
{
    if (m_pD3D != NULL) {
        m_pD3D->Release();
        m_pD3D = NULL;
    }

    if (m_pd3dDevice != NULL) {
        m_pd3dDevice->Release();
        m_pd3dDevice = NULL;
    }

    if (m_pd3dSurface != NULL) {
        m_pd3dSurface->Release();
        m_pd3dSurface = NULL;
    }
}
