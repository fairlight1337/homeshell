#include <homeshell/VirtualFilesystem.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace homeshell
{

bool VirtualFilesystem::addMount(std::shared_ptr<EncryptedMount> mount)
{
    if (!mount)
    {
        return false;
    }

    mounts_[mount->getName()] = mount;
    return true;
}

bool VirtualFilesystem::removeMount(const std::string& name)
{
    auto it = mounts_.find(name);
    if (it != mounts_.end())
    {
        it->second->unmount();
        mounts_.erase(it);
        return true;
    }
    return false;
}

EncryptedMount* VirtualFilesystem::getMount(const std::string& name)
{
    auto it = mounts_.find(name);
    if (it != mounts_.end())
    {
        return it->second.get();
    }
    return nullptr;
}

std::vector<std::string> VirtualFilesystem::getMountNames() const
{
    std::vector<std::string> names;
    for (const auto& pair : mounts_)
    {
        names.push_back(pair.first);
    }
    return names;
}

ResolvedPath VirtualFilesystem::resolvePath(const std::string& path) const
{
    ResolvedPath result;
    std::string resolved = resolveRelativePath(path);

    // Check if path matches any mount point
    for (const auto& pair : mounts_)
    {
        const std::string& mount_point = pair.second->getMountPoint();

        if (resolved == mount_point || (resolved.length() > mount_point.length() &&
                                        resolved.substr(0, mount_point.length()) == mount_point &&
                                        resolved[mount_point.length()] == '/'))
        {
            result.type = PathType::Virtual;
            result.full_path = resolved;
            result.mount_point = mount_point;

            if (resolved == mount_point)
            {
                result.relative_path = "/";
            }
            else
            {
                result.relative_path = resolved.substr(mount_point.length());
            }

            result.mount = pair.second.get();
            return result;
        }
    }

    // Regular filesystem path
    result.type = PathType::Real;
    result.full_path = resolved;
    return result;
}

bool VirtualFilesystem::isVirtualPath(const std::string& path) const
{
    return resolvePath(path).type == PathType::Virtual;
}

std::string VirtualFilesystem::resolveRelativePath(const std::string& path) const
{
    if (path.empty())
    {
        return current_directory_;
    }

    if (path[0] == '/')
    {
        return path;
    }

    // Handle . (current directory)
    if (path == ".")
    {
        return current_directory_;
    }

    // Relative path - resolve against current directory
    std::string result = current_directory_;
    if (result.back() != '/')
    {
        result += "/";
    }
    result += path;

    // Normalize path (remove ., .., etc.)
    std::vector<std::string> parts;
    std::stringstream ss(result);
    std::string part;

    while (std::getline(ss, part, '/'))
    {
        if (part.empty() || part == ".")
        {
            continue; // Skip empty parts and "."
        }
        else if (part == "..")
        {
            if (!parts.empty())
            {
                parts.pop_back(); // Go up one directory
            }
        }
        else
        {
            parts.push_back(part);
        }
    }

    // Reconstruct path
    if (parts.empty())
    {
        return "/";
    }

    result = "";
    for (const auto& p : parts)
    {
        result += "/" + p;
    }

    return result;
}

std::vector<VirtualFileInfo> VirtualFilesystem::listDirectory(const std::string& path)
{
    ResolvedPath resolved = resolvePath(path);

    if (resolved.type == PathType::Virtual)
    {
        if (resolved.mount && resolved.mount->is_mounted())
        {
            return resolved.mount->listDirectory(resolved.relative_path);
        }
        return {};
    }
    else
    {
        // Real filesystem - convert from FilesystemHelper format
        auto real_entries = FilesystemHelper::listDirectory(resolved.full_path);
        std::vector<VirtualFileInfo> result;

        for (const auto& entry : real_entries)
        {
            VirtualFileInfo info;
            info.name = entry.name;
            info.path = resolved.full_path + "/" + entry.name;
            info.is_directory = entry.is_directory;
            info.size = entry.size;
            info.mtime = 0; // FilesystemHelper doesn't provide this
            result.push_back(info);
        }

        return result;
    }
}

bool VirtualFilesystem::changeDirectory(const std::string& path, std::string& result_path)
{
    ResolvedPath resolved = resolvePath(path);

    if (resolved.type == PathType::Virtual)
    {
        if (resolved.mount && resolved.mount->is_mounted())
        {
            if (resolved.mount->isDirectory(resolved.relative_path))
            {
                current_directory_ = resolved.full_path;
                result_path = current_directory_;
                return true;
            }
        }
        return false;
    }
    else
    {
        // Real filesystem
        if (FilesystemHelper::exists(resolved.full_path) &&
            FilesystemHelper::isDirectory(resolved.full_path))
        {
            FilesystemHelper::changeDirectory(resolved.full_path);
            current_directory_ = FilesystemHelper::getCurrentDirectory().string();
            result_path = current_directory_;
            return true;
        }
        return false;
    }
}

bool VirtualFilesystem::exists(const std::string& path)
{
    ResolvedPath resolved = resolvePath(path);

    if (resolved.type == PathType::Virtual)
    {
        return resolved.mount && resolved.mount->is_mounted() &&
               resolved.mount->exists(resolved.relative_path);
    }
    else
    {
        return FilesystemHelper::exists(resolved.full_path);
    }
}

bool VirtualFilesystem::isDirectory(const std::string& path)
{
    ResolvedPath resolved = resolvePath(path);

    if (resolved.type == PathType::Virtual)
    {
        return resolved.mount && resolved.mount->is_mounted() &&
               resolved.mount->isDirectory(resolved.relative_path);
    }
    else
    {
        return FilesystemHelper::isDirectory(resolved.full_path);
    }
}

bool VirtualFilesystem::readFile(const std::string& path, std::string& content)
{
    ResolvedPath resolved = resolvePath(path);

    if (resolved.type == PathType::Virtual)
    {
        return resolved.mount && resolved.mount->is_mounted() &&
               resolved.mount->readFile(resolved.relative_path, content);
    }
    else
    {
        // Real filesystem
        std::ifstream file(resolved.full_path, std::ios::binary);
        if (!file.is_open())
        {
            return false;
        }

        content.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        return true;
    }
}

bool VirtualFilesystem::writeFile(const std::string& path, const std::string& content)
{
    ResolvedPath resolved = resolvePath(path);

    if (resolved.type == PathType::Virtual)
    {
        return resolved.mount && resolved.mount->is_mounted() &&
               resolved.mount->writeFile(resolved.relative_path, content);
    }
    else
    {
        // Real filesystem
        std::ofstream file(resolved.full_path, std::ios::binary);
        if (!file.is_open())
        {
            return false;
        }

        file.write(content.data(), content.size());
        return file.good();
    }
}

bool VirtualFilesystem::createDirectory(const std::string& path)
{
    ResolvedPath resolved = resolvePath(path);

    if (resolved.type == PathType::Virtual)
    {
        return resolved.mount && resolved.mount->is_mounted() &&
               resolved.mount->createDirectory(resolved.relative_path);
    }
    else
    {
        // Real filesystem
        std::error_code ec;
        std::filesystem::create_directories(resolved.full_path, ec);
        return !ec;
    }
}

bool VirtualFilesystem::remove(const std::string& path)
{
    ResolvedPath resolved = resolvePath(path);

    if (resolved.type == PathType::Virtual)
    {
        return resolved.mount && resolved.mount->is_mounted() &&
               resolved.mount->remove(resolved.relative_path);
    }
    else
    {
        // Real filesystem
        std::error_code ec;
        std::filesystem::remove_all(resolved.full_path, ec);
        return !ec;
    }
}

} // namespace homeshell
