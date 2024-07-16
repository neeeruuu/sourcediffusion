#pragma once

#include "util/callback.h"

class CViewSetup;
class IDirect3DDevice9;
struct _D3DPRESENT_PARAMETERS_;

inline auto g_D3DPresentCallback = new CallbackEvent<IDirect3DDevice9*>();
inline auto g_D3DResetCallback = new CallbackEvent<IDirect3DDevice9*, _D3DPRESENT_PARAMETERS_*>();

inline auto g_PostEnginePostProcessingCallback = new CallbackEvent();
inline auto g_PreRenderViewCallback = new CallbackEvent<CViewSetup&>();
