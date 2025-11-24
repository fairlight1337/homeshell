/**
 * @file TarCommand.hpp
 * @brief Create and extract tar archives
 *
 * This command creates and extracts tar archives (simplified version).
 * Note: This is a basic implementation for demonstration purposes.
 *
 * @author Homeshell Development Team
 * @date 2025
 */

#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace homeshell
{

class TarCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "tar";
    }

    std::string getDescription() const override
    {
        return "Create and extract tar archives (basic implementation)";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    Status execute(const CommandContext& context) override
    {
        if (context.args.empty() || context.args[0] == "--help" || context.args[0] == "-h")
        {
            showHelp();
            return Status::ok();
        }

        // Parse options
        bool create = false;
        bool extract = false;
        bool list = false;
        bool verbose = false;
        std::string archive_file;
        std::vector<std::string> files;

        for (size_t i = 0; i < context.args.size(); ++i)
        {
            const auto& arg = context.args[i];
            if (arg == "-c" || arg.find('c') != std::string::npos)
            {
                create = true;
            }
            if (arg == "-x" || arg.find('x') != std::string::npos)
            {
                extract = true;
            }
            if (arg == "-t" || arg.find('t') != std::string::npos)
            {
                list = true;
            }
            if (arg == "-v" || arg.find('v') != std::string::npos)
            {
                verbose = true;
            }
            if ((arg == "-f" || arg.find('f') != std::string::npos) && i + 1 < context.args.size())
            {
                archive_file = context.args[++i];
            }
            else if (arg[0] != '-')
            {
                if (archive_file.empty())
                {
                    archive_file = arg;
                }
                else
                {
                    files.push_back(arg);
                }
            }
        }

        if (archive_file.empty())
        {
            std::cerr << "tar: archive file not specified\n";
            return Status::error("Missing archive file");
        }

        if (create)
        {
            return createArchive(archive_file, files, verbose);
        }
        else if (extract)
        {
            return extractArchive(archive_file, verbose);
        }
        else if (list)
        {
            return listArchive(archive_file);
        }

        std::cerr << "tar: must specify one of -c, -x, or -t\n";
        return Status::error("Missing operation");
    }

private:
    void showHelp() const
    {
        std::cout << "Usage: tar [OPTION]... [FILE]...\n\n"
                  << "Create and extract tar archives (basic implementation).\n\n"
                  << "Options:\n"
                  << "  -c              Create a new archive\n"
                  << "  -x              Extract files from archive\n"
                  << "  -t              List contents of archive\n"
                  << "  -f ARCHIVE      Use archive file ARCHIVE\n"
                  << "  -v              Verbose mode\n"
                  << "  --help          Show this help message\n\n"
                  << "Examples:\n"
                  << "  tar -cf archive.tar file1.txt file2.txt\n"
                  << "  tar -xf archive.tar\n"
                  << "  tar -tf archive.tar\n"
                  << "  tar -cvf archive.tar *.txt\n";
    }

    Status createArchive(const std::string& archive, const std::vector<std::string>& files,
                         bool verbose)
    {
        std::ofstream out(archive, std::ios::binary);
        if (!out)
        {
            std::cerr << "tar: cannot create archive: " << archive << "\n";
            return Status::error("Cannot create archive");
        }

        for (const auto& file : files)
        {
            if (!std::filesystem::exists(file))
            {
                std::cerr << "tar: " << file << ": No such file or directory\n";
                continue;
            }

            if (std::filesystem::is_directory(file))
            {
                std::cerr << "tar: " << file << ": Is a directory (skipping)\n";
                continue;
            }

            // Read file content
            std::ifstream in(file, std::ios::binary);
            if (!in)
            {
                std::cerr << "tar: cannot read: " << file << "\n";
                continue;
            }

            std::string content((std::istreambuf_iterator<char>(in)),
                                std::istreambuf_iterator<char>());

            // Write simple header: filename_length | filename | content_length | content
            uint32_t name_len = file.length();
            uint32_t content_len = content.length();

            out.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
            out.write(file.c_str(), name_len);
            out.write(reinterpret_cast<const char*>(&content_len), sizeof(content_len));
            out.write(content.c_str(), content_len);

            if (verbose)
            {
                std::cout << file << "\n";
            }
        }

        return Status::ok();
    }

    Status extractArchive(const std::string& archive, bool verbose)
    {
        std::ifstream in(archive, std::ios::binary);
        if (!in)
        {
            std::cerr << "tar: cannot open archive: " << archive << "\n";
            return Status::error("Cannot open archive");
        }

        while (in.peek() != EOF)
        {
            uint32_t name_len = 0;
            in.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
            if (in.gcount() != sizeof(name_len))
                break;

            std::string filename(name_len, '\0');
            in.read(&filename[0], name_len);

            uint32_t content_len = 0;
            in.read(reinterpret_cast<char*>(&content_len), sizeof(content_len));

            std::string content(content_len, '\0');
            in.read(&content[0], content_len);

            // Write extracted file
            std::ofstream out(filename, std::ios::binary);
            if (!out)
            {
                std::cerr << "tar: cannot create: " << filename << "\n";
                continue;
            }

            out.write(content.c_str(), content_len);

            if (verbose)
            {
                std::cout << filename << "\n";
            }
        }

        return Status::ok();
    }

    Status listArchive(const std::string& archive)
    {
        std::ifstream in(archive, std::ios::binary);
        if (!in)
        {
            std::cerr << "tar: cannot open archive: " << archive << "\n";
            return Status::error("Cannot open archive");
        }

        while (in.peek() != EOF)
        {
            uint32_t name_len = 0;
            in.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
            if (in.gcount() != sizeof(name_len))
                break;

            std::string filename(name_len, '\0');
            in.read(&filename[0], name_len);

            uint32_t content_len = 0;
            in.read(reinterpret_cast<char*>(&content_len), sizeof(content_len));

            // Skip content
            in.seekg(content_len, std::ios::cur);

            std::cout << filename << "\n";
        }

        return Status::ok();
    }
};

} // namespace homeshell
