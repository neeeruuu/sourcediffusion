#pragma once

#include "util/callback.h"

#include <functional>

class CViewSetup;
class CViewRender;
class IDirect3DDevice9;
struct _D3DPRESENT_PARAMETERS_;

inline auto g_D3DPresentCallback = new CallbackEvent<IDirect3DDevice9*>();
inline auto g_D3DResetCallback = new CallbackEvent<IDirect3DDevice9*, _D3DPRESENT_PARAMETERS_*>();

inline auto g_ImGuiCallback = new CallbackEvent();

inline auto g_EnginePostProcessingCallback = new CallbackEvent();