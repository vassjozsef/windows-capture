#include <iostream>
#include <memory>

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

void Initialized() {
    auto windows = EnumerateWindows();

    size_t index{ 0 };
    for (const auto& window : windows) {
        auto title = window.title();
        std::string t(title.begin(), title.end());
        std::cout << "Window " << index++ << ": " << t << std::endl;
    }

    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice device{ nullptr };
    std::unique_ptr<SimpleCapture> capture_{ nullptr };

    auto d3dDevice = CreateD3DDevice();
    auto dxgiDevice = d3dDevice.as<IDXGIDevice>();
    device = CreateDirect3DDevice(dxgiDevice.get());

    std::cout << "Enter selection: ";
    size_t selectedWindow;
    std::cin >> selectedWindow;

    if (selectedWindow < windows.size()) {
        auto title = windows[selectedWindow].title();
        std::string t(title.begin(), title.end());
        std::cout << "Selected window: " << t << std::endl;

        auto hwnd = windows[selectedWindow].hwnd();
        auto item = CreateCaptureItemForWindow(hwnd);
        auto capture = std::make_unique<SimpleCapture>(device, item);

        capture->StartCapture();

        getchar();
        getchar();
    }
}

int main()
{
    // Init COM
    winrt::init_apartment(winrt::apartment_type::single_threaded);

    // Create a DispatcherQueue for our thread
    auto controller = CreateDispatcherQueueController();
    // Enqueue our capture work on the dispatcher
    auto queue = controller.DispatcherQueue();
    auto success = queue.TryEnqueue([=]() -> void
    {
        Initialized();
    });
    WINRT_VERIFY(success);

    // Message pump
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}