#include <homeshell/commands/PythonCommand.hpp>

#include <fmt/color.h>
#include <fmt/core.h>

#include <cstring>
#include <fstream>
#include <replxx.hxx>
#include <sstream>

extern "C"
{
#include <stdio.h>
}

namespace homeshell
{

// Static members
bool PythonCommand::micropython_initialized_ = false;
char PythonCommand::heap_[64 * 1024];

// LCOV_EXCL_START - MicroPython initialization is difficult to test comprehensively
void PythonCommand::initializeMicroPython()
{
    if (!micropython_initialized_)
    {
        int stack_top;
        mp_embed_init(&heap_[0], sizeof(heap_), &stack_top);
        micropython_initialized_ = true;
    }
}

void PythonCommand::deinitializeMicroPython()
{
    if (micropython_initialized_)
    {
        mp_embed_deinit();
        micropython_initialized_ = false;
    }
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START - Interactive REPL is not testable in automated tests
void PythonCommand::runREPL()
{
    fmt::print("MicroPython REPL\n");
    fmt::print("Type 'exit()' or press Ctrl-D to exit\n\n");

    replxx::Replxx rx;
    rx.set_max_history_size(1000);
    rx.set_max_hint_rows(3);

    std::string multiline_buffer;
    bool in_multiline = false;

    while (true)
    {
        const char* prompt = in_multiline ? "... " : ">>> ";
        const char* line = rx.input(prompt);

        if (line == nullptr) // Ctrl-D
        {
            fmt::print("\n");
            break;
        }

        std::string line_str(line);

        // Check for exit commands
        if (line_str == "exit()" || line_str == "quit()")
        {
            break;
        }

        // Add to history if not empty
        if (!line_str.empty())
        {
            rx.history_add(line_str);
        }

        // Handle multiline input
        if (in_multiline)
        {
            if (line_str.empty())
            {
                // Execute accumulated multiline code
                try
                {
                    mp_embed_exec_str(multiline_buffer.c_str());
                }
                catch (...)
                {
                    // MicroPython will have printed the error
                }
                multiline_buffer.clear();
                in_multiline = false;
            }
            else
            {
                multiline_buffer += line_str + "\n";
            }
        }
        else
        {
            // Check if line starts a multiline block
            std::string trimmed = line_str;
            trimmed.erase(0, trimmed.find_first_not_of(" \t"));

            if (!trimmed.empty() && (trimmed.back() == ':' || trimmed.find("def ") == 0 ||
                                     trimmed.find("class ") == 0 || trimmed.find("if ") == 0 ||
                                     trimmed.find("for ") == 0 || trimmed.find("while ") == 0 ||
                                     trimmed.find("with ") == 0 || trimmed.find("try") == 0))
            {
                multiline_buffer = line_str + "\n";
                in_multiline = true;
            }
            else if (!line_str.empty())
            {
                // Execute single line
                try
                {
                    mp_embed_exec_str(line_str.c_str());
                }
                catch (...)
                {
                    // MicroPython will have printed the error
                }
            }
        }
    }
}
// LCOV_EXCL_STOP

void PythonCommand::executeString(const std::string& code)
{
    try
    {
        mp_embed_exec_str(code.c_str());
    }
    catch (...)
    {
        fmt::print(fg(fmt::color::red), "Error executing Python code\n");
    }
}

void PythonCommand::executeFile(const std::string& filename)
{
    // Try to read from VirtualFilesystem first
    auto& vfs = VirtualFilesystem::getInstance();
    auto resolved_path = vfs.resolvePath(filename);

    std::string content;
    bool loaded = false;

    // Try virtual filesystem
    if (vfs.exists(resolved_path.full_path))
    {
        if (vfs.readFile(resolved_path.full_path, content))
        {
            loaded = true;
        }
    }
    // Try regular filesystem
    else
    {
        std::ifstream file(filename);
        if (file.good())
        {
            std::stringstream buffer;
            buffer << file.rdbuf();
            content = buffer.str();
            loaded = true;
        }
    }

    if (!loaded)
    {
        fmt::print(fg(fmt::color::red), "Error: Could not read file '{}'\n", filename);
        return;
    }

    try
    {
        mp_embed_exec_str(content.c_str());
    }
    catch (...)
    {
        fmt::print(fg(fmt::color::red), "Error executing Python script\n");
    }
}

Status PythonCommand::execute(const CommandContext& context)
{
    initializeMicroPython();

    // Check for -c option (execute code string)
    if (!context.args.empty() && context.args[0] == "-c")
    {
        if (context.args.size() < 2)
        {
            fmt::print(fg(fmt::color::red), "Error: -c option requires code argument\n");
            return Status::error("Missing code argument");
        }

        std::string code = context.args[1];
        executeString(code);
        return Status::ok();
    }

    // Check for script file
    if (!context.args.empty())
    {
        std::string filename = context.args[0];
        executeFile(filename);
        return Status::ok();
    }

    // No arguments - run REPL
    runREPL();

    return Status::ok();
}

} // namespace homeshell
