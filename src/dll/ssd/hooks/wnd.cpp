#include "ssd/callbacks.h"
#include "ssd/globals.h"
#include "ssd/hooks.h"

#include <windef.h>

#include <WinUser.h>

#include "util/bind.h"
#include "util/log.h"

#include <chrono>
#include <thread>

#include <backends/imgui_impl_win32.h>
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LONG_PTR oWndProc;
LRESULT APIENTRY hWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (GetForegroundWindow() != g_hWnd)
        return CallWindowProcA(reinterpret_cast<WNDPROC>(oWndProc), hWnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
        case WM_KEYDOWN:
        case WM_KEYUP:
            BindController::instance()->onKey(wParam, uMsg == WM_KEYDOWN ? true : false);
            break;
        default:
            if (g_CaptureInput)
            {
                bool shouldCapture = false;
                switch (uMsg)
                {
                    case WM_CONTEXTMENU:
                    case WM_SETCURSOR:
                    case WM_NCHITTEST:
                    case WM_CHAR:
                    case WM_INPUT:
                        shouldCapture = true;
                        break;
                    default:
                        /*
                            capture mouse events
                        */
                        if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MBUTTONDBLCLK)
                            shouldCapture = true;
                        if (uMsg >= WM_MOUSEWHEEL && uMsg <= WM_MOUSEHWHEEL)
                            shouldCapture = true;
                }

                if (!shouldCapture)
                    return CallWindowProcA(reinterpret_cast<WNDPROC>(oWndProc), hWnd, uMsg, wParam, lParam);
            }
    }

    if (g_CaptureInput)
    {
        ImGui_ImplWin32_WndProcHandler(reinterpret_cast<struct HWND__*>(hWnd), uMsg, wParam, lParam);
        return true;
    }

    return CallWindowProcA(reinterpret_cast<WNDPROC>(oWndProc), hWnd, uMsg, wParam, lParam);
}

bool applyWndHooks()
{
    auto start = std::chrono::steady_clock::now();
    while (true)
    {
        g_hWnd = FindWindowA("Valve001", NULL);
        if (g_hWnd)
            break;

        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(30))
        {
            Log::error("timed out while awaiting for window");
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    oWndProc = SetWindowLongPtrA(reinterpret_cast<HWND>(g_hWnd), GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(hWndProc));

    return true;
}

bool removeWndHooks()
{
    if (g_hWnd && oWndProc)
        SetWindowLongPtrW(reinterpret_cast<HWND>(g_hWnd), GWLP_WNDPROC, oWndProc);
    return true;
}

HOOK_INFO(wnd, applyWndHooks, removeWndHooks);