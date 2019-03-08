#include "stdafx.h"
#include "NeverRestart.h"

#include <windows.h>
#include <stdio.h>

#include "M2BaseHelpers.h"

LRESULT CALLBACK NeverRestartWindowProc(
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

HRESULT NeverRestartCreateBlockingWindow(
    _Out_ HWND* BlockingWindowHandle,
    _In_ LPCWSTR ReasonText)
{
  
    bool Succeed = false;

    do
    {
        // Step 1: Creating the Window.
        *BlockingWindowHandle = CreateWindowExW(
            0,
            L"Static",
            L"NeverRestart",
            WS_OVERLAPPED,
            0,
            0,
            0,
            0,
            nullptr,
            nullptr,
            nullptr,
            nullptr);
        if (!*BlockingWindowHandle)
            break;

        // Step 2: Setting the window procedure. 
        if (!SetWindowLongPtrW(
            *BlockingWindowHandle,
            GWL_WNDPROC,
            reinterpret_cast<LONG>(NeverRestartWindowProc)))
            break;

        // Step 3: We provide a reason for the shutdown prevention.
        if (!ShutdownBlockReasonCreate(
            *BlockingWindowHandle,
            ReasonText))
            break;

        // Step 4: We elevate the program to be asked as soon as possible 
        // to inhibit shutdown.
        if (!SetProcessShutdownParameters(
            0x4FF,
            SHUTDOWN_NORETRY))
            break;

        Succeed = true;

    } while (false);

    if (!Succeed)
    {
        DestroyWindow(*BlockingWindowHandle);
        *BlockingWindowHandle = nullptr;

        M2GetLastHRESULTErrorKnownFailedCall();
    }

    return S_OK;

}


int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd)
{
    HWND BlockingWindowHandle = nullptr;

    if (FAILED(NeverRestartCreateBlockingWindow(
        &BlockingWindowHandle,
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
