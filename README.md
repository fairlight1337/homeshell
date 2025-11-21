# Homeshell

A modern, interactive C++20 shell with advanced features including line editing, tab completion, color support, and asynchronous command execution.

## Features

### 🎨 Modern Terminal Support
- **Intelligent Color Detection** - Automatically detects terminal capabilities (8, 16, 256, or TrueColor)
- **UTF-8 & Emoji Support** - Full Unicode support with automatic detection
- **Graceful Degradation** - Works seamlessly in piped/non-TTY environments

### ⌨️ Interactive Line Editing
- **Tab Completion** - Smart command completion with prefix matching
- **Command History** - Navigate previous commands with ↑/↓ arrows
- **Syntax Highlighting** - Commands highlighted in real-time
- **Emacs Keybindings** - Ctrl+A, Ctrl+E, Ctrl+W, and more

### 🔧 Extensible Command System
- **Registry Pattern** - Easy command registration and discovery
- **Sync/Async Support** - Commands can run synchronously or asynchronously
- **Type-safe Context** - Structured command arguments and options

### ⚙️ Configuration
- **JSON-based Config** - Flexible configuration via JSON files
- **Customizable Prompt** - Define your own prompt format
- **Sensible Defaults** - Works out of the box without configuration

## Quick Start

### Clone & Build

```bash
# Clone the repository
git clone <repository-url>
cd homeshell

# Initialize submodules
git submodule update --init --recursive

# Build
mkdir build
cd build
cmake ..
make

# Format code (optional)
make format
```

### Run

```bash
# Basic usage
./homeshell-linux

# With custom config
./homeshell-linux --config ../config/homeshell.json

# Execute a command and exit
./homeshell-linux -e "pwd"
./homeshell-linux --execute "ls -l"

# Show version
./homeshell-linux --version

# Verbose output
./homeshell-linux --verbose
```

## Usage

### Interactive Session

```
$ ./homeshell-linux
Welcome to Homeshell!
Type 'help' for available commands.

✨ Emoji support detected!

homeshell> help
Available commands:

  echo            - Echo the arguments
  exit            - Exit the shell
  help            - Show available commands
  sleep           - Sleep for N seconds (async demo)

homeshell> echo Hello World 🚀
Hello World 🚀

homeshell> pwd
/home/user/projects

homeshell> ls
build/
config/
src/
README.md

homeshell> ls -l
drwxrwxr-x     0 B  build/
drwxrwxr-x     0 B  config/
drwxrwxr-x     0 B  src/
-rw-rw-r--  7.5 KB  README.md

homeshell> cd src
homeshell> pwd
/home/user/projects/src

homeshell> exit
```

### Tab Completion

Press **TAB** to autocomplete commands:

```
homeshell> ec<TAB>
homeshell> echo

homeshell> hel<TAB>
homeshell> help
```

### Built-in Commands

| Command | Description | Type |
|---------|-------------|------|
| `help` | Display all available commands | Sync |
| `exit` | Exit the shell | Sync |
| `echo` | Echo arguments (supports emoji) | Sync |
| `pwd` | Print working directory | Sync |
| `ls` | List directory contents | Sync |
| `cd` | Change current directory | Sync |
| `sleep` | Sleep for N seconds (async demo) | Async |

#### Filesystem Commands

**`ls [options] [path]`**
- List directory contents with color-coded output
- Directories shown in **blue**, symlinks in **cyan**
- Options:
  - `-l, --long` - Show detailed listing with permissions and sizes

**`cd [directory]`**
- Change current working directory
- `cd` with no arguments goes to home directory
- Supports relative and absolute paths

**`pwd`**
- Print the current working directory

## Configuration

Configuration is loaded from JSON files. Example (`config/homeshell.json`):

```json
{
  "prompt_format": "homeshell> "
}
```

### Configuration Options

- `prompt_format` - Custom prompt string with token support

#### Prompt Tokens

The `prompt_format` supports the following tokens:

| Token | Description | Example Output |
|-------|-------------|----------------|
| `%user%` | Current username | `john` |
| `%path%` | Full current path | `/home/john/projects` |
| `%folder%` | Current folder name | `projects` |
| `%time%` | Current time (HH:MM:SS) | `14:30:25` |
| `%date%` | Current date (YYYY-MM-DD) | `2025-11-21` |

**Example configurations:**

```json
{
  "prompt_format": "homeshell> "
}
```

```json
{
  "prompt_format": "[%user%@%folder%]> "
}
```

```json
{
  "prompt_format": "[%time% %path%]$ "
}
```

## Cross-Platform Filesystem Support

Homeshell uses C++17's `<filesystem>` library for cross-platform filesystem operations. The `FilesystemHelper` class provides:

- **Directory navigation** - `cd`, `pwd`
- **Directory listing** - `ls` with color-coded output
- **Path operations** - Absolute paths, existence checks
- **Human-readable sizes** - Automatic formatting (B, KB, MB, GB, TB)
- **Permission display** - Unix-style permission strings

All filesystem operations work seamlessly on:
- ✅ Linux
- ✅ Windows
- ✅ macOS

### FilesystemHelper API

For custom commands needing filesystem access:

```cpp
#include <homeshell/FilesystemHelper.hpp>

// Get current directory
auto cwd = FilesystemHelper::getCurrentDirectory();

// Change directory
FilesystemHelper::changeDirectory("/path/to/dir");

// List directory contents
auto entries = FilesystemHelper::listDirectory(".");

// Check if path exists
bool exists = FilesystemHelper::exists("/some/path");

// Format file size
std::string size = FilesystemHelper::formatSize(1024000); // "1000.0 KB"
```

