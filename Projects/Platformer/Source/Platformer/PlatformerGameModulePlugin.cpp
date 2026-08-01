#include "PlatformerGameModule.h"

#include "Game/GameModuleInterface.h"

AMBER_GAME_MODULE_EXPORT AE::IGameModule* AmberCreateGameModule()
{
	return new PlatformerGameModule();
}

AMBER_GAME_MODULE_EXPORT void AmberDestroyGameModule(AE::IGameModule* module)
{
	delete module;
}
