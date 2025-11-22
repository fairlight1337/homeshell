#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>

#include <string>
#include <vector>

namespace homeshell
{

/**
 * @brief Change file permissions command
 *
 * Modifies file and directory permissions using either octal notation
 * (e.g., 755, 0644) or symbolic notation (e.g., +x, u+w, g-r).
 *
 * @details Usage: chmod <mode> <file>...
 *
 *          Octal modes:
 *          - chmod 755 file.sh  (rwxr-xr-x)
 *          - chmod 0644 file.txt (rw-r--r--)
 *
 *          Symbolic modes:
 *          - chmod +x file      (add execute for all)
 *          - chmod u+w file     (add write for user)
 *          - chmod g-r file     (remove read for group)
 *          - chmod o=r file     (set others to read-only)
 *
 * Example: chmod +x script.sh
 */
class ChmodCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "chmod";
    }

    std::string getDescription() const override
    {
        return "Change file permissions";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    /**
     * @brief Execute the chmod command
     * @param context Command context with mode and file arguments
     * @return Status indicating success/failure
     */
    Status execute(const CommandContext& context) override;

private:
    /**
     * @brief Parse octal mode string (e.g., "755", "0644")
     * @param mode_str Mode string to parse
     * @param[out] mode Parsed mode value
     * @return true if parsing successful, false otherwise
     */
    bool parseOctalMode(const std::string& mode_str, mode_t& mode);

    /**
     * @brief Parse symbolic mode string (e.g., "+x", "u+w", "g-r")
     * @param mode_str Mode string to parse
     * @param current_mode Current file permissions
     * @param[out] new_mode Calculated new permissions
     * @return true if parsing successful, false otherwise
     */
    bool parseSymbolicMode(const std::string& mode_str, mode_t current_mode, mode_t& new_mode);

    /**
     * @brief Apply chmod to a file
     * @param path File path
     * @param mode New permissions mode
     * @return true if successful, false otherwise
     */
    bool applyChmod(const std::string& path, mode_t mode);
};

} // namespace homeshell
