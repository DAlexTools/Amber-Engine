#ifndef AMBER_RUNTIME_GAME_RUNTIME_PLAYER_H
#define AMBER_RUNTIME_GAME_RUNTIME_PLAYER_H

#include "Game/GameModuleInterface.h"

#include <filesystem>
#include <string>

namespace AE
{

struct RuntimePlayerOptions
{
    std::filesystem::path projectFilePath;
    std::filesystem::path sceneOverride;
    std::string windowTitle;
    int windowWidth = 1280;
    int windowHeight = 720;
    bool smokeTest = false;
    unsigned long smokeFrameCount = 1;
};

class RuntimePlayer
{
public:
    int Run(IGameModule& module, const RuntimePlayerOptions& options, std::string* error = nullptr);

    static int RunFromArguments(
        IGameModule& module,
        int argc,
        char** argv,
        const RuntimePlayerOptions& defaults = RuntimePlayerOptions{});
};

}

#endif
