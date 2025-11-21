#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>
#include <homeshell/VirtualFilesystem.hpp>
#include <fmt/color.h>
#include <miniz.h>
#include <vector>
#include <string>
#include <filesystem>

namespace homeshell
{

class UnzipCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "unzip";
    }

    std::string getDescription() const override
    {
        return "Extract files from a zip archive";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    Status execute(const CommandContext& context) override
    {
        if (context.args.empty())
        {
            fmt::print(fg(fmt::color::red), "Error: No archive specified\n");
            fmt::print("Usage: unzip <archive.zip> [-o <destination>] [-j]\n");
            fmt::print("  -o <destination>  Extract to directory (default: .)\n");
            fmt::print("  -j                Junk paths (don't preserve directory structure)\n");
            return Status::error("No archive specified");
        }

        std::string archive_name = context.args[0];
        std::string dest_dir = ".";
        bool junk_paths = false;

        // Parse options
        for (size_t i = 1; i < context.args.size(); ++i)
        {
            if (context.args[i] == "-o" && i + 1 < context.args.size())
            {
                dest_dir = context.args[i + 1];
                i++;
            }
            else if (context.args[i] == "-j")
            {
                junk_paths = true;
            }
        }

        return extractZipArchive(archive_name, dest_dir, junk_paths);
    }

private:
    Status extractZipArchive(const std::string& archive_name, const std::string& dest_dir, bool junk_paths)
    {
        auto& vfs = VirtualFilesystem::getInstance();
        
        // Ensure destination directory exists
        if (!vfs.exists(dest_dir))
        {
            if (!vfs.createDirectory(dest_dir))
            {
                fmt::print(fg(fmt::color::red), "Error: Failed to create destination directory '{}'\n", dest_dir);
                return Status::error("Failed to create destination");
            }
        }

        mz_zip_archive zip;
        memset(&zip, 0, sizeof(zip));

        if (!mz_zip_reader_init_file(&zip, archive_name.c_str(), 0))
        {
            fmt::print(fg(fmt::color::red), "Error: Failed to open archive '{}'\n", archive_name);
            return Status::error("Failed to open archive");
        }

        int num_files = mz_zip_reader_get_num_files(&zip);
        int extracted_count = 0;
        bool all_success = true;

        fmt::print("Extracting {} file(s) from '{}'...\n", num_files, archive_name);

        for (int i = 0; i < num_files; ++i)
        {
            mz_zip_archive_file_stat file_stat;
            if (!mz_zip_reader_file_stat(&zip, i, &file_stat))
            {
                continue;
            }

            std::string filename = file_stat.m_filename;
            
            // Skip if it's a directory entry
            if (filename.back() == '/')
            {
                continue;
            }

            // Determine output path
            std::string output_path;
            if (junk_paths)
            {
                // Extract just the filename
                size_t last_slash = filename.find_last_of('/');
                std::string basename = (last_slash != std::string::npos) ? 
                                      filename.substr(last_slash + 1) : filename;
                output_path = dest_dir + "/" + basename;
            }
            else
            {
                // Preserve directory structure
                output_path = dest_dir + "/" + filename;
            }

            // Extract file
            size_t uncomp_size = file_stat.m_uncomp_size;
            void* p = mz_zip_reader_extract_to_heap(&zip, i, &uncomp_size, 0);
            
            if (!p)
            {
                fmt::print(fg(fmt::color::red), "Error: Failed to extract '{}'\n", filename);
                all_success = false;
                continue;
            }

            // Ensure parent directory exists
            if (!junk_paths)
            {
                std::filesystem::path output_fs_path(output_path);
                std::string parent_dir = output_fs_path.parent_path().string();
                if (!parent_dir.empty() && !vfs.exists(parent_dir))
                {
                    vfs.createDirectory(parent_dir);
                }
            }

            // Write extracted content
            std::string content(static_cast<char*>(p), uncomp_size);
            mz_free(p);

            if (vfs.writeFile(output_path, content))
            {
                fmt::print(fg(fmt::color::green), "  Extracted: {}\n", output_path);
                extracted_count++;
            }
            else
            {
                fmt::print(fg(fmt::color::red), "Error: Failed to write '{}'\n", output_path);
                all_success = false;
            }
        }

        mz_zip_reader_end(&zip);

        fmt::print("\n{} Extracted {} file(s)\n",
                   all_success ? "✓" : "⚠", extracted_count);

        return all_success ? Status::ok() : Status::error("Some files failed");
    }
};

} // namespace homeshell

