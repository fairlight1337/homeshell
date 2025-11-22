#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>
#include <homeshell/VirtualFilesystem.hpp>

#include <fmt/color.h>

#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace homeshell
{

/**
 * @brief Determine file type via magic number detection
 *
 * @details The `file` command identifies file types by examining file contents
 * using magic numbers (byte signatures), file structure analysis, and text heuristics.
 * Similar to the Unix `file` command but integrated into Homeshell.
 *
 * **Detection Methods:**
 * 1. **Magic Numbers** - Binary signatures in file headers
 * 2. **Text Analysis** - Character encoding and format patterns
 * 3. **Structure Detection** - JSON, XML, HTML document structure
 * 4. **Script Detection** - Shebang (#!) interpreter lines
 *
 * **Supported File Types:**
 *
 * **Images:**
 * - PNG, JPEG, GIF (via magic numbers)
 *
 * **Documents:**
 * - PDF, XML, HTML, JSON
 *
 * **Archives:**
 * - ZIP (including JAR, DOCX, etc.), gzip, bzip2
 *
 * **Executables:**
 * - ELF (Linux binaries)
 *
 * **Databases:**
 * - SQLite 3.x
 *
 * **Scripts:**
 * - Shell scripts, Python, Perl, Ruby (via shebang)
 *
 * **Text:**
 * - ASCII text, UTF-8 text
 *
 * **Usage:**
 * @code
 * file <path> [path2 ...]
 * @endcode
 *
 * **Parameters:**
 * - `path` - One or more file paths to analyze
 *
 * **Examples:**
 * @code
 * file document.pdf
 * # Output: document.pdf: PDF document
 *
 * file image.png script.py data.json
 * # Output:
 * # image.png: PNG image data
 * # script.py: script, #!/usr/bin/env python3
 * # data.json: JSON data
 *
 * file /secure/encrypted.db
 * # Output: /secure/encrypted.db: SQLite 3.x database
 *
 * file *
 * # Analyze all files in current directory
 * @endcode
 *
 * **Color Coding:**
 * - Blue: Directories
 * - Magenta: Images (PNG, JPEG, GIF)
 * - Cyan: Documents (PDF, JSON)
 * - Yellow: Archives (ZIP, gzip, bzip2)
 * - Green: Executables (ELF), structured text (XML, HTML)
 * - Red: Errors (file not found, cannot read)
 * - Default: Text files, unknown types
 *
 * **Detection Algorithm:**
 * 1. Check if file exists and is readable
 * 2. Identify directories immediately
 * 3. Check magic numbers for binary formats
 * 4. Analyze text vs binary (null bytes, non-printable chars)
 * 5. For text files, check shebang and structure
 * 6. Apply heuristics for JSON, XML, HTML
 * 7. Default to "ASCII text" or "data"
 *
 * **Error Handling:**
 * - Non-existent files: Shows error, continues with remaining files
 * - Unreadable files: Shows error, continues
 * - Empty files: Identified as "empty"
 * - Returns error status if any file failed
 *
 * @note Works with virtual filesystem paths
 * @note Magic number detection limited to first few bytes
 * @note Text detection examines first 512 bytes
 * @note Binary threshold: >30% non-printable characters
 */
class FileCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "file";
    }

    std::string getDescription() const override
    {
        return "Determine file type";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    Status execute(const CommandContext& context) override
    {
        if (context.args.empty())
        {
            fmt::print(fg(fmt::color::red), "Error: No file specified\n");
            fmt::print("Usage: file <path> [path2 ...]\n");
            return Status::error("No file specified");
        }

        auto& vfs = VirtualFilesystem::getInstance();
        bool all_success = true;

        for (const auto& path : context.args)
        {
            if (!vfs.exists(path))
            {
                fmt::print(
                    "{}: {}\n", path,
                    fmt::format(fg(fmt::color::red), "cannot open (No such file or directory)"));
                all_success = false;
                continue;
            }

            if (vfs.isDirectory(path))
            {
                fmt::print("{}: {}\n", path, fmt::format(fg(fmt::color::blue), "directory"));
                continue;
            }

            // Read file content
            std::string content;
            if (!vfs.readFile(path, content))
            {
                fmt::print("{}: {}\n", path, fmt::format(fg(fmt::color::red), "cannot read file"));
                all_success = false;
                continue;
            }

            std::string type = detectFileType(path, content);
            fmt::print("{}: {}\n", path, type);
        }

        return all_success ? Status::ok() : Status::error("Some files could not be processed");
    }

private:
    std::string detectFileType(const std::string& path, const std::string& content)
    {
        if (content.empty())
        {
            return "empty";
        }

        // Check for magic numbers (first few bytes)
        std::string magic = checkMagicNumbers(content);
        if (!magic.empty())
        {
            return magic;
        }

        // Check if it's text or binary
        if (isTextFile(content))
        {
            // Check for script types
            if (content.size() > 2 && content[0] == '#' && content[1] == '!')
            {
                size_t eol = content.find('\n');
                if (eol != std::string::npos)
                {
                    std::string shebang = content.substr(2, eol - 2);
                    return fmt::format("script, {}", trim(shebang));
                }
                return "script";
            }

            // Check for specific text formats
            std::string text_type = detectTextType(content);
            if (!text_type.empty())
            {
                return text_type;
            }

            return "ASCII text";
        }
        else
        {
            return "data";
        }
    }

    std::string checkMagicNumbers(const std::string& content)
    {
        if (content.size() < 4)
        {
            return "";
        }

        const unsigned char* bytes = reinterpret_cast<const unsigned char*>(content.data());

        // PNG
        if (content.size() >= 8 && bytes[0] == 0x89 && bytes[1] == 'P' && bytes[2] == 'N' &&
            bytes[3] == 'G')
        {
            return fmt::format(fg(fmt::color::magenta), "PNG image data");
        }

        // JPEG
        if (bytes[0] == 0xFF && bytes[1] == 0xD8 && bytes[2] == 0xFF)
        {
            return fmt::format(fg(fmt::color::magenta), "JPEG image data");
        }

        // GIF
        if (content.size() >= 6 && bytes[0] == 'G' && bytes[1] == 'I' && bytes[2] == 'F')
        {
            return fmt::format(fg(fmt::color::magenta), "GIF image data");
        }

        // PDF
        if (bytes[0] == '%' && bytes[1] == 'P' && bytes[2] == 'D' && bytes[3] == 'F')
        {
            return fmt::format(fg(fmt::color::cyan), "PDF document");
        }

        // ZIP/JAR/DOCX etc
        if (bytes[0] == 'P' && bytes[1] == 'K' && bytes[2] == 0x03 && bytes[3] == 0x04)
        {
            return fmt::format(fg(fmt::color::yellow), "Zip archive data");
        }

        // gzip
        if (bytes[0] == 0x1F && bytes[1] == 0x8B)
        {
            return fmt::format(fg(fmt::color::yellow), "gzip compressed data");
        }

        // bzip2
        if (content.size() >= 3 && bytes[0] == 'B' && bytes[1] == 'Z' && bytes[2] == 'h')
        {
            return fmt::format(fg(fmt::color::yellow), "bzip2 compressed data");
        }

        // ELF (Linux executables)
        if (bytes[0] == 0x7F && bytes[1] == 'E' && bytes[2] == 'L' && bytes[3] == 'F')
        {
            return fmt::format(fg(fmt::color::green), "ELF executable");
        }

        // SQLite
        if (content.size() >= 16 && std::memcmp(bytes, "SQLite format 3", 15) == 0)
        {
            return fmt::format(fg(fmt::color::cyan), "SQLite 3.x database");
        }

        return "";
    }

    std::string detectTextType(const std::string& content)
    {
        // Simple heuristics for common text formats
        if (content.find("<?xml") != std::string::npos ||
            content.find("<!DOCTYPE") != std::string::npos)
        {
            return fmt::format(fg(fmt::color::green), "XML document");
        }

        if (content.find("<!DOCTYPE html") != std::string::npos ||
            content.find("<html") != std::string::npos)
        {
            return fmt::format(fg(fmt::color::green), "HTML document");
        }

        // Check for JSON (simple check)
        size_t first_nonspace = content.find_first_not_of(" \t\n\r");
        if (first_nonspace != std::string::npos)
        {
            char first_char = content[first_nonspace];
            if (first_char == '{' || first_char == '[')
            {
                if (content.find("\":") != std::string::npos)
                {
                    return fmt::format(fg(fmt::color::cyan), "JSON data");
                }
            }
        }

        return "";
    }

    bool isTextFile(const std::string& content)
    {
        // Check first 512 bytes for binary characters
        size_t check_size = std::min(content.size(), size_t(512));
        int binary_count = 0;

        for (size_t i = 0; i < check_size; ++i)
        {
            unsigned char ch = content[i];

            // Null bytes are definitely binary
            if (ch == 0)
            {
                return false;
            }

            // Count non-printable characters (except common whitespace)
            if (ch < 32 && ch != '\t' && ch != '\n' && ch != '\r')
            {
                binary_count++;
            }
        }

        // If more than 30% of checked bytes are non-printable, consider it binary
        return binary_count < (check_size * 0.3);
    }

    std::string trim(const std::string& str)
    {
        size_t first = str.find_first_not_of(" \t\n\r");
        if (first == std::string::npos)
        {
            return "";
        }

        size_t last = str.find_last_not_of(" \t\n\r");
        return str.substr(first, last - first + 1);
    }
};

} // namespace homeshell
