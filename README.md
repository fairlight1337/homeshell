# Homeshell

[![CI Build](https://github.com/fairlight1337/homeshell/workflows/CI%20Build%20and%20Test/badge.svg)](https://github.com/fairlight1337/homeshell/actions)
[![codecov](https://codecov.io/gh/fairlight1337/homeshell/branch/main/graph/badge.svg)](https://codecov.io/gh/fairlight1337/homeshell)
[![Documentation](https://img.shields.io/badge/docs-doxygen-blue.svg)](https://fairlight1337.github.io/homeshell/docs/)
[![Coverage Report](https://img.shields.io/badge/coverage-report-brightgreen.svg)](https://fairlight1337.github.io/homeshell/coverage/)
[![License](https://img.shields.io/github/license/fairlight1337/homeshell)](LICENSE)
[![Release](https://img.shields.io/github/v/release/fairlight1337/homeshell)](https://github.com/fairlight1337/homeshell/releases)

**Your Portable, Self-Contained Command-Line Toolkit**

Homeshell is a modern, standalone shell designed to be carried with you—on a USB drive, portable SSD, or any removable media. Built with C++20, it's a fully self-contained environment that runs anywhere without installation or dependencies.

📚 [**Wiki Documentation**](https://github.com/fairlight1337/homeshell/wiki) | 🚀 [Usage Examples](https://github.com/fairlight1337/homeshell/wiki/Usage-Examples) | 📦 [Command Reference](https://github.com/fairlight1337/homeshell/wiki/Command-Reference) | 💾 [Download](https://github.com/fairlight1337/homeshell/releases)

## Philosophy

**Take Your Shell Everywhere**

Homeshell is designed as an ad-hoc, portable toolkit that you can carry with you and run on any system:

- 🔌 **USB Drive Ready** - Copy to a USB drive and run immediately
- 📦 **Self-Contained** - No installation required, no system dependencies
- 🌍 **Cross-Platform Goal** - Currently Linux, with Windows and macOS support planned
- 🚀 **Run Anywhere** - Works on any Linux system with glibc 2.17+ (most distributions since ~2013)
- 🔒 **Your Environment** - Encrypted storage, custom configs, Python scripts—all portable

Think of it as your personal command-line toolkit that travels with you, providing a consistent environment regardless of the host system.

## Key Features

- ⚡ **Interactive Line Editing** - Tab completion, persistent history, syntax highlighting, Emacs keybindings
- 🔐 **Encrypted Storage** - SQLCipher-based virtual filesystem with AES-256 encryption
- 🐍 **Embedded Python** - MicroPython interpreter with full REPL and script execution
- 📝 **Built-in Editor** - nano-style text editor (Ctrl+O to save, Ctrl+X to exit)
- 🖥️ **System Tools** - Hardware detection (USB, PCI, block devices), process monitoring, network utilities
- 📦 **Archive Support** - Create and extract ZIP archives with full directory structure preservation
- 🔍 **Fast File Search** - SQLite-backed `updatedb`/`locate` for instant file searches
- 🎨 **Smart Terminal** - Auto-detects color support (8/16/256/TrueColor) and UTF-8/emoji capabilities
- 📝 **Output Redirection** - Bash-compatible redirection (`>`, `>>`, `2>`, `2>>`, `&>`, `&>>`)
- 🛡️ **Signal Handling** - Graceful CTRL-C handling with command cancellation support

[**See all features →**](https://github.com/fairlight1337/homeshell/wiki/Home#key-features)

## Quick Start

```bash
# Clone with submodules
git clone --recursive <repository-url>
cd homeshell

# Build dependencies (SQLCipher + MicroPython)
cd external/sqlcipher
./configure CFLAGS="-DSQLITE_HAS_CODEC -DSQLITE_TEMP_STORE=2" LDFLAGS="-lcrypto"
make sqlite3.c
cd ..
make -f micropython_build.mk
cd ..

# Build Homeshell
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run
./homeshell-linux
```

📖 **Detailed build instructions:** [Installation Guide](https://github.com/fairlight1337/homeshell/wiki/Installation-Guide)

## Portable Usage (USB Drive)

```bash
# Copy to USB drive
cp build/homeshell-linux /media/usb/homeshell

# On any Linux system
/media/usb/homeshell

# Keep your environment portable
/media/usb/homeshell --config /media/usb/config.json
```

The ~12MB statically-linked binary includes everything: Python interpreter, encryption support, text editor, and all libraries. No installation needed on target systems.

📖 **Full portable setup guide:** [Portable Usage](https://github.com/fairlight1337/homeshell/wiki/Portable-Usage)

## Usage Examples

### Basic File Operations
```bash
# Navigate and list files
cd /home/user
ls -lh
pwd
tree -d 2  # Directory tree, 2 levels deep

# Search for files
updatedb --path /home/user
locate config.json
```

### Encrypted Storage
```bash
# Mount encrypted storage
mount /secure mypassword

# Work with encrypted files
cd /secure
edit secrets.txt      # Built-in editor
cat secrets.txt
ls -lh

# Unmount when done
unmount /secure
```

### Embedded Python
```bash
# Interactive Python REPL
python

# Run Python scripts
python calculate.py

# Or use shebang
chmod +x script.py
./script.py
```

### Archive Operations
```bash
# Create archives
zip backup.zip documents/
zip -r full-backup.zip /home/user

# Extract archives
unzip backup.zip
unzip backup.zip -o extracted/  # Custom output directory

# List archive contents
zipinfo backup.zip
```

📖 **More examples:** [Usage Examples Wiki](https://github.com/fairlight1337/homeshell/wiki/Usage-Examples)

## Command Reference

Homeshell provides a rich set of built-in commands organized by category:

**Filesystem:** `cd`, `pwd`, `ls`, `mkdir`, `rm`, `touch`, `cat`, `cp`, `mv`, `chmod`, `tree`, `find`, `locate`, `updatedb`

**Archives:** `zip`, `unzip`, `zipinfo`

**Text Editing:** `edit`

**Encrypted Storage:** `mount`, `unmount`, `vfs`

**System Info:** `lsusb`, `lspci`, `lsblk`, `sysinfo`, `top`, `kill`, `datetime`, `ping`, `file`

**Shell:** `history`, `clear`, `help`, `version`, `exit`

**Python:** `python` - Full MicroPython REPL and script execution

**Output Redirection:** All commands support `>`, `>>`, `2>`, `2>>`, `&>`, `&>>` for flexible output routing

📖 **Full command documentation:** [Command Reference Wiki](https://github.com/fairlight1337/homeshell/wiki/Command-Reference)

## Configuration

Homeshell uses JSON configuration files for customization:

```json
{
  "shell": {
    "prompt": "[{cyan}{user}{reset}@{yellow}{host}{reset} {blue}{cwd}{reset}]$ ",
    "history_file": "~/.homeshell_history",
    "history_size": 1000
  },
  "terminal": {
    "force_color": false,
    "color_mode": "auto"
  },
  "encrypted_mounts": [
    {
      "path": "/secure",
      "database": "~/.homeshell/secure.db",
      "quota_mb": 100,
      "auto_mount": false
    }
  ]
}
```

📖 **Configuration guide:** [Configuration Wiki](https://github.com/fairlight1337/homeshell/wiki/Configuration)
📖 **Encrypted storage setup:** [Encrypted Storage Wiki](https://github.com/fairlight1337/homeshell/wiki/Encrypted-Storage)

## Extending Homeshell

Add custom commands with just a few lines of C++:

```cpp
class MyCommand : public ICommand {
public:
    std::string getName() const override { return "mycommand"; }
    std::string getDescription() const override { return "My custom command"; }
    CommandType getType() const override { return CommandType::Synchronous; }
    
    Status execute(const CommandContext& context) override {
        fmt::print("Hello from my command!\n");
        return Status::ok();
    }
};

// Register in main.cpp
shell.registerCommand(std::make_unique<MyCommand>());
```

📖 **Developer guide:** [Extending Homeshell Wiki](https://github.com/fairlight1337/homeshell/wiki/Extending-Homeshell)

## Development

```bash
# Run tests
cd build
make test

# Generate coverage report
make coverage
open coverage/index.html

# Generate documentation
make docs
open docs/html/index.html

# Format code
make format
```

**Build Targets:**
- `make` - Build the main binary
- `make test` - Run all tests (245 tests, 81.4% line coverage, 94.4% function coverage)
- `make coverage` - Generate LCOV coverage report
- `make docs` - Generate Doxygen documentation
- `make format` - Format code with clang-format

📖 **Development guide:** [Development Wiki](https://github.com/fairlight1337/homeshell/wiki/Development-Guide)
📖 **Architecture details:** [Architecture Wiki](https://github.com/fairlight1337/homeshell/wiki/Architecture)

## Contributing

Contributions are welcome! To contribute:

1. **Fork** the repository
2. **Create a feature branch** (`git checkout -b feature/my-feature`)
3. **Match the current style** - Use consistent naming and formatting
4. **Format your code** - Run `make format` before committing
5. **Add tests** - Include unit tests for new functionality
6. **Document your code** - Add Doxygen comments to public APIs
7. **Open a Pull Request** - Describe your changes clearly

We value code quality, test coverage, and thorough documentation. Please ensure your PR maintains or improves the current standards.

## License

Homeshell is licensed under the **BSD 3-Clause License**.

```
Copyright (c) 2025, Jan Winkler
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

See [LICENSE](LICENSE) file for full license text.

## Links

- 📚 [**Wiki Documentation**](https://github.com/fairlight1337/homeshell/wiki) - Comprehensive guides and tutorials
- 🔧 [**API Documentation**](https://fairlight1337.github.io/homeshell/docs/) - Doxygen-generated API reference
- 📊 [**Coverage Report**](https://fairlight1337.github.io/homeshell/coverage/) - Code coverage analysis
- 💬 [**Discussions**](https://github.com/fairlight1337/homeshell/discussions) - Ask questions and share ideas
- 🐛 [**Issues**](https://github.com/fairlight1337/homeshell/issues) - Report bugs or request features
- 💾 [**Releases**](https://github.com/fairlight1337/homeshell/releases) - Download pre-built binaries

---

**Made with ❤️ for portable computing**
