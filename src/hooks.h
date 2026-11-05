#pragma once
#include "sdk.h"
#include <d3d9.h>

namespace Hooks {
    bool InitD3D9();
    bool InitCreateMove();
    void Shutdown();

    using EndScene_t   = HRESULT(__stdcall*)(IDirect3DDevice9*);
    using Reset_t      = HRESULT(__stdcall*)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
    using CreateMove_t = bool(__thiscall*)(void*, float, CUserCmd*);
    using WndProc_t    = LRESULT(CALLBACK*)(HWND, UINT, WPARAM, LPARAM);

    inline void**        g_D3DVtable = nullptr;
    inline IDirect3DDevice9* g_Device = nullptr;
    inline HWND          g_Hwnd      = nullptr;
    inline bool          g_ImgInit   = false;

    inline EndScene_t   oEndScene   = nullptr;
    inline Reset_t      oReset      = nullptr;
    inline CreateMove_t oCreateMove = nullptr;
    inline WndProc_t    oWndProc    = nullptr;
}