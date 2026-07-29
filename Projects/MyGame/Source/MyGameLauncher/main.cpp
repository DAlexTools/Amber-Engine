#include "MyGameModule.h"

#include "Game/RuntimePlayer.h"

int main(int argc, char** argv)
{
    MyGame::MyGameModule module;
    AE::RuntimePlayerOptions options;
    options.projectFilePath = "MyGame.amberproject";
    options.windowTitle = "MyGame";
    return AE::RuntimePlayer::RunFromArguments(module, argc, argv, options);
}
