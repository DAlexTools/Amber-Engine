#include "MyGameModule.h"

#include "Logging/Logger.h"

namespace MyGame
{

void MyGameModule::Startup()
{
    AE::Logger::Log("MyGame module startup", "MyGame");
}

void MyGameModule::Shutdown()
{
    AE::Logger::Log("MyGame module shutdown", "MyGame");
}

const char* MyGameModule::GetName() const
{
    return "MyGame";
}

}
