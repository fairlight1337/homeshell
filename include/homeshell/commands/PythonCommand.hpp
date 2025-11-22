#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>
#include <homeshell/VirtualFilesystem.hpp>

#include <string>
#include <vector>

extern "C"
{
#include "port/micropython_embed.h"
}

namespace homeshell
{

class PythonCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "python";
    }

    std::string getDescription() const override
    {
        return "Execute Python code or run Python scripts (MicroPython)";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    Status execute(const CommandContext& context) override;

private:
    static bool micropython_initialized_;
    static char heap_[64 * 1024]; // 64 KB heap for MicroPython

    void initializeMicroPython();
    void deinitializeMicroPython();
    void runREPL();
    void executeString(const std::string& code);
    void executeFile(const std::string& filename);
};

} // namespace homeshell
