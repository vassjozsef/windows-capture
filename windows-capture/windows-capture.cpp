#include "window_enumeration.h"

int main()
{
    auto windows = EnumerateWindows();

    for (const auto& window : windows) {
        auto title = window.title();
        std::string t(title.begin(), title.end());
        printf("Window: %s\n", t.c_str());
    }

//    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice device_{ nullptr };
//    std::unique_ptr<SimpleCapture> capture_{ nullptr };
}