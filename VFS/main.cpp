// Author : Jihong Shin (snowapril)

#include <pch.h>
#include <Common/Logger.h>
#include <Application.h>
#include <iostream>

extern "C"
{
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
}

int main(int argc, char* argv[])
{
    vfs::Logging::SetAllStream(&std::cout);

    vfs::Application app;
    if (!app.initialize(argc, argv))
    {
        VFS_ERROR << "Failed to initialize application";
        return -1;
    }

    // Main rendering loop
    app.run();

    return 0;
}