#pragma once

#include <string>
#include <vector>
#include <windows.h>

struct Screen
{
public:
    Screen(nullptr_t) {}
    Screen(HMONITOR hmonitor) : hmonitor_{ hmonitor }
    {
    }

    HMONITOR hmonitor() const noexcept { return hmonitor_; }

private:
    HMONITOR hmonitor_;
    std::wstring title_;
    std::wstring className_;
};

const std::vector<Screen> EnumerateScreens();