#include "Scene/SceneAsset.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <utility>

namespace AE::Scene
{
namespace
{
constexpr const char* SceneMagic = "AmberScene";
constexpr int SceneVersion = 2;
constexpr int MinSceneVersion = 1;

void SetError(std::string* error, const std::string& message)
{
	if (error)
	{
		*error = message;
	}
}

bool ReadBoolToken(std::istream& stream, bool& value)
{
	int raw = 0;
	if (!(stream >> raw))
	{
		return false;
	}

	value = raw != 0;
	return true;
}

std::string ToLower(std::string Value)
{
	std::transform(Value.begin(), Value.end(), Value.begin(), [](unsigned char Character)
				   { return static_cast<char>(std::tolower(Character)); });
	return Value;
}
} // namespace

const char* ObjectKindName(ObjectKind kind)
{
	switch (kind)
	{
	case ObjectKind::Camera:
		return "Camera";
	case ObjectKind::Grid:
		return "Grid";
	case ObjectKind::RuntimeWorld:
		return "RuntimeWorld";
	case ObjectKind::AssetInstance:
		return "AssetInstance";
	case ObjectKind::Box:
		return "Box";
	case ObjectKind::Circle:
		return "Circle";
	default:
		return "Empty";
	}
}

bool TryParseObjectKind(const std::string& value, ObjectKind& kind)
{
	if (value == "Camera")
	{
		kind = ObjectKind::Camera;
		return true;
	}
	if (value == "Grid")
	{
		kind = ObjectKind::Grid;
		return true;
	}
	if (value == "RuntimeWorld" || value == "Runtime World")
	{
		kind = ObjectKind::RuntimeWorld;
		return true;
	}
	if (value == "AssetInstance" || value == "Asset Instance")
	{
		kind = ObjectKind::AssetInstance;
		return true;
	}
	if (value == "Box")
	{
		kind = ObjectKind::Box;
		return true;
	}
	if (value == "Circle")
	{
		kind = ObjectKind::Circle;
		return true;
	}
	if (value == "Empty")
	{
		kind = ObjectKind::Empty;
		return true;
	}

	return false;
}

const char* ComponentPropertyTypeName(ComponentPropertyType type)
{
	switch (type)
	{
	case ComponentPropertyType::Bool:
		return "Bool";
	case ComponentPropertyType::Int:
		return "Int";
	case ComponentPropertyType::Float:
		return "Float";
	default:
		return "String";
	}
}

bool TryParseComponentPropertyType(const std::string& value, ComponentPropertyType& type)
{
	if (value == "Bool" || value == "Boolean")
	{
		type = ComponentPropertyType::Bool;
		return true;
	}
	if (value == "Int" || value == "Integer")
	{
		type = ComponentPropertyType::Int;
		return true;
	}
	if (value == "Float")
	{
		type = ComponentPropertyType::Float;
		return true;
	}
	if (value == "String")
	{
		type = ComponentPropertyType::String;
		return true;
	}

	return false;
}

ComponentData* FindComponent(ObjectData& Object, const std::string& ComponentName)
{
	for (ComponentData& Component : Object.components)
	{
		if (Component.name == ComponentName)
		{
			return &Component;
		}
	}

	return nullptr;
}

const ComponentData* FindComponent(const ObjectData& Object, const std::string& ComponentName)
{
	for (const ComponentData& Component : Object.components)
	{
		if (Component.name == ComponentName)
		{
			return &Component;
		}
	}

	return nullptr;
}

ComponentPropertyData* FindComponentProperty(ObjectData& Object, const std::string& ComponentName, const std::string& PropertyName)
{
	ComponentData* Component = FindComponent(Object, ComponentName);
	if (!Component)
	{
		return nullptr;
	}

	for (ComponentPropertyData& Property : Component->properties)
	{
		if (Property.name == PropertyName)
		{
			return &Property;
		}
	}

	return nullptr;
}

const ComponentPropertyData* FindComponentProperty(const ObjectData& Object, const std::string& ComponentName, const std::string& PropertyName)
{
	const ComponentData* Component = FindComponent(Object, ComponentName);
	if (!Component)
	{
		return nullptr;
	}

	for (const ComponentPropertyData& Property : Component->properties)
	{
		if (Property.name == PropertyName)
		{
			return &Property;
		}
	}

	return nullptr;
}

bool GetComponentPropertyBool(const ObjectData& Object, const std::string& ComponentName, const std::string& PropertyName, bool DefaultValue)
{
	const ComponentPropertyData* Property = FindComponentProperty(Object, ComponentName, PropertyName);
	if (!Property)
	{
		return DefaultValue;
	}

	const std::string Value = ToLower(Property->value);
	if (Value == "1" || Value == "true" || Value == "yes")
	{
		return true;
	}
	if (Value == "0" || Value == "false" || Value == "no")
	{
		return false;
	}

	return DefaultValue;
}

int32 GetComponentPropertyInt(const ObjectData& Object, const std::string& ComponentName, const std::string& PropertyName, int32 DefaultValue)
{
	const ComponentPropertyData* Property = FindComponentProperty(Object, ComponentName, PropertyName);
	if (!Property)
	{
		return DefaultValue;
	}

	std::istringstream Stream(Property->value);
	int32 Value = DefaultValue;
	return (Stream >> Value) ? Value : DefaultValue;
}

float GetComponentPropertyFloat(const ObjectData& Object, const std::string& ComponentName, const std::string& PropertyName, float DefaultValue)
{
	const ComponentPropertyData* Property = FindComponentProperty(Object, ComponentName, PropertyName);
	if (!Property)
	{
		return DefaultValue;
	}

	std::istringstream Stream(Property->value);
	float Value = DefaultValue;
	return (Stream >> Value) ? Value : DefaultValue;
}

std::string GetComponentPropertyString(const ObjectData& Object, const std::string& ComponentName, const std::string& PropertyName, std::string DefaultValue)
{
	const ComponentPropertyData* Property = FindComponentProperty(Object, ComponentName, PropertyName);
	return Property ? Property->value : std::move(DefaultValue);
}

bool SaveScene(const Document& document, const std::filesystem::path& path, std::string* error)
{
	std::error_code filesystemError;
	const std::filesystem::path parent = path.parent_path();
	if (!parent.empty())
	{
		std::filesystem::create_directories(parent, filesystemError);
		if (filesystemError)
		{
			SetError(error, "Could not create scene directory: " + filesystemError.message());
			return false;
		}
	}

	std::ofstream file(path, std::ios::out | std::ios::trunc);
	if (!file)
	{
		SetError(error, "Could not open scene for writing: " + path.string());
		return false;
	}

	file << SceneMagic << ' ' << SceneVersion << '\n';
	file << "name " << std::quoted(document.name) << '\n';
	file << std::fixed << std::setprecision(3);

	for (const ObjectData& object : document.objects)
	{
		file
			<< "object "
			<< ObjectKindName(object.kind) << ' '
			<< std::quoted(object.name) << ' '
			<< std::quoted(object.assetId) << ' '
			<< std::quoted(object.className) << ' '
			<< object.transform.position.x << ' '
			<< object.transform.position.y << ' '
			<< object.transform.rotationDegrees << ' '
			<< object.transform.scale.x << ' '
			<< object.transform.scale.y << ' '
			<< object.size.x << ' '
			<< object.size.y << ' '
			<< (object.visible ? 1 : 0) << ' '
			<< (object.locked ? 1 : 0) << '\n';

		for (const ComponentData& Component : object.components)
		{
			if (Component.name.empty())
			{
				continue;
			}

			for (const ComponentPropertyData& Property : Component.properties)
			{
				if (Property.name.empty())
				{
					continue;
				}

				file
					<< "component "
					<< std::quoted(Component.name) << ' '
					<< std::quoted(Property.name) << ' '
					<< ComponentPropertyTypeName(Property.type) << ' '
					<< std::quoted(Property.value) << '\n';
			}
		}
	}

	if (!file)
	{
		SetError(error, "Could not finish writing scene: " + path.string());
		return false;
	}

	return true;
}

bool LoadScene(const std::filesystem::path& path, Document& document, std::string* error)
{
	std::ifstream file(path);
	if (!file)
	{
		SetError(error, "Could not open scene for reading: " + path.string());
		return false;
	}

	std::string magic;
	int version = 0;
	file >> magic >> version;
	if (magic != SceneMagic || version < MinSceneVersion || version > SceneVersion)
	{
		SetError(error, "Unsupported scene file header: " + path.string());
		return false;
	}

	std::string line;
	std::getline(file, line);

	Document loaded;
	ObjectData* CurrentObject = nullptr;
	SizeT lineNumber = 1;
	while (std::getline(file, line))
	{
		++lineNumber;
		if (line.empty())
		{
			continue;
		}

		std::istringstream stream(line);
		std::string command;
		stream >> command;
		if (command.empty() || command[0] == '#')
		{
			continue;
		}

		if (command == "name")
		{
			if (!(stream >> std::quoted(loaded.name)))
			{
				SetError(error, "Invalid scene name at line " + std::to_string(lineNumber));
				return false;
			}
		}
		else if (command == "object")
		{
			std::string kindName;
			ObjectData object;
			if (!(stream >> kindName >> std::quoted(object.name) >> std::quoted(object.assetId)))
			{
				SetError(error, "Invalid scene object header at line " + std::to_string(lineNumber));
				return false;
			}

			if (!TryParseObjectKind(kindName, object.kind))
			{
				SetError(error, "Unknown scene object kind at line " + std::to_string(lineNumber));
				return false;
			}

			stream >> std::ws;
			if (stream.peek() == '"')
			{
				if (!(stream >> std::quoted(object.className)))
				{
					SetError(error, "Invalid scene object class at line " + std::to_string(lineNumber));
					return false;
				}
			}
			else
			{
				object.className = object.kind == ObjectKind::AssetInstance ? "SpriteObject" : "Object";
			}

			if (!(stream >> object.transform.position.x >> object.transform.position.y >> object.transform.rotationDegrees >> object.transform.scale.x >> object.transform.scale.y >> object.size.x >> object.size.y) ||
				!ReadBoolToken(stream, object.visible) ||
				!ReadBoolToken(stream, object.locked))
			{
				SetError(error, "Invalid scene object transform at line " + std::to_string(lineNumber));
				return false;
			}

			loaded.objects.push_back(std::move(object));
			CurrentObject = &loaded.objects.back();
		}
		else if (command == "component")
		{
			if (!CurrentObject)
			{
				SetError(error, "Scene component without object at line " + std::to_string(lineNumber));
				return false;
			}

			std::string ComponentName;
			std::string PropertyName;
			std::string TypeName;
			std::string Value;
			ComponentPropertyType Type = ComponentPropertyType::String;
			if (!(stream >> std::quoted(ComponentName) >> std::quoted(PropertyName) >> TypeName >> std::quoted(Value)) ||
				!TryParseComponentPropertyType(TypeName, Type))
			{
				SetError(error, "Invalid scene component property at line " + std::to_string(lineNumber));
				return false;
			}

			ComponentData* Component = FindComponent(*CurrentObject, ComponentName);
			if (!Component)
			{
				CurrentObject->components.push_back(ComponentData{ComponentName, {}});
				Component = &CurrentObject->components.back();
			}

			Component->properties.push_back(ComponentPropertyData{PropertyName, Type, Value});
		}
		else
		{
			SetError(error, "Unknown scene command at line " + std::to_string(lineNumber));
			return false;
		}
	}

	if (!file.eof())
	{
		SetError(error, "Could not finish reading scene: " + path.string());
		return false;
	}

	document = std::move(loaded);
	return true;
}

} // namespace AE::Scene
