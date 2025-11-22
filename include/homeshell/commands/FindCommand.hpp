#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>

#include <string>
#include <vector>

namespace homeshell
{

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

    Status execute(const CommandContext& context) override;

private:
    enum class FileType
    {
        All,
        File,
        Directory
    };

    struct FindOptions
    {
        std::string start_path = ".";
        std::string name_pattern = "";
        FileType type = FileType::All;
        bool case_insensitive = false;
        int max_depth = -1; // -1 means unlimited
    };

    void findRecursive(const std::string& path, const FindOptions& options, int current_depth,
                       const CommandContext& context);
    bool matchesPattern(const std::string& filename, const std::string& pattern,
                        bool case_insensitive);
    std::string toLower(const std::string& str);
};

} // namespace homeshell
