// Author : Jihong Shin (snowapril)

#include <Common/pch.h>
#include <Application.h>
#include <iostream>

// TODO(snowapril) : remove below codes (for laptop external gpu auto-selection)
extern "C" {
    _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

int main(int argc, char* argv[])
{
    vfs::Application app;
    if (!app.initialize(argc, argv))
    {
        std::cerr << "Failed to initialize application\n";
        return -1;
    }

    // Main rendering loop
    app.run();

    return 0;
}