#include "screen_enumeration.h"

BOOL CALLBACK EnumScreensProc(HMONITOR hmonitor, HDC hdc, LPRECT lRect, LPARAM lParam)
{
    auto window = Screen(hmonitor);

    std::vector<Screen>& screens = *reinterpret_cast<std::vector<Screen>*>(lParam);
    screens.push_back(window);

    return TRUE;
}

const std::vector<Screen> EnumerateScreens()
{
    std::vector<Screen> screens;
    EnumDisplayMonitors(NULL, NULL, EnumScreensProc, reinterpret_cast<LPARAM>(&screens));

    return screens;
}