#include "PlatformerSceneObjects.h"

#include "Scene/ObjectFactory.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <memory>
#include <utility>

namespace PlatformerScene
{
namespace
{
void SetClassName(AE::Scene::Object& object)
{
	object.GetData().className = object.GetClassName();
}

bool ContainsText(const std::string& Text, const std::string& Needle)
{
	std::string LowerText = Text;
	std::string LowerNeedle = Needle;
	std::transform(LowerText.begin(), LowerText.end(), LowerText.begin(), [](unsigned char Character)
				   { return static_cast<char>(std::tolower(Character)); });
	std::transform(LowerNeedle.begin(), LowerNeedle.end(), LowerNeedle.begin(), [](unsigned char Character)
				   { return static_cast<char>(std::tolower(Character)); });
	return LowerText.find(LowerNeedle) != std::string::npos;
}

FEnemySpawnComponent BuildEnemySpawnComponent(const AE::Scene::ObjectData& ObjectData)
{
	FEnemySpawnComponent Component;
	const bool Hopper = ContainsText(ObjectData.name, "hopper");
	const bool Sentry = ContainsText(ObjectData.name, "sentry");
	const bool Shooter = Sentry || ContainsText(ObjectData.name, "shooter") || ContainsText(ObjectData.name, "turret");

	Component.Speed = Sentry ? 54.0f : (Hopper ? 58.0f : 70.0f);
	Component.Direction = ContainsText(ObjectData.name, "left") ? -1.0f : 1.0f;
	Component.PatrolWidth = Sentry ? 260.0f : 192.0f;
	Component.MaxHealth = Sentry ? 4 : (Hopper ? 3 : 2);
	Component.JumpCooldown = Hopper ? 1.1f : 1.2f;
	Component.JumpVelocity = Hopper ? -360.0f : -340.0f;
	Component.AlertRange = Shooter ? 460.0f : 260.0f;
	Component.CanShoot = Shooter;
	Component.ShootCooldown = Sentry ? 1.05f : 1.45f;
	Component.ShootRange = Sentry ? 460.0f : 340.0f;

	Component.Speed = AE::Scene::GetComponentPropertyFloat(ObjectData, "FEnemySpawnComponent", "Speed", Component.Speed);
	Component.Direction = AE::Scene::GetComponentPropertyFloat(ObjectData, "FEnemySpawnComponent", "Direction", Component.Direction);
	Component.PatrolWidth = AE::Scene::GetComponentPropertyFloat(ObjectData, "FEnemySpawnComponent", "PatrolWidth", Component.PatrolWidth);
	Component.MaxHealth = AE::Scene::GetComponentPropertyInt(ObjectData, "FEnemySpawnComponent", "MaxHealth", Component.MaxHealth);
	Component.JumpCooldown = AE::Scene::GetComponentPropertyFloat(ObjectData, "FEnemySpawnComponent", "JumpCooldown", Component.JumpCooldown);
	Component.JumpVelocity = AE::Scene::GetComponentPropertyFloat(ObjectData, "FEnemySpawnComponent", "JumpVelocity", Component.JumpVelocity);
	Component.AlertRange = AE::Scene::GetComponentPropertyFloat(ObjectData, "FEnemySpawnComponent", "AlertRange", Component.AlertRange);
	Component.CanShoot = AE::Scene::GetComponentPropertyBool(ObjectData, "FEnemySpawnComponent", "CanShoot", Component.CanShoot);
	Component.ShootCooldown = AE::Scene::GetComponentPropertyFloat(ObjectData, "FEnemySpawnComponent", "ShootCooldown", Component.ShootCooldown);
	Component.ShootRange = AE::Scene::GetComponentPropertyFloat(ObjectData, "FEnemySpawnComponent", "ShootRange", Component.ShootRange);
	Component.ProjectileSpeed = AE::Scene::GetComponentPropertyFloat(ObjectData, "FEnemySpawnComponent", "ProjectileSpeed", Component.ProjectileSpeed);
	return Component;
}

FPhysicsBoxComponent BuildPhysicsBoxComponent(const AE::Scene::ObjectData& ObjectData)
{
	FPhysicsBoxComponent Component;
	Component.Mass = AE::Scene::GetComponentPropertyFloat(ObjectData, "FPhysicsBoxComponent", "Mass", Component.Mass);
	Component.Friction = AE::Scene::GetComponentPropertyFloat(ObjectData, "FPhysicsBoxComponent", "Friction", Component.Friction);
	Component.Restitution = AE::Scene::GetComponentPropertyFloat(ObjectData, "FPhysicsBoxComponent", "Restitution", Component.Restitution);
	return Component;
}

FPhysicsCircleComponent BuildPhysicsCircleComponent(const AE::Scene::ObjectData& ObjectData)
{
	FPhysicsCircleComponent Component;
	Component.Mass = AE::Scene::GetComponentPropertyFloat(ObjectData, "FPhysicsCircleComponent", "Mass", Component.Mass);
	Component.Friction = AE::Scene::GetComponentPropertyFloat(ObjectData, "FPhysicsCircleComponent", "Friction", Component.Friction);
	Component.Restitution = AE::Scene::GetComponentPropertyFloat(ObjectData, "FPhysicsCircleComponent", "Restitution", Component.Restitution);
	return Component;
}

FPhysicsBridgeComponent BuildPhysicsBridgeComponent(const AE::Scene::ObjectData& ObjectData)
{
	FPhysicsBridgeComponent Component;
	Component.SegmentCount = AE::Scene::GetComponentPropertyInt(ObjectData, "FPhysicsBridgeComponent", "SegmentCount", Component.SegmentCount);
	return Component;
}

FPhysicsChainComponent BuildPhysicsChainComponent(const AE::Scene::ObjectData& ObjectData)
{
	FPhysicsChainComponent Component;
	Component.LinkCount = AE::Scene::GetComponentPropertyInt(ObjectData, "FPhysicsChainComponent", "LinkCount", Component.LinkCount);
	return Component;
}

FMovingPlatformComponent BuildMovingPlatformComponent(const AE::Scene::ObjectData& ObjectData)
{
	FMovingPlatformComponent Component;
	Component.VerticalMotion = ContainsText(ObjectData.name, "elevator") ||
							   ContainsText(ObjectData.name, "vertical") ||
							   std::abs(ObjectData.size.y * ObjectData.transform.scale.y) > std::abs(ObjectData.size.x * ObjectData.transform.scale.x);
	Component.Amplitude = Component.VerticalMotion ? 72.0f : 92.0f;
	Component.Speed = Component.VerticalMotion ? 1.05f : 1.15f;
	Component.Phase = Component.VerticalMotion ? 1.4f : 0.0f;

	Component.VerticalMotion = AE::Scene::GetComponentPropertyBool(ObjectData, "FMovingPlatformComponent", "VerticalMotion", Component.VerticalMotion);
	Component.Amplitude = AE::Scene::GetComponentPropertyFloat(ObjectData, "FMovingPlatformComponent", "Amplitude", Component.Amplitude);
	Component.Speed = AE::Scene::GetComponentPropertyFloat(ObjectData, "FMovingPlatformComponent", "Speed", Component.Speed);
	Component.Phase = AE::Scene::GetComponentPropertyFloat(ObjectData, "FMovingPlatformComponent", "Phase", Component.Phase);
	return Component;
}
} // namespace

PlayerSpawnObject::PlayerSpawnObject(AE::Scene::ObjectData data)
	: BoxObject(std::move(data))
{
	SetClassName(*this);
}

const char* PlayerSpawnObject::GetClassName() const
{
	return "PlayerSpawnObject";
}

void PlayerSpawnObject::ConfigureEntity(Registry& ownerRegistry)
{
	BoxObject::ConfigureEntity(ownerRegistry);
	AddComponent<FPlayerSpawnComponent>();
}

GoalObject::GoalObject(AE::Scene::ObjectData data)
	: BoxObject(std::move(data))
{
	SetClassName(*this);
}

const char* GoalObject::GetClassName() const
{
	return "GoalObject";
}

void GoalObject::ConfigureEntity(Registry& ownerRegistry)
{
	BoxObject::ConfigureEntity(ownerRegistry);
	AddComponent<FGoalComponent>();
}

CoinObject::CoinObject(AE::Scene::ObjectData data)
	: CircleObject(std::move(data))
{
	SetClassName(*this);
}

const char* CoinObject::GetClassName() const
{
	return "CoinObject";
}

void CoinObject::ConfigureEntity(Registry& ownerRegistry)
{
	CircleObject::ConfigureEntity(ownerRegistry);
	AddComponent<FCoinComponent>();
}

SolidPlatformObject::SolidPlatformObject(AE::Scene::ObjectData data)
	: BoxObject(std::move(data))
{
	SetClassName(*this);
}

const char* SolidPlatformObject::GetClassName() const
{
	return "SolidPlatformObject";
}

void SolidPlatformObject::ConfigureEntity(Registry& ownerRegistry)
{
	BoxObject::ConfigureEntity(ownerRegistry);
	AddComponent<FSolidPlatformComponent>();
}

EnemySpawnObject::EnemySpawnObject(AE::Scene::ObjectData data)
	: BoxObject(std::move(data))
{
	SetClassName(*this);
}

const char* EnemySpawnObject::GetClassName() const
{
	return "EnemySpawnObject";
}

void EnemySpawnObject::ConfigureEntity(Registry& ownerRegistry)
{
	BoxObject::ConfigureEntity(ownerRegistry);
	AddComponent<FEnemySpawnComponent>(BuildEnemySpawnComponent(GetData()));
}

PhysicsBoxObject::PhysicsBoxObject(AE::Scene::ObjectData data)
	: BoxObject(std::move(data))
{
	SetClassName(*this);
}

const char* PhysicsBoxObject::GetClassName() const
{
	return "PhysicsBoxObject";
}

void PhysicsBoxObject::ConfigureEntity(Registry& ownerRegistry)
{
	BoxObject::ConfigureEntity(ownerRegistry);
	AddComponent<FPhysicsBoxComponent>(BuildPhysicsBoxComponent(GetData()));
}

PhysicsCircleObject::PhysicsCircleObject(AE::Scene::ObjectData data)
	: CircleObject(std::move(data))
{
	SetClassName(*this);
}

const char* PhysicsCircleObject::GetClassName() const
{
	return "PhysicsCircleObject";
}

void PhysicsCircleObject::ConfigureEntity(Registry& ownerRegistry)
{
	CircleObject::ConfigureEntity(ownerRegistry);
	AddComponent<FPhysicsCircleComponent>(BuildPhysicsCircleComponent(GetData()));
}

PhysicsBridgeObject::PhysicsBridgeObject(AE::Scene::ObjectData data)
	: BoxObject(std::move(data))
{
	SetClassName(*this);
}

const char* PhysicsBridgeObject::GetClassName() const
{
	return "PhysicsBridgeObject";
}

void PhysicsBridgeObject::ConfigureEntity(Registry& ownerRegistry)
{
	BoxObject::ConfigureEntity(ownerRegistry);
	AddComponent<FPhysicsBridgeComponent>(BuildPhysicsBridgeComponent(GetData()));
}

PhysicsChainObject::PhysicsChainObject(AE::Scene::ObjectData data)
	: BoxObject(std::move(data))
{
	SetClassName(*this);
}

const char* PhysicsChainObject::GetClassName() const
{
	return "PhysicsChainObject";
}

void PhysicsChainObject::ConfigureEntity(Registry& ownerRegistry)
{
	BoxObject::ConfigureEntity(ownerRegistry);
	AddComponent<FPhysicsChainComponent>(BuildPhysicsChainComponent(GetData()));
}

MovingPlatformObject::MovingPlatformObject(AE::Scene::ObjectData data)
	: BoxObject(std::move(data))
{
	SetClassName(*this);
}

const char* MovingPlatformObject::GetClassName() const
{
	return "MovingPlatformObject";
}

void MovingPlatformObject::ConfigureEntity(Registry& ownerRegistry)
{
	BoxObject::ConfigureEntity(ownerRegistry);
	AddComponent<FMovingPlatformComponent>(BuildMovingPlatformComponent(GetData()));
}

void RegisterPlatformerSceneObjects(AE::Scene::ObjectFactory& factory)
{
	factory.RegisterClass("PlayerSpawnObject", [](AE::Scene::ObjectData data)
						  { return std::make_unique<PlayerSpawnObject>(std::move(data)); });
	factory.RegisterClass("GoalObject", [](AE::Scene::ObjectData data)
						  { return std::make_unique<GoalObject>(std::move(data)); });
	factory.RegisterClass("CoinObject", [](AE::Scene::ObjectData data)
						  { return std::make_unique<CoinObject>(std::move(data)); });
	factory.RegisterClass("SolidPlatformObject", [](AE::Scene::ObjectData data)
						  { return std::make_unique<SolidPlatformObject>(std::move(data)); });
	factory.RegisterClass("EnemySpawnObject", [](AE::Scene::ObjectData data)
						  { return std::make_unique<EnemySpawnObject>(std::move(data)); });
	factory.RegisterClass("PhysicsBoxObject", [](AE::Scene::ObjectData data)
						  { return std::make_unique<PhysicsBoxObject>(std::move(data)); });
	factory.RegisterClass("PhysicsCircleObject", [](AE::Scene::ObjectData data)
						  { return std::make_unique<PhysicsCircleObject>(std::move(data)); });
	factory.RegisterClass("PhysicsBridgeObject", [](AE::Scene::ObjectData data)
						  { return std::make_unique<PhysicsBridgeObject>(std::move(data)); });
	factory.RegisterClass("PhysicsChainObject", [](AE::Scene::ObjectData data)
						  { return std::make_unique<PhysicsChainObject>(std::move(data)); });
	factory.RegisterClass("MovingPlatformObject", [](AE::Scene::ObjectData data)
						  { return std::make_unique<MovingPlatformObject>(std::move(data)); });
}

bool IsPlatformerGameplayClass(const std::string& className)
{
	return className == "PlayerSpawnObject" ||
		   className == "GoalObject" ||
		   className == "CoinObject" ||
		   className == "SolidPlatformObject" ||
		   className == "EnemySpawnObject" ||
		   className == "PhysicsBoxObject" ||
		   className == "PhysicsCircleObject" ||
		   className == "PhysicsBridgeObject" ||
		   className == "PhysicsChainObject" ||
		   className == "MovingPlatformObject";
}

} // namespace PlatformerScene
