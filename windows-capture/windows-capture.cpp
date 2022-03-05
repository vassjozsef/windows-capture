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

int CALLBACK WinMain(
    HINSTANCE instance,
    HINSTANCE previousInstance,
    LPSTR     cmdLine,
    int       cmdShow)
{
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
    auto dxgiDevice = d3dDevice.as<IDXGIDevice>();
    auto device = CreateDirect3DDevice(dxgiDevice.get());

    size_t windowIndex = 0;
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