#include "Game/GameModuleInterface.h"

namespace AE
{

void IGameModule::RegisterSceneObjects(Scene::ObjectFactory&)
{
}

bool IGameModule::StartPlay(const GameModuleStartContext&, std::string*)
{
	return true;
}

void IGameModule::Tick(const GameModuleTickContext&)
{
}

void IGameModule::Render(const GameModuleRenderContext&)
{
}

void IGameModule::StopPlay()
{
}

} // namespace AE
