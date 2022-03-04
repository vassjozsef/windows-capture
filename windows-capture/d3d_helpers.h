#pragma once

#include <winrt/Windows.Foundation.h>
#include <d3d11.h>

winrt::com_ptr<ID3D11Device> CreateD3DDevice();