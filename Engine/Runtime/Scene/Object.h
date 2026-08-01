#ifndef AMBER_RUNTIME_SCENE_OBJECT_H
#define AMBER_RUNTIME_SCENE_OBJECT_H

#include "EntityComponentSystem/ECS.h"
#include "Scene/SceneAsset.h"

#include <string>
#include <utility>

namespace AE::Scene
{

class Object
{
public:
	explicit Object(ObjectData data);
	virtual ~Object() = default;

	Object(const Object&) = delete;
	Object& operator=(const Object&) = delete;

	virtual const char* GetClassName() const;
	virtual void ConfigureEntity(Registry& ownerRegistry);
	virtual void OnCreate();
	virtual void OnDestroy();

	bool HasEntity() const;
	Entity GetEntity() const;
	Registry* GetRegistry() const;
	const ObjectData& GetData() const;
	ObjectData& GetData();

	const std::string& GetName() const;
	const std::string& GetAssetId() const;
	ObjectKind GetKind() const;
	const Transform& GetTransform() const;
	const Vec2& GetSize() const;
	bool IsVisible() const;

	template <typename TComponent, typename... TArgs>
	void AddComponent(TArgs&&... args)
	{
		entity.AddComponent<TComponent>(std::forward<TArgs>(args)...);
	}

	template <typename TComponent>
	bool HasComponent() const
	{
		return entity.HasComponent<TComponent>();
	}

	template <typename TComponent>
	TComponent& GetComponent() const
	{
		return entity.GetComponent<TComponent>();
	}

protected:
	ObjectData data;
	Registry* registry = nullptr;
	Entity entity{-1};
	bool entityConfigured = false;
};

} // namespace AE::Scene

#endif
