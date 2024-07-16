#pragma once

inline bool g_CaptureInput = false;
inline void* g_hWnd = nullptr;

inline void* g_DllMod = nullptr;
inline char g_DllPath[261];

inline class IDirect3DDevice9* g_D3DDev;