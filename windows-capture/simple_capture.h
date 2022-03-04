#pragma once

#include <atomic>

#include <Unknwn.h>
//#include <inspectable.h>


//#include <winrt/Windows.Foundation.h>
//#include <winrt/Windows.System.h>
//#include <winrt/Windows.UI.h>
//#include <winrt/Windows.UI.Composition.h>
//#include <winrt/Windows.UI.Composition.Desktop.h>
//#include <winrt/Windows.UI.Popups.h>
#include <winrt/Windows.Graphics.Capture.h>
//#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3d11.h>

#include <d3d11.h>
//#include <dxgi1_6.h>
//#include <d2d1_3.h>
//#include <wincodec.h>


class SimpleCapture
{
public:
    SimpleCapture(
        winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice const& device,
        winrt::Windows::Graphics::Capture::GraphicsCaptureItem const& item);
    ~SimpleCapture() { Close(); }

    void StartCapture();
 //   winrt::Windows::UI::Composition::ICompositionSurface CreateSurface(
 //       winrt::Windows::UI::Composition::Compositor const& compositor);

    void Close();

private:
    void OnFrameArrived(
        winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& sender,
        winrt::Windows::Foundation::IInspectable const& args);

    void CheckClosed()
    {
        if (closed_.load() == true)
        {
            throw winrt::hresult_error(RO_E_CLOSED);
        }
    }

private:
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem item_{ nullptr };
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool framePool_{ nullptr };
    winrt::Windows::Graphics::Capture::GraphicsCaptureSession session_{ nullptr };
    winrt::Windows::Graphics::SizeInt32 lastSize_;

    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice device_{ nullptr };
    //   winrt::com_ptr<IDXGISwapChain1> swapChain_{ nullptr };
    winrt::com_ptr<ID3D11DeviceContext> d3dContext_{ nullptr };

    std::atomic<bool> closed_ = false;
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::FrameArrived_revoker frameArrived_;
};