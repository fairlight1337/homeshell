#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>

#include <string>
#include <vector>

namespace homeshell
{

/**
 * @brief Find command - search for files and directories
 *
 * Recursively searches the filesystem for files and directories matching
 * specified criteria. Supports pattern matching, type filtering, and depth
 * limiting similar to the Unix find command.
 *
 * @details Usage: find [path] [options]
 *          Options:
 *          - -name <pattern>: Match files by name (literal matching)
 *          - -iname <pattern>: Case-insensitive name matching
 *          - -type <f|d>: Filter by type (f=file, d=directory)
 *          - -maxdepth <n>: Limit recursion depth
 *
 * Example: find /tmp -name "test.txt" -type f -maxdepth 2
 */
class FindCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "find";
    }

    std::string getDescription() const override
    {
        return "Search for files and directories";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    /**
     * @brief Execute the find command
     * @param context Command context with search parameters
     * @return Status indicating success/failure
     */
    Status execute(const CommandContext& context) override;

private:
    /**
     * @brief Type filter for search results
     */
    enum class FileType
    {
        All,      ///< Include both files and directories
        File,     ///< Only regular files
        Directory ///< Only directories
    };

    /**
     * @brief Search options parsed from command line
     */
    struct FindOptions
    {
        std::string start_path = ".";  ///< Starting directory for search
        std::string name_pattern = ""; ///< Name pattern to match (empty = match all)
        FileType type = FileType::All; ///< Type filter
        bool case_insensitive = false; ///< Case-insensitive matching
        int max_depth = -1;            ///< Maximum recursion depth (-1 = unlimited)
    };

    /**
     * @brief Recursively search a directory
     * @param path Current directory path
     * @param options Search criteria
     * @param current_depth Current recursion depth (0 = start path)
     * @param context Command context for output formatting
     */
    void findRecursive(const std::string& path, const FindOptions& options, int current_depth,
                       const CommandContext& context);

    /**
     * @brief Check if a filename matches the pattern
     * @param filename File name to test
     * @param pattern Pattern to match against (literal matching)
     * @param case_insensitive Whether to perform case-insensitive matching
     * @return true if filename matches pattern, false otherwise
     */
    bool matchesPattern(const std::string& filename, const std::string& pattern,
                        bool case_insensitive);

    /**
     * @brief Convert string to lowercase
     * @param str Input string
     * @return Lowercase version of the string
     */
    std::string toLower(const std::string& str);
};

} // namespace homeshell
