#pragma once

#include <string>
#include <vector>
#include <windows.h>

struct Window
{
public:
    Window(nullptr_t) {}
    Window(HWND hwnd, std::wstring const& title, std::wstring& className) :
        hwnd_{ hwnd },
        title_{ title },
        className_{ className }
    {
    }

    HWND hwnd() const noexcept { return hwnd_; }
    std::wstring title() const noexcept { return title_; }
    std::wstring className() const noexcept { return className_; }

private:
    HWND hwnd_;
    std::wstring title_;
    std::wstring className_;
};

const std::vector<Window> EnumerateWindows();