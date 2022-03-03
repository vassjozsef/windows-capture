#include "simple_capture.h"

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
    auto d3dDevice = GetDXGIInterfaceFromObject<ID3D11Device>(device_);
    d3dDevice->GetImmediateContext(d3dContext_.put());

    auto size = item_.Size();

/*
    m_swapChain = CreateDXGISwapChain(
        d3dDevice,
        static_cast<uint32_t>(size.Width),
        static_cast<uint32_t>(size.Height),
        static_cast<DXGI_FORMAT>(DirectXPixelFormat::B8G8R8A8UIntNormalized),
        2);
*/

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

// Start sending capture frames
void SimpleCapture::StartCapture()
{
    CheckClosed();
    session_.StartCapture();
}

/*
ICompositionSurface SimpleCapture::CreateSurface(
    Compositor const& compositor)
{
    CheckClosed();
    return CreateCompositionSurfaceForSwapChain(compositor, m_swapChain.get());
}
*/

// Process captured frames
void SimpleCapture::Close()
{
    auto expected = false;
    if (closed_.compare_exchange_strong(expected, true))
    {
        frameArrived_.revoke();
        framePool_.Close();
        session_.Close();

  //      m_swapChain = nullptr;
        framePool_ = nullptr;
        session_ = nullptr;
        item_ = nullptr;
    }
}

void SimpleCapture::OnFrameArrived(
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& sender,
    winrt::Windows::Foundation::IInspectable const&)
{
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
/*
            m_swapChain->ResizeBuffers(
                2,
                static_cast<uint32_t>(m_lastSize.Width),
                static_cast<uint32_t>(m_lastSize.Height),
                static_cast<DXGI_FORMAT>(DirectXPixelFormat::B8G8R8A8UIntNormalized),
                0);
*/
        }

        /*
        {
           
            auto frameSurface = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());

            winrt::com_ptr<ID3D11Texture2D> backBuffer;
            winrt::check_hresult(m_swapChain->GetBuffer(0, guid_of<ID3D11Texture2D>(), backBuffer.put_void()));

            d3dContext_->CopyResource(backBuffer.get(), frameSurface.get());
        }
        */
    }

  //  DXGI_PRESENT_PARAMETERS presentParameters = { 0 };
  //  m_swapChain->Present1(1, 0, &presentParameters);

    if (newSize)
    {
        framePool_.Recreate(
            device_,
            winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
            2,
            lastSize_);
    }
}
