/**
 * @file Base64Command.hpp
 * @brief Encode or decode base64 data
 *
 * This command encodes or decodes data using base64 encoding,
 * similar to the Unix `base64` command.
 *
 * @author Homeshell Development Team
 * @date 2025
 */

#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>

#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace homeshell
{

class Base64Command : public ICommand
{
public:
    std::string getName() const override
    {
        return "base64";
    }

    std::string getDescription() const override
    {
        return "Encode or decode base64 data";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    Status execute(const CommandContext& context) override
    {
        if (context.args.size() > 0 && (context.args[0] == "--help" || context.args[0] == "-h"))
        {
            showHelp();
            return Status::ok();
        }

        // Parse options
        bool decode = false;
        bool wrap = true;
        std::vector<std::string> files;

        for (size_t i = 0; i < context.args.size(); ++i)
        {
            const auto& arg = context.args[i];
            if (arg == "-d" || arg == "--decode")
            {
                decode = true;
            }
            else if (arg == "-w" || arg == "--wrap")
            {
                if (i + 1 < context.args.size())
                {
                    int wrap_cols = std::stoi(context.args[++i]);
                    wrap = (wrap_cols > 0);
                }
            }
            else if (arg[0] != '-')
            {
                files.push_back(arg);
            }
        }

        // Process input
        if (files.empty())
        {
            // Read from stdin
            std::string input;
            std::string line;
            while (std::getline(std::cin, line))
            {
                input += line;
                if (!decode)
                {
                    input += "\n";
                }
            }

            std::string output = decode ? base64Decode(input) : base64Encode(input);
            std::cout << output;
            if (!decode || !output.empty())
            {
                std::cout << "\n";
            }
        }
        else
        {
            // Process each file
            for (const auto& filename : files)
            {
                std::ifstream file(filename, std::ios::binary);
                if (!file)
                {
                    std::cerr << "base64: " << filename << ": No such file or directory\n";
                    continue;
                }

                std::string input((std::istreambuf_iterator<char>(file)),
                                  std::istreambuf_iterator<char>());

                std::string output = decode ? base64Decode(input) : base64Encode(input);
                std::cout << output;
                if (!decode || !output.empty())
                {
                    std::cout << "\n";
                }
            }
        }

        return Status::ok();
    }

private:
    void showHelp() const
    {
        std::cout << "Usage: base64 [OPTION]... [FILE]\n\n"
                  << "Encode or decode base64 data.\n\n"
                  << "Options:\n"
                  << "  -d, --decode    Decode data\n"
                  << "  -w, --wrap=COLS Wrap encoded lines after COLS characters (default 76)\n"
                  << "  --help          Show this help message\n\n"
                  << "Examples:\n"
                  << "  echo 'hello' | base64\n"
                  << "  base64 file.txt\n"
                  << "  echo 'aGVsbG8K' | base64 -d\n"
                  << "  base64 -d encoded.txt\n";
    }

    std::string base64Encode(const std::string& input) const
    {
        if (input.empty())
        {
            return "";
        }

        BIO* bio = BIO_new(BIO_s_mem());
        BIO* b64 = BIO_new(BIO_f_base64());
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL); // No newlines
        bio = BIO_push(b64, bio);

        BIO_write(bio, input.c_str(), input.length());
        BIO_flush(bio);

        BUF_MEM* bufferPtr;
        BIO_get_mem_ptr(bio, &bufferPtr);

        std::string output(bufferPtr->data, bufferPtr->length);
        BIO_free_all(bio);

        return output;
    }

    std::string base64Decode(const std::string& input) const
    {
        if (input.empty())
        {
            return "";
        }

        std::string clean_input = input;
        // Remove newlines
        clean_input.erase(std::remove(clean_input.begin(), clean_input.end(), '\n'),
                          clean_input.end());
        clean_input.erase(std::remove(clean_input.begin(), clean_input.end(), '\r'),
                          clean_input.end());

        BIO* bio = BIO_new_mem_buf(clean_input.c_str(), clean_input.length());
        BIO* b64 = BIO_new(BIO_f_base64());
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
        bio = BIO_push(b64, bio);

        std::vector<char> buffer(clean_input.length());
        int decoded_length = BIO_read(bio, buffer.data(), buffer.size());
        BIO_free_all(bio);

        if (decoded_length < 0)
        {
            return "";
        }

        return std::string(buffer.data(), decoded_length);
    }
};

} // namespace homeshell
