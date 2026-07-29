#include "MyGameModule.h"

#include "Logging/Logger.h"

#include <string>

namespace MyGame
{

const char* MyGameModule::GetName() const
{
    return "MyGameModule";
}

bool MyGameModule::StartPlay(const AE::GameModuleStartContext& context, std::string*)
{
    tickCount = 0;
    renderCount = 0;
    AE::Logger::Log("MyGame StartPlay: " + context.projectName, "MyGame");
    return true;
}

void MyGameModule::Tick(const AE::GameModuleTickContext&)
{
    ++tickCount;
}

void MyGameModule::Render(const AE::GameModuleRenderContext&)
{
    ++renderCount;
}

void MyGameModule::StopPlay()
{
    AE::Logger::Log(
        "MyGame StopPlay ticks=" + std::to_string(tickCount) +
            " renders=" + std::to_string(renderCount),
        "MyGame");
}

}
