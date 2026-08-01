#include "Actors/ActorTypeRegistry.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <utility>

namespace AE::Editor
{
namespace
{
constexpr const char* ActorTypesMagic = "AmberActorTypes";
constexpr int ActorTypesVersion = 1;

void SetError(std::string* Error, const std::string& Message)
{
	if (Error)
	{
		*Error = Message;
	}
}

std::string Trim(std::string Value)
{
	const auto First = std::find_if_not(
		Value.begin(),
		Value.end(),
		[](unsigned char Character)
		{
			return std::isspace(Character) != 0;
		});
	const auto Last = std::find_if_not(
						  Value.rbegin(),
						  Value.rend(),
						  [](unsigned char Character)
						  {
							  return std::isspace(Character) != 0;
						  })
						  .base();

	if (First >= Last)
	{
		return {};
	}
	return std::string(First, Last);
}

int HexDigit(char Character)
{
	if (Character >= '0' && Character <= '9')
	{
		return Character - '0';
	}
	if (Character >= 'a' && Character <= 'f')
	{
		return 10 + Character - 'a';
	}
	if (Character >= 'A' && Character <= 'F')
	{
		return 10 + Character - 'A';
	}
	return -1;
}

bool ParseHexByte(const std::string& Value, SizeT Offset, uint8& OutByte)
{
	if (Offset + 1 >= Value.size())
	{
		return false;
	}

	const int High = HexDigit(Value[Offset]);
	const int Low = HexDigit(Value[Offset + 1]);
	if (High < 0 || Low < 0)
	{
		return false;
	}

	OutByte = static_cast<uint8>((High << 4) | Low);
	return true;
}

bool ParseColor(std::string Value, FActorPreviewColor& OutColor)
{
	if (!Value.empty() && Value.front() == '#')
	{
		Value.erase(Value.begin());
	}

	if (Value.size() != 6 && Value.size() != 8)
	{
		return false;
	}

	FActorPreviewColor Color;
	if (!ParseHexByte(Value, 0, Color.R) ||
		!ParseHexByte(Value, 2, Color.G) ||
		!ParseHexByte(Value, 4, Color.B))
	{
		return false;
	}

	Color.A = 255;
	if (Value.size() == 8 && !ParseHexByte(Value, 6, Color.A))
	{
		return false;
	}

	OutColor = Color;
	return true;
}

SceneObjectKind ToEditorKind(AE::Scene::ObjectKind Kind)
{
	switch (Kind)
	{
	case AE::Scene::ObjectKind::Camera:
		return SceneObjectKind::Camera;
	case AE::Scene::ObjectKind::Grid:
		return SceneObjectKind::Grid;
	case AE::Scene::ObjectKind::RuntimeWorld:
		return SceneObjectKind::RuntimeWorld;
	case AE::Scene::ObjectKind::AssetInstance:
		return SceneObjectKind::AssetInstance;
	case AE::Scene::ObjectKind::Box:
		return SceneObjectKind::Box;
	case AE::Scene::ObjectKind::Circle:
		return SceneObjectKind::Circle;
	default:
		return SceneObjectKind::Empty;
	}
}

bool ParseEditorKind(const std::string& Value, SceneObjectKind& OutKind)
{
	AE::Scene::ObjectKind RuntimeKind = AE::Scene::ObjectKind::Empty;
	if (!AE::Scene::TryParseObjectKind(Value, RuntimeKind))
	{
		return false;
	}

	OutKind = ToEditorKind(RuntimeKind);
	return true;
}

FActorTypeDefinition ActorType(
	std::string TypeId,
	std::string DisplayName,
	std::string ClassName,
	std::string Category,
	SceneObjectKind Kind,
	EditorVec2 DefaultSize,
	FActorPreviewColor FillColor,
	FActorPreviewColor OutlineColor)
{
	FActorTypeDefinition Definition;
	Definition.TypeId = std::move(TypeId);
	Definition.DisplayName = std::move(DisplayName);
	Definition.ClassName = std::move(ClassName);
	Definition.Category = std::move(Category);
	Definition.Kind = Kind;
	Definition.DefaultSize = DefaultSize;
	Definition.FillColor = FillColor;
	Definition.OutlineColor = OutlineColor;
	return Definition;
}

bool ParseActorLine(
	std::istringstream& Stream,
	FActorTypeDefinition& OutActorType,
	SizeT LineNumber,
	std::string* Error)
{
	std::string TypeId;
	std::string DisplayName;
	std::string ClassName;
	std::string Category;
	std::string KindName;
	float Width = 0.0f;
	float Height = 0.0f;

	if (!(Stream >> std::quoted(TypeId) >> std::quoted(DisplayName) >> std::quoted(ClassName) >> std::quoted(Category) >>
		  std::quoted(KindName) >> Width >> Height))
	{
		SetError(Error, "Invalid actor definition at line " + std::to_string(LineNumber));
		return false;
	}

	SceneObjectKind Kind = SceneObjectKind::Empty;
	if (!ParseEditorKind(KindName, Kind))
	{
		SetError(Error, "Invalid actor kind at line " + std::to_string(LineNumber) + ": " + KindName);
		return false;
	}

	FActorPreviewColor FillColor = Kind == SceneObjectKind::Circle ? FActorPreviewColor{225, 142, 72, 180} : FActorPreviewColor{78, 150, 204, 176};
	FActorPreviewColor OutlineColor{104, 184, 238, 230};

	std::string FillColorText;
	if (Stream >> std::quoted(FillColorText))
	{
		if (!ParseColor(FillColorText, FillColor))
		{
			SetError(Error, "Invalid actor fill color at line " + std::to_string(LineNumber) + ": " + FillColorText);
			return false;
		}

		std::string OutlineColorText;
		if (Stream >> std::quoted(OutlineColorText) && !ParseColor(OutlineColorText, OutlineColor))
		{
			SetError(Error, "Invalid actor outline color at line " + std::to_string(LineNumber) + ": " + OutlineColorText);
			return false;
		}
	}

	OutActorType = ActorType(
		std::move(TypeId),
		std::move(DisplayName),
		std::move(ClassName),
		std::move(Category),
		Kind,
		EditorVec2{Width, Height},
		FillColor,
		OutlineColor);
	return true;
}

bool ParsePropertyLine(
	std::istringstream& Stream,
	FActorComponentSchema& Component,
	SizeT LineNumber,
	std::string* Error)
{
	std::string PropertyName;
	std::string TypeName;
	std::string DefaultValue;
	if (!(Stream >> std::quoted(PropertyName) >> std::quoted(TypeName) >> std::quoted(DefaultValue)))
	{
		SetError(Error, "Invalid actor property at line " + std::to_string(LineNumber));
		return false;
	}

	AE::Scene::ComponentPropertyType Type = AE::Scene::ComponentPropertyType::String;
	if (!AE::Scene::TryParseComponentPropertyType(TypeName, Type))
	{
		SetError(Error, "Invalid actor property type at line " + std::to_string(LineNumber) + ": " + TypeName);
		return false;
	}

	Component.Properties.push_back(FActorComponentPropertySchema{std::move(PropertyName), Type, std::move(DefaultValue)});
	return true;
}
} // namespace

void FActorTypeRegistry::Clear()
{
	ActorTypes.clear();
}

void FActorTypeRegistry::RegisterActorType(FActorTypeDefinition ActorType)
{
	if (ActorType.TypeId.empty() || ActorType.ClassName.empty())
	{
		return;
	}

	const auto ExistingActor = std::find_if(
		ActorTypes.begin(),
		ActorTypes.end(),
		[&ActorType](const FActorTypeDefinition& RegisteredActor)
		{
			return RegisteredActor.TypeId == ActorType.TypeId ||
				   RegisteredActor.ClassName == ActorType.ClassName;
		});

	if (ExistingActor != ActorTypes.end())
	{
		*ExistingActor = std::move(ActorType);
		return;
	}

	ActorTypes.push_back(std::move(ActorType));
}

const std::vector<FActorTypeDefinition>& FActorTypeRegistry::GetActorTypes() const
{
	return ActorTypes;
}

const FActorTypeDefinition* FActorTypeRegistry::FindByTypeId(const std::string& TypeId) const
{
	const auto FoundActor = std::find_if(
		ActorTypes.begin(),
		ActorTypes.end(),
		[&TypeId](const FActorTypeDefinition& ActorType)
		{
			return ActorType.TypeId == TypeId;
		});

	return FoundActor == ActorTypes.end() ? nullptr : &*FoundActor;
}

const FActorTypeDefinition* FActorTypeRegistry::FindByClassName(const std::string& ClassName) const
{
	const auto FoundActor = std::find_if(
		ActorTypes.begin(),
		ActorTypes.end(),
		[&ClassName](const FActorTypeDefinition& ActorType)
		{
			return ActorType.ClassName == ClassName;
		});

	return FoundActor == ActorTypes.end() ? nullptr : &*FoundActor;
}

bool FActorTypeRegistry::IsManagedComponentName(const std::string& ComponentName) const
{
	for (const FActorTypeDefinition& ActorType : ActorTypes)
	{
		for (const FActorComponentSchema& Component : ActorType.Components)
		{
			if (Component.Name == ComponentName)
			{
				return true;
			}
		}
	}

	return false;
}

bool FActorTypeRegistry::IsComponentExpectedForClass(
	const std::string& ClassName,
	const std::string& ComponentName) const
{
	const FActorTypeDefinition* ActorType = FindByClassName(ClassName);
	if (!ActorType)
	{
		return false;
	}

	for (const FActorComponentSchema& Component : ActorType->Components)
	{
		if (Component.Name == ComponentName)
		{
			return true;
		}
	}

	return false;
}

void RegisterDefaultActorTypes(FActorTypeRegistry& Registry)
{
	Registry.RegisterActorType(ActorType(
		"Default.Box",
		"Box",
		"BoxObject",
		"Basic",
		SceneObjectKind::Box,
		EditorVec2{128.0f, 32.0f},
		FActorPreviewColor{78, 150, 204, 176},
		FActorPreviewColor{104, 184, 238, 230}));

	Registry.RegisterActorType(ActorType(
		"Default.Circle",
		"Circle",
		"CircleObject",
		"Basic",
		SceneObjectKind::Circle,
		EditorVec2{64.0f, 64.0f},
		FActorPreviewColor{225, 142, 72, 180},
		FActorPreviewColor{245, 168, 94, 230}));
}

bool LoadActorTypesFromFile(const std::filesystem::path& Path, FActorTypeRegistry& Registry, std::string* Error)
{
	std::ifstream File(Path);
	if (!File)
	{
		SetError(Error, "Could not open actor type schema: " + Path.string());
		return false;
	}

	std::string Magic;
	int Version = 0;
	File >> Magic >> Version;
	if (Magic != ActorTypesMagic || Version != ActorTypesVersion)
	{
		SetError(Error, "Unsupported actor type schema header: " + Path.string());
		return false;
	}

	std::string Line;
	std::getline(File, Line);
	SizeT LineNumber = 1;
	bool HasCurrentActor = false;
	int CurrentComponentIndex = -1;
	FActorTypeDefinition CurrentActor;

	while (std::getline(File, Line))
	{
		++LineNumber;
		Line = Trim(std::move(Line));
		if (Line.empty() || Line.front() == '#')
		{
			continue;
		}

		std::istringstream Stream(Line);
		std::string Command;
		Stream >> Command;

		if (Command == "actor")
		{
			if (HasCurrentActor)
			{
				SetError(Error, "Nested actor definition at line " + std::to_string(LineNumber));
				return false;
			}

			if (!ParseActorLine(Stream, CurrentActor, LineNumber, Error))
			{
				return false;
			}
			HasCurrentActor = true;
			CurrentComponentIndex = -1;
		}
		else if (Command == "component")
		{
			if (!HasCurrentActor)
			{
				SetError(Error, "Component outside actor at line " + std::to_string(LineNumber));
				return false;
			}

			std::string ComponentName;
			if (!(Stream >> std::quoted(ComponentName)))
			{
				SetError(Error, "Invalid actor component at line " + std::to_string(LineNumber));
				return false;
			}

			CurrentActor.Components.push_back(FActorComponentSchema{std::move(ComponentName), {}});
			CurrentComponentIndex = static_cast<int>(CurrentActor.Components.size()) - 1;
		}
		else if (Command == "property")
		{
			if (!HasCurrentActor || CurrentComponentIndex < 0)
			{
				SetError(Error, "Property outside component at line " + std::to_string(LineNumber));
				return false;
			}

			if (!ParsePropertyLine(Stream, CurrentActor.Components[static_cast<SizeT>(CurrentComponentIndex)], LineNumber, Error))
			{
				return false;
			}
		}
		else if (Command == "endcomponent")
		{
			CurrentComponentIndex = -1;
		}
		else if (Command == "endactor")
		{
			if (!HasCurrentActor)
			{
				SetError(Error, "endactor without actor at line " + std::to_string(LineNumber));
				return false;
			}

			Registry.RegisterActorType(std::move(CurrentActor));
			CurrentActor = FActorTypeDefinition{};
			HasCurrentActor = false;
			CurrentComponentIndex = -1;
		}
		else
		{
			SetError(Error, "Unknown actor schema command at line " + std::to_string(LineNumber) + ": " + Command);
			return false;
		}
	}

	if (HasCurrentActor)
	{
		Registry.RegisterActorType(std::move(CurrentActor));
	}

	if (Error)
	{
		Error->clear();
	}
	return true;
}

} // namespace AE::Editor