## Extending Homeshell

### Adding New Commands

1. Create a new command class implementing `ICommand`:

```cpp
#include <homeshell/Command.hpp>

class MyCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "mycommand";
    }

    std::string getDescription() const override
    {
        return "My custom command";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    Status execute(const CommandContext& context) override
    {
        // Your implementation here
        fmt::print("Hello from mycommand!\n");
        return Status::ok();
    }
};
```

2. Register the command in `main.cpp`:

```cpp
#include <homeshell/commands/MyCommand.hpp>

// In main()
auto& registry = CommandRegistry::getInstance();
registry.registerCommand(std::make_shared<MyCommand>());
```

### Asynchronous Commands

For long-running operations, use `CommandType::Asynchronous`:

```cpp
CommandType getType() const override
{
    return CommandType::Asynchronous;
}

Status execute(const CommandContext& context) override
{
    // This runs in a separate thread
    std::this_thread::sleep_for(std::chrono::seconds(5));
    fmt::print("Async task completed!\n");
    return Status::ok();
}
```

## Architecture

```
main.cpp
  ├─ TerminalInfo::detect()     → Detect terminal capabilities
  ├─ Config::load()             → Load JSON configuration
  ├─ CommandRegistry            → Register commands
  └─ Shell::run()               → REPL loop
      ├─ Replxx                 → Line editing & history
      ├─ Tab completion         → Command completion
      ├─ Syntax highlighting    → Real-time highlighting
      └─ Command execution
          ├─ Synchronous        → Direct execution
          └─ Asynchronous       → std::async execution
```

## Project Structure

```
homeshell/
├── CMakeLists.txt              # Build configuration
├── README.md                   # This file
├── config/
│   ├── .clang-format           # Code formatting rules
│   ├── .clang-tidy             # Static analysis rules
│   └── homeshell.json          # Default configuration
├── include/homeshell/
│   ├── Command.hpp             # Command interface & registry
│   ├── Config.hpp              # Configuration management
│   ├── FilesystemHelper.hpp    # Cross-platform filesystem ops
│   ├── PromptFormatter.hpp     # Prompt token replacement
│   ├── Shell.hpp               # Interactive shell REPL
│   ├── Status.hpp              # Status/error handling
│   ├── TerminalInfo.hpp        # Terminal capability detection
│   ├── version.h.in            # Version template
│   └── commands/               # Built-in commands
│       ├── CdCommand.hpp       # Change directory
│       ├── EchoCommand.hpp     # Echo arguments
│       ├── ExitCommand.hpp     # Exit shell
│       ├── HelpCommand.hpp     # Show help
│       ├── LsCommand.hpp       # List directory
│       ├── PwdCommand.hpp      # Print working dir
│       └── SleepCommand.hpp    # Async demo
├── src/
│   ├── Homeshell.cpp
│   └── main.cpp
├── tests/                      # Unit tests
│   ├── CMakeLists.txt
│   ├── test_config.cpp
│   ├── test_filesystem_helper.cpp
│   ├── test_prompt_formatter.cpp
│   └── test_status.cpp
└── external/                   # Git submodules
    ├── CLI11/                  # Command-line parsing
    ├── fmt/                    # Formatting & colors
    ├── googletest/             # Unit testing
    ├── json/                   # JSON parsing
    └── replxx/                 # Interactive line editing
```

## Dependencies

All dependencies are managed as git submodules:

- **[CLI11](https://github.com/CLIUtils/CLI11)** - Command-line argument parsing
- **[nlohmann/json](https://github.com/nlohmann/json)** - JSON configuration
- **[fmt](https://github.com/fmtlib/fmt)** - Modern formatting with colors
- **[Replxx](https://github.com/AmokHuginnsson/replxx)** - Interactive line editing
- **[Google Test](https://github.com/google/googletest)** - Unit testing framework

## Development

### Running Tests

The project includes comprehensive unit tests using Google Test:

```bash
cd build
make
./tests/homeshell_tests

# Or use CTest
ctest --output-on-failure
```

Test coverage includes:
- **Status** - Status codes and error handling
- **Config** - Configuration loading and parsing
- **FilesystemHelper** - Cross-platform filesystem operations
- **PromptFormatter** - Prompt token replacement

### Code Formatting

The project uses clang-format for consistent code style:

```bash
cd build
make format
```

### Code Style
- **C++ Standard**: C++20
- **Brace Style**: Allman (braces on new lines)
- **Indentation**: 4 spaces
- **Line Length**: 100 characters max

### Static Analysis

The project integrates clang-tidy for static analysis. Configuration is in `config/.clang-tidy`.

## Versioning

Version information is managed via CMake and accessible at runtime:

```cpp
Version version;
std::cout << version.getVersionString() << std::endl;
// Output: "1.0.0-stable"
```

Version components:
- `major` - Major version (uint32_t)
- `minor` - Minor version (uint32_t)
- `patch` - Patch version (uint32_t)
- `flavor` - Version flavor string (e.g., "stable", "beta")

## Future Enhancements

- [ ] Command aliases and shortcuts
- [ ] Shell scripts/macros
- [ ] Persistent command history
- [ ] Multiple concurrent async tasks
- [ ] Advanced output buffering for async commands
- [ ] Plugin system for dynamic command loading
- [ ] Command piping and redirection
- [ ] Environment variable support
- [ ] More filesystem commands (mkdir, rm, cp, mv, cat, etc.)
- [ ] File search and filtering
- [ ] Glob pattern support

## License

[Add your license here]

## Contributing

[Add contribution guidelines here]
