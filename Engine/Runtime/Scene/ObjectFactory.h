#ifndef AMBER_RUNTIME_SCENE_OBJECT_FACTORY_H
#define AMBER_RUNTIME_SCENE_OBJECT_FACTORY_H

#include "Scene/Object.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace AE::Scene
{

class ObjectFactory
{
public:
    using Creator = std::function<std::unique_ptr<Object>(ObjectData)>;

    ObjectFactory();

    void RegisterClass(std::string className, Creator creator);
    std::unique_ptr<Object> CreateObject(const ObjectData& data, Registry* registry = nullptr) const;
    std::vector<std::unique_ptr<Object>> CreateObjects(const Document& document, Registry* registry = nullptr) const;

private:
    std::unordered_map<std::string, Creator> creators;
};

}

#endif
