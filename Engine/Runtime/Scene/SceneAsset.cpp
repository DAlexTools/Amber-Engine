#include "Scene/SceneAsset.h"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <utility>

namespace AE::Scene
{
namespace
{
    constexpr const char* SceneMagic = "AmberScene";
    constexpr int SceneVersion = 1;

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
}

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
    if (magic != SceneMagic || version != SceneVersion)
    {
        SetError(error, "Unsupported scene file header: " + path.string());
        return false;
    }

    std::string line;
    std::getline(file, line);

    Document loaded;
    std::size_t lineNumber = 1;
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

            if (!(stream
                >> object.transform.position.x
                >> object.transform.position.y
                >> object.transform.rotationDegrees
                >> object.transform.scale.x
                >> object.transform.scale.y
                >> object.size.x
                >> object.size.y) ||
                !ReadBoolToken(stream, object.visible) ||
                !ReadBoolToken(stream, object.locked))
            {
                SetError(error, "Invalid scene object transform at line " + std::to_string(lineNumber));
                return false;
            }

            loaded.objects.push_back(std::move(object));
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

}
