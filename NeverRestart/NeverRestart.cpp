#include "stdafx.h"
#include "NeverRestart.h"

#include <windows.h>
#include <stdio.h>

#include "M2BaseHelpers.h"

LRESULT CALLBACK M2BlockingShutdownWindowProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_QUERYENDSESSION:
        return FALSE; // FALSE should prevent reboot
        break;
    case WM_ENDSESSION:
        Sleep(5000); // Should never get here!
        break;
    case WM_DESTROY:
        ShutdownBlockReasonDestroy(hWnd);
        break;
    default:
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

HRESULT M2CreateBlockingShutdownWindow(
    _Out_ HWND* BlockingWindowHandle,
    _In_ LPCWSTR WindowName,
    _In_ LPCWSTR ReasonText)
{
    // Step 1: Creating the Window.
    *BlockingWindowHandle = CreateWindowExW(
        0,
        L"Static",
        WindowName,
        WS_OVERLAPPED,
        0,
        0,
        0,
        0,
        nullptr,
        nullptr,
        nullptr,
        nullptr);
    if (*BlockingWindowHandle)
    {
        // Step 2: Setting the window procedure. 
        if (SetWindowLongPtrW(
            *BlockingWindowHandle,
            GWL_WNDPROC,
            reinterpret_cast<LONG>(M2BlockingShutdownWindowProc)))
        {
            // Step 3: We provide a reason for the shutdown prevention.
            if (ShutdownBlockReasonCreate(
                *BlockingWindowHandle,
                ReasonText))
            {
                // Step 4: We elevate the program to be asked as soon as 
                // possible to inhibit shutdown.
                if (SetProcessShutdownParameters(
                    0x4FF,
                    SHUTDOWN_NORETRY))
                {
                    // Succeed.
                    return S_OK;
                }
            }
        }

        // Cleanup if failed
        DestroyWindow(*BlockingWindowHandle);
        *BlockingWindowHandle = nullptr;
    }

    // Failed return.
    return M2GetLastHRESULTErrorKnownFailedCall();
}


int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd)
{
    HWND BlockingWindowHandle = nullptr;

    if (FAILED(M2CreateBlockingShutdownWindow(
        &BlockingWindowHandle,
        L"NeverRestart",
        L"永不重启侦测到了一个你不希望的重启或关机。"/*L"NeverRestart detects an unwanted restart or shutdown."*/)))
    {
        MessageBoxW(
            nullptr,
            L"CNeverRestart::Enable() Failed!",
            L"Error!",
            MB_ICONEXCLAMATION | MB_OK);
    }

    Sleep(INFINITE);

    DestroyWindow(BlockingWindowHandle);

    return 0;
}
