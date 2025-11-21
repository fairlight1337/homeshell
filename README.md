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

homeshell> sleep 2
Sleeping for 2 seconds...
Done sleeping!

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
| `sleep` | Sleep for N seconds | Async |

## Configuration

Configuration is loaded from JSON files. Example (`config/homeshell.json`):

```json
{
  "prompt_format": "homeshell> "
}
```

### Configuration Options

- `prompt_format` - Custom prompt string (supports emoji)

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
│   ├── Shell.hpp               # Interactive shell REPL
│   ├── Status.hpp              # Status/error handling
│   ├── TerminalInfo.hpp        # Terminal capability detection
│   ├── version.h.in            # Version template
│   └── commands/               # Built-in commands
│       ├── EchoCommand.hpp
│       ├── ExitCommand.hpp
│       ├── HelpCommand.hpp
│       └── SleepCommand.hpp
├── src/
│   ├── Homeshell.cpp
│   └── main.cpp
└── external/                   # Git submodules
    ├── CLI11/                  # Command-line parsing
    ├── fmt/                    # Formatting & colors
    ├── json/                   # JSON parsing
    └── replxx/                 # Interactive line editing
```

## Dependencies

All dependencies are managed as git submodules:

- **[CLI11](https://github.com/CLIUtils/CLI11)** - Command-line argument parsing
- **[nlohmann/json](https://github.com/nlohmann/json)** - JSON configuration
- **[fmt](https://github.com/fmtlib/fmt)** - Modern formatting with colors
- **[Replxx](https://github.com/AmokHuginnsson/replxx)** - Interactive line editing

## Development

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

## License

[Add your license here]

## Contributing

[Add contribution guidelines here]
