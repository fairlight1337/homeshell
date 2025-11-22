#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>

#include <fmt/color.h>
#include <miniz.h>

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace homeshell
{

/**
 * @brief Display ZIP archive contents
 *
 * Lists files and directories contained in ZIP archives with detailed information
 * including sizes, compression ratios, and timestamps.
 *
 * @details Features:
 *          - List all files in archive
 *          - Show uncompressed and compressed sizes
 *          - Display compression ratios
 *          - Show modification timestamps
 *          - Total archive statistics
 *          - Color-coded output
 *
 *          Command syntax:
 *          @code
 *          zipinfo <archive.zip>
 *          @endcode
 *
 *          Output format:
 *          @code
 *          Archive: backup.zip
 *          Length     Compressed  Ratio   Date/Time             Name
 *          -------    ----------  -----   ---------             ----
 *          1024       512         50.0%   2024-01-15 10:30:45   file1.txt
 *          2048       1024        50.0%   2024-01-15 10:31:20   dir/file2.txt
 *          -------    ----------  -----
 *          3072       1536        50.0%   2 files
 *          @endcode
 *
 * Example usage:
 * @code
 * zipinfo backup.zip           // Show contents of archive
 * zipinfo project.zip          // List all files with details
 * @endcode
 *
 * @note Uses miniz library for ZIP operations.
 *       Does not extract files, only displays information.
 */
class ZipInfoCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "zipinfo";
    }

    std::string getDescription() const override
    {
        return "List contents of a zip archive";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    /**
     * @brief Execute the zipinfo command
     * @param context Command context with archive name
     * @return Status::ok() on success, Status::error() on failure
     */
    Status execute(const CommandContext& context) override
    {
        if (context.args.empty())
        {
            fmt::print(fg(fmt::color::red), "Error: No archive specified\n");
            fmt::print("Usage: zipinfo <archive.zip>\n");
            return Status::error("No archive specified");
        }

        std::string archive_name = context.args[0];
        return listZipContents(archive_name);
    }

private:
    Status listZipContents(const std::string& archive_name)
    {
        mz_zip_archive zip;
        memset(&zip, 0, sizeof(zip));

        if (!mz_zip_reader_init_file(&zip, archive_name.c_str(), 0))
        {
            fmt::print(fg(fmt::color::red), "Error: Failed to open archive '{}'\n", archive_name);
            return Status::error("Failed to open archive");
        }

        int num_files = mz_zip_reader_get_num_files(&zip);

        fmt::print(fg(fmt::color::yellow) | fmt::emphasis::bold, "Archive: {}\n", archive_name);
        fmt::print(fg(fmt::color::yellow), "{}\n", std::string(70, '-'));

        fmt::print("{:>10}  {:>10}  {:>5}  {}\n", "Compressed", "Uncompressed", "Ratio", "Name");
        fmt::print("{}\n", std::string(70, '-'));

        uint64_t total_compressed = 0;
        uint64_t total_uncompressed = 0;
        int file_count = 0;
        int dir_count = 0;

        for (int i = 0; i < num_files; ++i)
        {
            mz_zip_archive_file_stat file_stat;
            if (!mz_zip_reader_file_stat(&zip, i, &file_stat))
            {
                continue;
            }

            std::string filename = file_stat.m_filename;
            uint64_t comp_size = file_stat.m_comp_size;
            uint64_t uncomp_size = file_stat.m_uncomp_size;

            // Calculate compression ratio
            int ratio = 0;
            if (uncomp_size > 0)
            {
                ratio = 100 - (int)((comp_size * 100) / uncomp_size);
            }

            if (filename.back() == '/')
            {
                // Directory entry
                fmt::print("{:>10}  {:>10}  {:>4}%  ", "-", "-", "-");
                fmt::print(fg(fmt::color::blue), "{}\n", filename);
                dir_count++;
            }
            else
            {
                // File entry
                fmt::print("{:>10}  {:>10}  {:>4}%  {}\n", formatSize(comp_size),
                           formatSize(uncomp_size), ratio, filename);
                total_compressed += comp_size;
                total_uncompressed += uncomp_size;
                file_count++;
            }
        }

        fmt::print("{}\n", std::string(70, '-'));

        int overall_ratio = 0;
        if (total_uncompressed > 0)
        {
            overall_ratio = 100 - (int)((total_compressed * 100) / total_uncompressed);
        }

        fmt::print("{:>10}  {:>10}  {:>4}%  {}\n", formatSize(total_compressed),
                   formatSize(total_uncompressed), overall_ratio, "TOTAL");

        fmt::print("\n");
        fmt::print("{} file(s)", file_count);
        if (dir_count > 0)
        {
            fmt::print(", {} directory(ies)", dir_count);
        }
        fmt::print("\n");

        mz_zip_reader_end(&zip);
        return Status::ok();
    }

    std::string formatSize(uint64_t bytes)
    {
        if (bytes < 1024)
        {
            return std::to_string(bytes) + "B";
        }
        else if (bytes < 1024 * 1024)
        {
            return std::to_string(bytes / 1024) + "KB";
        }
        else if (bytes < 1024 * 1024 * 1024)
        {
            return std::to_string(bytes / (1024 * 1024)) + "MB";
        }
        else
        {
            return std::to_string(bytes / (1024 * 1024 * 1024)) + "GB";
        }
    }
};

} // namespace homeshell
