// libremote_screen_test.cpp : Defines the entry point for the console application.
//

#include "remote_screen.h"

#include <iostream>
#include <fstream>
#include <windowsx.h>
#include <Windows.h>

bool going_on = true;
bool g_one_shot = true;
bool g_ok = false;

void GeneralExceptionCallBack(
    RS_STATUS error_code
    )
{
    going_on = false;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hWnd, message, wParam, lParam);
}


HWND CreateRenderWindow(int width, int height, void *ctx)
{
    HWND ret_hwnd;
    WNDCLASS window;
    memset(&window, 0, sizeof(window));

    window.lpfnWndProc= (WNDPROC)WindowProc;
    window.hInstance= GetModuleHandle(NULL);;
    window.hCursor= LoadCursor(NULL, IDC_ARROW);
    window.lpszClassName= L"RemoteScreen";

    if (!RegisterClass(&window))
        return 0;

    ::RECT displayRegion = {0, 0, width, height};
    ret_hwnd = CreateWindowEx(NULL,
                              window.lpszClassName,
                              L"Remote screen render",
                              WS_POPUP|WS_BORDER|WS_MAXIMIZE,
                              displayRegion.left,
                              displayRegion.top,
                              displayRegion.right,
                              displayRegion.bottom,
                              NULL,
                              NULL,
                              GetModuleHandle(NULL),
                              NULL);

    if (!ret_hwnd)
        return ret_hwnd;

    ShowWindow(ret_hwnd, SW_SHOWDEFAULT);
    UpdateWindow(ret_hwnd);

#ifdef WIN64
    SetWindowLongPtr(m_Hwnd, GWLP_USERDATA, (LONG_PTR)ctx);
#else
    SetWindowLong(ret_hwnd, GWL_USERDATA, PtrToLong(ctx));
#endif
    return ret_hwnd;
}

void CaptureFrameBuffer(
    unsigned int   frame_width,
    unsigned int   frame_height
    )
{
    std::cout << "width: " << frame_width << std::endl;
    std::cout << "height: " << frame_height << std::endl;

    //if (g_one_shot) {
    //    g_one_shot = false;
    //    HWND ret_hwnd;
    //    ret_hwnd = CreateRenderWindow(frame_width, frame_height, NULL);
    //    if (!ret_hwnd)
    //        return;
    //    if (g_render.InitD3D(ret_hwnd, frame_width, frame_height))
    //        return;
    //    g_ok = true;
    //}
//
//    int frame_size = frame_width * frame_height * 32 / 8;
//    unsigned char *frame_buffer = new unsigned char[frame_size];
//    memset(frame_buffer, 0, frame_size);
//
//    GetFrameBuffer(frame_buffer, frame_size);
//
//#if 1
//    static unsigned long long i = 0;
//    i++;
//    if (i % 100 == 0) {
//        char tmp[512] = {0};
//        sprintf_s(tmp, "%d.hh", i);
//        std::ofstream of(tmp, std::ofstream::out | std::ofstream::binary | std::ofstream::app);
//        of.write((char *)(frame_buffer), frame_size);
//        of.close();
//    }
//#endif
//
//    //if (g_ok)
//    //    g_render.RenderFrame(frame_buffer, frame_size);
//
//    delete frame_buffer;
}

int main(int argc, char* argv[])
{
    if (argc < 3) {
        std::cout << "Pls specify server address and port" << std::endl;
        std::cout << "    libremote_screen_test <server ip> <server port>" << std::endl;
        return -1;
    }

    InitializeRSModule();
    ConfigLogLevel(LOG_ERRO);

    if (RS_SUCCESS != Start(argv[1], atoi(argv[2]), 0, false, CaptureFrameBuffer, NULL))
    {
        std::cout << "Cannot connect to remote target" << std::endl;
        UninitializeRSModule();
        return -1;
    }

    static int cnt = 0;
    while (true) {
        //cnt++;
        Sleep(10);
    }

    Stop();
    UninitializeRSModule();

    return 0;
}
