#ifndef MYGAME_MODULE_H
#define MYGAME_MODULE_H

#include "Core/Platform/PlatformTypes.h"
#include "Game/GameModuleInterface.h"

#include <string>

namespace MyGame
{

class MyGameModule final : public AE::IGameModule
{
public:
	const char* GetName() const override;
	bool StartPlay(const AE::GameModuleStartContext& context, std::string* error) override;
	void Tick(const AE::GameModuleTickContext& context) override;
	void Render(const AE::GameModuleRenderContext& context) override;
	void StopPlay() override;

private:
	AE::uint64 tickCount = 0;
	AE::uint64 renderCount = 0;
};

} // namespace MyGame

#endif
