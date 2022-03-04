#include <memory>

// #include "d3d_helpers.h"
// #include "direct3d11_interop.h"
#include "simple_capture.h"
#include "window_enumeration.h"

int main()
{
    auto windows = EnumerateWindows();

    for (const auto& window : windows) {
        auto title = window.title();
        std::string t(title.begin(), title.end());
        printf("Window: %s\n", t.c_str());
    }

    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice device{ nullptr };
    std::unique_ptr<SimpleCapture> capture_{ nullptr };

  //  auto d3dDevice = CreateD3DDevice();
  //  auto dxgiDevice = d3dDevice.as<IDXGIDevice>();
  //  device = CreateDirect3DDevice(dxgiDevice.get());
}