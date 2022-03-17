#include "simple_capture.h"

#include <sstream>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3d11.h>

#include "direct3d11_interop.h"

SimpleCapture::SimpleCapture(
    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice const& device,
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem const& item)
{
    item_ = item;
    device_ = device;

    // Set up 
    auto size = item_.Size();
    std::wstringstream str;
    str << "Capture created with frame size: " << size.Width << " x " << size.Height << std::endl;
    OutputDebugString(str.str().c_str());

    // Create framepool, define pixel format (DXGI_FORMAT_B8G8R8A8_UNORM), and frame size. 
    framePool_ = winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::Create(
        device_,
        winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
        2,
        size);
    session_ = framePool_.CreateCaptureSession(item_);
    lastSize_ = size;
   frameArrived_ = framePool_.FrameArrived(winrt::auto_revoke, { this, &SimpleCapture::OnFrameArrived });
}

void SimpleCapture::StartCapture()
{
    CheckClosed();
    try {
        session_.StartCapture();
    }
    catch (...) {
        OutputDebugString(L"Failed to start capture\n");
    }
}

void SimpleCapture::Close()
{
    auto expected = false;
    if (closed_.compare_exchange_strong(expected, true))
    {
        frameArrived_.revoke();
        framePool_.Close();
        session_.Close();

        framePool_ = nullptr;
        session_ = nullptr;
        item_ = nullptr;
    }
}

void SimpleCapture::OnFrameArrived(
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& sender,
    winrt::Windows::Foundation::IInspectable const&)
{
    count_++;
    if (!(count_ % 100)) {
        std::wstringstream str;
        str << "Frames received: " << count_ << std::endl;
        OutputDebugString(str.str().c_str());
    }
    auto newSize = false;

    {
        auto frame = sender.TryGetNextFrame();
        auto frameContentSize = frame.ContentSize();

        if (frameContentSize.Width != lastSize_.Width ||
            frameContentSize.Height != lastSize_.Height)
        {
            // The thing we have been capturing has changed size.
            // We need to resize our swap chain first, then blit the pixels.
            // After we do that, retire the frame and then recreate our frame pool.
            newSize = true;
            lastSize_ = frameContentSize;
        }
    }

    if (newSize)
    {
        std::wstringstream str;
        str << "New frame size: " << lastSize_.Width << " x " << lastSize_.Height << std::endl;
        OutputDebugString(str.str().c_str());
        framePool_.Recreate(
            device_,
            winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
            2,
            lastSize_);
    }
}
