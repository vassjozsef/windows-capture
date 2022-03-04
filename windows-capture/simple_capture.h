#pragma once

#include <atomic>

#include <Unknwn.h>

#include <d3d11.h>

#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.Direct3d11.h>

class SimpleCapture
{
public:
    SimpleCapture(
        winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice const& device,
        winrt::Windows::Graphics::Capture::GraphicsCaptureItem const& item);
    ~SimpleCapture() { Close(); }

    void StartCapture();

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
    winrt::com_ptr<ID3D11DeviceContext> d3dContext_{ nullptr };

    std::atomic<bool> closed_ = false;
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::FrameArrived_revoker frameArrived_;
};