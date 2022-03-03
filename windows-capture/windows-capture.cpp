#include "window_enumeration.h"

int main()
{
    auto windows = EnumerateWindows();

    for (const auto& window : windows) {
        auto title = window.title();
        std::string t(title.begin(), title.end());
        printf("Window: %s\n", t.c_str());
    }
}