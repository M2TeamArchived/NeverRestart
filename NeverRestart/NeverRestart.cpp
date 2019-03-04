#include "stdafx.h"
#include "NeverRestart.h"

#include <windows.h>
#include <stdio.h>

#include "M2BaseHelpers.h"

struct CWindowHandleDefiner
{
    static inline HWND GetInvalidValue()
    {
        return nullptr;
    }

    static inline void Close(HWND Object)
    {
        DestroyWindow(Object);
    }
};

typedef M2::CObject<HWND, CWindowHandleDefiner> CWindowHandle;

class CNeverRestart
{
private:
    static LRESULT CALLBACK WindowProc(
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

private:
    CWindowHandle m_WindowHandle;
    std::wstring m_ReasonText;

public:
    CNeverRestart(
        const std::wstring_view& ReasonText) : 
        m_ReasonText(ReasonText)
    {

    }

    ~CNeverRestart() = default;

    HRESULT Enable()
    {
        bool Succeed = false;

        do
        {
            //Step 1: Registering the Window Class.
            WNDCLASSEXW WindowClass = { 0 };
            WindowClass.cbSize = sizeof(WNDCLASSEXW);
            WindowClass.lpfnWndProc = CNeverRestart::WindowProc;
            WindowClass.lpszClassName = L"NeverRestartWindowClass";
            if (!RegisterClassExW(&WindowClass))
                break;

            // Step 2: Creating the Window.
            this->m_WindowHandle = CreateWindowExW(
                0,
                WindowClass.lpszClassName,
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
            if (!this->m_WindowHandle)
                break;

            // Step 3: We provide a reason for the shutdown prevention.
            if (!ShutdownBlockReasonCreate(
                this->m_WindowHandle,
                this->m_ReasonText.c_str()))
                break;

            // Step 4: We elevate the program to be asked as soon as possible 
            // to inhibit shutdown.
            if (!SetProcessShutdownParameters(
                0x4FF,
                SHUTDOWN_NORETRY))
                break;

            Succeed = true;

        } while (false);

        return Succeed ? S_OK : M2GetLastHRESULTErrorKnownFailedCall();
    }

    void Disable()
    {
        this->m_WindowHandle = nullptr;
    }
};


int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd)
{
    CNeverRestart Instance(
        L"NeverRestart detects an unwanted restart or shutdown.");

    if (FAILED(Instance.Enable()))
    {
        MessageBoxW(
            nullptr,
            L"CNeverRestart::Enable() Failed!",
            L"Error!",
            MB_ICONEXCLAMATION | MB_OK);
    }

    Sleep(INFINITE);

    return 0;
}
