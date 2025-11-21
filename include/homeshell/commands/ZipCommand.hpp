#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>
#include <homeshell/VirtualFilesystem.hpp>

#include <fmt/color.h>
#include <miniz.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace homeshell
{

class ZipCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "zip";
    }

    std::string getDescription() const override
    {
        return "Create a zip archive";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    Status execute(const CommandContext& context) override
    {
        if (context.args.size() < 2)
        {
            fmt::print(fg(fmt::color::red), "Error: Insufficient arguments\n");
            fmt::print("Usage: zip <archive.zip> <file1> [file2 ...]\n");
            return Status::error("Insufficient arguments");
        }

        std::string archive_name = context.args[0];
        std::vector<std::string> files(context.args.begin() + 1, context.args.end());

        return createZipArchive(archive_name, files);
    }

private:
    Status createZipArchive(const std::string& archive_name, const std::vector<std::string>& files)
    {
        auto& vfs = VirtualFilesystem::getInstance();
        mz_zip_archive zip;
        memset(&zip, 0, sizeof(zip));

        if (!mz_zip_writer_init_file(&zip, archive_name.c_str(), 0))
        {
            fmt::print(fg(fmt::color::red), "Error: Failed to create archive '{}'\n", archive_name);
            return Status::error("Failed to create archive");
        }

        bool all_success = true;
        int added_count = 0;

        for (const auto& file_path : files)
        {
            bool exists = vfs.isVirtualPath(file_path) ? vfs.exists(file_path)
                                                       : std::filesystem::exists(file_path);

            if (!exists)
            {
                fmt::print(fg(fmt::color::yellow), "Warning: '{}' not found, skipping\n",
                           file_path);
                all_success = false;
                continue;
            }

            bool is_dir = vfs.isVirtualPath(file_path) ? vfs.isDirectory(file_path)
                                                       : std::filesystem::is_directory(file_path);

            if (is_dir)
            {
                // Add directory recursively
                if (addDirectoryToZip(zip, file_path, file_path, added_count))
                {
                    fmt::print(fg(fmt::color::green), "Added directory: {}/\n", file_path);
                }
                else
                {
                    all_success = false;
                }
            }
            else
            {
                // Add single file
                if (addFileToZip(zip, file_path, file_path))
                {
                    added_count++;
                    fmt::print(fg(fmt::color::green), "Added: {}\n", file_path);
                }
                else
                {
                    all_success = false;
                }
            }
        }

        if (!mz_zip_writer_finalize_archive(&zip))
        {
            fmt::print(fg(fmt::color::red), "Error: Failed to finalize archive\n");
            mz_zip_writer_end(&zip);
            return Status::error("Failed to finalize");
        }

        mz_zip_writer_end(&zip);

        fmt::print("\n{} Created '{}' with {} file(s)\n", all_success ? "✓" : "⚠", archive_name,
                   added_count);

        return all_success ? Status::ok() : Status::error("Some files failed");
    }

    bool addFileToZip(mz_zip_archive& zip, const std::string& file_path,
                      const std::string& archive_name)
    {
        auto& vfs = VirtualFilesystem::getInstance();

        std::string content;

        // Try VFS first (for virtual paths), then fall back to regular filesystem
        if (vfs.isVirtualPath(file_path))
        {
            if (!vfs.readFile(file_path, content))
            {
                fmt::print(fg(fmt::color::red), "Error: Failed to read '{}'\n", file_path);
                return false;
            }
        }
        else
        {
            // Read regular file directly
            std::ifstream file(file_path, std::ios::binary);
            if (!file)
            {
                fmt::print(fg(fmt::color::red), "Error: Failed to read '{}'\n", file_path);
                return false;
            }
            content = std::string((std::istreambuf_iterator<char>(file)),
                                  std::istreambuf_iterator<char>());
        }

        if (!mz_zip_writer_add_mem(&zip, archive_name.c_str(), content.data(), content.size(),
                                   MZ_DEFAULT_COMPRESSION))
        {
            fmt::print(fg(fmt::color::red), "Error: Failed to add '{}' to archive\n", file_path);
            return false;
        }

        return true;
    }

    bool addDirectoryToZip(mz_zip_archive& zip, const std::string& dir_path,
                           const std::string& base_path, int& count)
    {
        auto& vfs = VirtualFilesystem::getInstance();

        if (vfs.isVirtualPath(dir_path))
        {
            // Handle virtual directory
            auto entries = vfs.listDirectory(dir_path);

            for (const auto& entry : entries)
            {
                std::string full_path = dir_path + "/" + entry.name;

                if (entry.is_directory)
                {
                    addDirectoryToZip(zip, full_path, base_path, count);
                }
                else
                {
                    std::string archive_name = full_path;
                    if (addFileToZip(zip, full_path, archive_name))
                    {
                        count++;
                    }
                }
            }
        }
        else
        {
            // Handle regular directory
            for (const auto& entry : std::filesystem::recursive_directory_iterator(dir_path))
            {
                if (entry.is_regular_file())
                {
                    std::string full_path = entry.path().string();
                    std::string archive_name = full_path;
                    if (addFileToZip(zip, full_path, archive_name))
                    {
                        count++;
                    }
                }
            }
        }

        return true;
    }
};

} // namespace homeshell
