#include <memory>
#include <sstream>

#include <d3d11.h>
#include <DispatcherQueue.h>
#include <winrt/Windows.System.h>

#include "capture_interop.h"
#include "d3d_helpers.h"
#include "direct3d11_interop.h"
#include "simple_capture.h"
#include "window_enumeration.h"

// Direct3D11CaptureFramePool requires a DispatcherQueue
winrt::Windows::System::DispatcherQueueController CreateDispatcherQueueController()
{
    namespace abi = ABI::Windows::System;

    DispatcherQueueOptions options
    {
        sizeof(DispatcherQueueOptions),
        DQTYPE_THREAD_CURRENT,
        DQTAT_COM_STA
    };

    winrt::Windows::System::DispatcherQueueController controller{ nullptr };
    winrt::check_hresult(CreateDispatcherQueueController(options, reinterpret_cast<abi::IDispatcherQueueController**>(winrt::put_abi(controller))));
    return controller;
}

// really ugly global variable
std::unique_ptr<SimpleCapture> capture{ nullptr };

LRESULT CALLBACK WndProc(
    HWND   hwnd,
    UINT   msg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        capture.reset();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
        break;
    }

    return 0;
}

//
// Taken from: https://chromium.googlesource.com/chromium/src/sandbox/+/refs/heads/main/win/src/restricted_token_utils.cc
//

DWORD GetObjectSecurityDescriptor(
    HANDLE handle,
    SECURITY_INFORMATION security_info,
    std::vector<char>* security_desc_buffer,
    PSECURITY_DESCRIPTOR* security_desc)
{
    DWORD last_error = 0;
    DWORD length_needed = 0;
    ::GetKernelObjectSecurity(handle, security_info, nullptr, 0, &length_needed);

    last_error = ::GetLastError();
    if (last_error != ERROR_INSUFFICIENT_BUFFER)
        return last_error;

    security_desc_buffer->resize(length_needed);
    *security_desc =
        reinterpret_cast<PSECURITY_DESCRIPTOR>(security_desc_buffer->data());
    if (!::GetKernelObjectSecurity(handle, security_info, *security_desc,
        length_needed, &length_needed)) {
        return ::GetLastError();
    }

    return ERROR_SUCCESS;
}

DWORD HardenTokenIntegrityLevelPolicy(HANDLE token)
{
    std::vector<char> security_desc_buffer;
    PSECURITY_DESCRIPTOR security_desc = nullptr;
    DWORD last_error = GetObjectSecurityDescriptor(
        token, LABEL_SECURITY_INFORMATION, &security_desc_buffer, &security_desc);

    if (last_error != ERROR_SUCCESS)
        return last_error;

    PACL sacl = nullptr;
    BOOL sacl_present = false;
    BOOL sacl_defaulted = false;
    if (!::GetSecurityDescriptorSacl(security_desc, &sacl_present, &sacl,
        &sacl_defaulted)) {
        return ::GetLastError();
    }

    for (DWORD ace_index = 0; ace_index < sacl->AceCount; ++ace_index) {
        PSYSTEM_MANDATORY_LABEL_ACE ace;
        if (::GetAce(sacl, ace_index, reinterpret_cast<LPVOID*>(&ace)) &&
            ace->Header.AceType == SYSTEM_MANDATORY_LABEL_ACE_TYPE)
        {
            // We only need the NO_READ to reproduce this
            ace->Mask |= SYSTEM_MANDATORY_LABEL_NO_READ_UP /* | SYSTEM_MANDATORY_LABEL_NO_EXECUTE_UP*/;
            break;
        }
    }

    if (!::SetKernelObjectSecurity(token, LABEL_SECURITY_INFORMATION,
        security_desc))
        return ::GetLastError();

    return ERROR_SUCCESS;
}

DWORD HardenProcessIntegrityLevelPolicy() {
    HANDLE token_handle;
    if (!::OpenProcessToken(GetCurrentProcess(), READ_CONTROL | WRITE_OWNER,
        &token_handle))
        return ::GetLastError();

    const auto retval = HardenTokenIntegrityLevelPolicy(token_handle);
    CloseHandle(token_handle);

    return retval;
}

int CALLBACK WinMain(
    HINSTANCE instance,
    HINSTANCE previousInstance,
    LPSTR     cmdLine,
    int       cmdShow)
{
    // Harden the process integrity level before attempting a Windows graphics capture
    // This results in null GraphicsCaptureItem
    DWORD error = HardenProcessIntegrityLevelPolicy();
    if ((error != ERROR_SUCCESS) && (error != ERROR_ACCESS_DENIED)) {
        return false;
    }

    // Init COM
    winrt::init_apartment(winrt::apartment_type::single_threaded);

    // Create the window
    WNDCLASSEX wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = instance;
    wcex.hIcon = LoadIcon(instance, MAKEINTRESOURCE(IDI_APPLICATION));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"ScreenCaptureforHWND";
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
    WINRT_VERIFY(RegisterClassEx(&wcex));

    HWND hwnd = CreateWindow(
        L"ScreenCaptureforHWND",
        L"ScreenCaptureforHWND",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        800,
        600,
        NULL,
        NULL,
        instance,
        NULL);
    WINRT_VERIFY(hwnd);

    ShowWindow(hwnd, cmdShow);
    UpdateWindow(hwnd);

    // Create a DispatcherQueue for our thread
    auto controller = CreateDispatcherQueueController();
    // Enqueue our capture work on the dispatcher
    auto queue = controller.DispatcherQueue();

    auto windows = EnumerateWindows();

    for (const auto& window : windows) {
        auto title = window.title();
        std::wstringstream str;
        str << "Window " << ": " << title << std::endl;
        OutputDebugString(str.str().c_str());
    }

    auto d3dDevice = CreateD3DDevice();
#if 1
    IDXGIDevice* dxgiDevice = NULL;
    auto uuid = __uuidof(IDXGIDevice);
    auto hr = d3dDevice->QueryInterface(uuid, (void **)&dxgiDevice);
    auto device = CreateDirect3DDevice(dxgiDevice);
#else
    auto dxgiDevice = d3dDevice.as<IDXGIDevice>();
    auto device = CreateDirect3DDevice(dxgiDevice.get());
#endif

    // window selection
    size_t windowIndex = 0;
    for (int i = 0; i < windows.size(); i++) {
        if (windows[i].title().find(L"League") != std::string::npos) {
            windowIndex = i;
            break;
        }
    }

    std::wstringstream str;
    str << "Capturing window " << ": " << windows[windowIndex].title() << std::endl;
    OutputDebugString(str.str().c_str());
    auto capturedHwnd = windows[windowIndex].hwnd();
    auto item = CreateCaptureItemForWindow(capturedHwnd);
    capture = std::make_unique<SimpleCapture>(device, item);

    capture->StartCapture();

    // Message pump
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}