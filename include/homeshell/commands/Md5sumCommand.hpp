/**
 * @file Md5sumCommand.hpp
 * @brief Compute MD5 checksums
 *
 * This command computes and verifies MD5 (128-bit) checksums,
 * similar to the Unix `md5sum` command.
 *
 * @author Homeshell Development Team
 * @date 2025
 */

#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>

#include <openssl/md5.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace homeshell
{

class Md5sumCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "md5sum";
    }

    std::string getDescription() const override
    {
        return "Compute MD5 checksums";
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
        bool check_mode = false;
        bool binary_mode = false;
        std::vector<std::string> files;

        for (const auto& arg : context.args)
        {
            if (arg == "-c" || arg == "--check")
            {
                check_mode = true;
            }
            else if (arg == "-b" || arg == "--binary")
            {
                binary_mode = true;
            }
            else if (arg[0] != '-')
            {
                files.push_back(arg);
            }
        }

        if (files.empty())
        {
            // Read from stdin
            std::string hash = computeHashFromStream(std::cin);
            std::cout << hash << "  -\n";
            return Status::ok();
        }

        // Process each file
        for (const auto& filename : files)
        {
            if (check_mode)
            {
                if (!verifyChecksums(filename))
                {
                    return Status::error("Checksum verification failed");
                }
            }
            else
            {
                std::string hash = computeHashFromFile(filename);
                if (hash.empty())
                {
                    std::cerr << "md5sum: " << filename << ": No such file or directory\n";
                    continue;
                }
                std::cout << hash << "  " << filename << "\n";
            }
        }

        return Status::ok();
    }

private:
    void showHelp() const
    {
        std::cout << "Usage: md5sum [OPTION]... [FILE]...\n\n"
                  << "Compute MD5 checksums.\n\n"
                  << "Options:\n"
                  << "  -c, --check     Read checksums from FILEs and verify them\n"
                  << "  -b, --binary    Read in binary mode\n"
                  << "  --help          Show this help message\n\n"
                  << "Examples:\n"
                  << "  md5sum file.txt\n"
                  << "  md5sum file1.txt file2.txt\n"
                  << "  echo 'hello' | md5sum\n"
                  << "  md5sum -c checksums.txt\n";
    }

    std::string computeHashFromFile(const std::string& filename) const
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file)
        {
            return "";
        }
        return computeHashFromStream(file);
    }

    std::string computeHashFromStream(std::istream& stream) const
    {
        MD5_CTX md5;
        MD5_Init(&md5);

        char buffer[8192];
        while (stream.read(buffer, sizeof(buffer)) || stream.gcount() > 0)
        {
            MD5_Update(&md5, buffer, stream.gcount());
        }

        unsigned char hash[MD5_DIGEST_LENGTH];
        MD5_Final(hash, &md5);

        std::stringstream ss;
        for (int i = 0; i < MD5_DIGEST_LENGTH; ++i)
        {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }

        return ss.str();
    }

    bool verifyChecksums(const std::string& checksum_file) const
    {
        std::ifstream file(checksum_file);
        if (!file)
        {
            std::cerr << "md5sum: " << checksum_file << ": No such file or directory\n";
            return false;
        }

        std::string line;
        bool all_ok = true;
        while (std::getline(file, line))
        {
            // Parse line: "hash  filename"
            size_t space_pos = line.find("  ");
            if (space_pos == std::string::npos)
            {
                continue;
            }

            std::string expected_hash = line.substr(0, space_pos);
            std::string filename = line.substr(space_pos + 2);

            std::string actual_hash = computeHashFromFile(filename);
            if (actual_hash.empty())
            {
                std::cout << filename << ": FAILED open or read\n";
                all_ok = false;
            }
            else if (actual_hash == expected_hash)
            {
                std::cout << filename << ": OK\n";
            }
            else
            {
                std::cout << filename << ": FAILED\n";
                all_ok = false;
            }
        }

        return all_ok;
    }
};

} // namespace homeshell
