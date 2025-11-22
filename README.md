# Homeshell

A modern, interactive C++20 shell with advanced features including line editing, tab completion, color support, and asynchronous command execution.

## Features

### ⚡ Signal Handling & Cancellation
- **CTRL-C handling** - Gracefully cancels ongoing operations instead of killing the shell
- **Command cancellation** - Async commands can be interrupted safely
- **Non-blocking** - Shell remains responsive during long-running operations

### 🎨 Modern Terminal Support
- **Intelligent Color Detection** - Automatically detects terminal capabilities (8, 16, 256, or TrueColor)
- **UTF-8 & Emoji Support** - Full Unicode support with automatic detection
- **Graceful Degradation** - Works seamlessly in piped/non-TTY environments

### ⌨️ Interactive Line Editing
- **Tab Completion** - Smart command completion with prefix matching
- **Persistent History** - Commands saved to `~/.homeshell_history` (last 1000 commands)
- **History Navigation** - Use ↑/↓ arrows to browse previous commands
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

### 🔐 Encrypted Storage
- **SQLCipher Integration** - Secure block-level encryption using SQLCipher
- **Virtual Filesystem** - Mount encrypted storage as virtual paths within the shell
- **Password Protection** - AES-256 encryption with password-based key derivation
- **Automatic Growth** - Storage grows dynamically up to configured quotas
- **Auto-mounting** - Configure mounts to load automatically on startup
- **Full File Operations** - All filesystem commands work transparently with encrypted mounts

### 🖥️ System Information
- **Hardware Detection** - USB, PCI, and block device enumeration
- **Comprehensive Reports** - CPU, memory, GPU, disk, and network information
- **Real-time Monitoring** - Process listing with CPU and memory usage
- **Color-coded Output** - Visual indicators for system health and resource usage

### 📝 Output Redirection
- **Bash-compatible** - Support for `>`, `>>`, `2>`, `2>>`, `&>`, `&>>`
- **Flexible Routing** - Redirect stdout, stderr, or both
- **Virtual Filesystem** - Works with encrypted mounts and regular files
- **Color Stripping** - ANSI color codes automatically removed when redirecting to files

### 🐍 Embedded Python
- **MicroPython Integration** - Full Python interpreter embedded in the shell
- **No External Dependencies** - Python runtime statically linked into the binary
- **Interactive REPL** - Full-featured Python REPL with history and multiline editing
- **Script Execution** - Run `.py` files directly within the shell
- **Core Modules** - math, collections, json, re, hashlib, time, and more
- **Secure** - File I/O disabled for safety, focuses on computation and logic

### 📝 Text Editor
- **Built-in Editor** - nano-style text editor using ncurses
- **Full-screen Editing** - Comfortable editing experience
- **Keyboard Shortcuts** - Familiar Ctrl+O (save), Ctrl+X (exit), Ctrl+K (cut), Ctrl+U (paste)
- **Line Navigation** - Ctrl+A (line start), Ctrl+E (line end)
- **Works Everywhere** - Edits both regular files and encrypted virtual filesystem files

### 🔧 Executable Support
- **Direct Execution** - Run any executable file using its path (e.g., `./script.sh`)
- **Shell Scripts** - Execute bash, Python, or any shebang-based scripts
- **System Binaries** - Call system utilities with full paths
- **Permission Management** - Built-in `chmod` command (octal and symbolic modes)

### 📦 Static Linking
- **Minimal Dependencies** - Most libraries statically linked into the binary
- **Portable Binary** - Only requires glibc and libm (standard on all Linux systems)
- **Statically Linked:**
  - C++ standard library (libstdc++)
  - GCC runtime (libgcc)
  - Terminal UI (libncurses, libtinfo)
  - All third-party libraries (fmt, replxx, sqlcipher, miniz, micropython)
- **Dynamic Only:** Core system libraries (libc, libm, linux-vdso)
- **Binary Size:** ~12MB with all features included (including full Python interpreter!)

## Quick Start

### Clone & Build

```bash
# Clone the repository
git clone <repository-url>
cd homeshell

# Initialize submodules
git submodule update --init --recursive

# Build SQLCipher (required for encrypted storage)
cd external/sqlcipher
./configure CFLAGS="-DSQLITE_HAS_CODEC -DSQLITE_TEMP_STORE=2" LDFLAGS="-lcrypto"
make sqlite3.c
cd ../..

# Build MicroPython embed files (required for python command)
cd external
make -f micropython_build.mk
cd ..

# Build Homeshell
mkdir build
cd build
cmake ..
make -j$(nproc)

# Format code (optional)
make format
```

### Build Options

The build system supports several configuration options:

```bash
# Static linking (enabled by default)
cmake -DSTATIC_LINKING=ON ..

# Disable static linking for faster builds during development
cmake -DSTATIC_LINKING=OFF ..

# Disable tests
cmake -DBUILD_TESTING=OFF ..

# Check binary dependencies after building
ldd ./homeshell-linux
```

**Static Linking Details:**
- **Enabled by default** - The binary is built with maximum static linking
- **Minimal runtime dependencies** - Only requires `libc.so.6` and `libm.so.6`
- **Improved portability** - Binary works across different Linux distributions
- **No version conflicts** - Embedded libraries won't conflict with system versions
- **Larger binary size** - Trade-off for reduced dependencies (~12MB vs ~2MB)

### Setup for Ping Command (Optional)

The `ping` command uses raw ICMP sockets which require special privileges. Grant the necessary capability:

```bash
# Grant CAP_NET_RAW capability to the binary
sudo setcap cap_net_raw+ep ./homeshell-linux

# Verify the capability was set
getcap ./homeshell-linux
# Should output: ./homeshell-linux = cap_net_raw+ep
```

Without this capability, ping will show an error. All other commands work without special privileges.

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

homeshell> ping 127.0.0.1 3
PING 127.0.0.1 (3 packets):
Reply from 127.0.0.1: seq=1 time=0.05ms
Reply from 127.0.0.1: seq=2 time=0.04ms
Reply from 127.0.0.1: seq=3 time=0.06ms
(Press CTRL-C to cancel at any time)

homeshell> exit
```

### Tab Completion

Press **TAB** to autocomplete commands and file/directory names:

**Command completion:**
```
homeshell> ec<TAB>
homeshell> echo

homeshell> hel<TAB>
homeshell> help
```

**File/directory completion:**
```
homeshell> ls /ho<TAB>
homeshell> ls /home/

homeshell> cat myfi<TAB>
homeshell> cat myfile.txt

homeshell> cd /sec<TAB>
homeshell> cd /secure/
```

Tab completion works for:
- ✅ Commands (first word)
- ✅ Files and directories (subsequent words)
- ✅ Virtual encrypted filesystem paths
- ✅ Directories show trailing `/`

### Built-in Commands

| Command | Description | Type |
|---------|-------------|------|
| `help` | Display all available commands | Sync |
| `exit` | Exit the shell | Sync |
| `echo` | Echo arguments (supports emoji) | Sync |
| `pwd` | Print working directory | Sync |
| `ls` | List directory contents | Sync |
| `cd` | Change current directory | Sync |
| `cat` | Display file contents | Sync |
| `mkdir` | Create a directory | Sync |
| `touch` | Create or update a file | Sync |
| `rm` | Remove files or directories | Sync |
| `file` | Determine file type (magic detection) | Sync |
| `tree` | Display directory tree structure | Sync |
| `find` | Search for files and directories | Sync |
| `edit` | Edit a file (nano-style editor) | Sync |
| `zip` | Create zip archives | Sync |
| `unzip` | Extract zip archives | Sync |
| `zipinfo` | List zip archive contents | Sync |
| `mount` | Mount encrypted storage | Sync |
| `unmount` | Unmount encrypted storage | Sync |
| `vfs` | Show virtual filesystem info | Sync |
| `top` | Display running processes | Sync |
| `kill` | Send signal to a process | Sync |
| `lsusb` | List USB devices and controllers | Sync |
| `lspci` | List PCI devices | Sync |
| `lsblk` | List block devices and partitions | Sync |
| `sysinfo` | Show comprehensive system information | Sync |
| `datetime` | Show current date/time in ISO format | Sync |
| `chmod` | Change file permissions (octal/symbolic) | Sync |
| `version` | Show Homeshell and library versions | Sync |
| `python` | MicroPython interpreter and script executor | Sync |
| `ping` | Ping a host | Async |
| `sleep` | Sleep for N seconds (async demo) | Async |

**Note:** Homeshell can also execute external scripts and binaries using file paths (e.g., `./script.sh`, `/usr/bin/python3`, `../tools/mytool`). Any command containing a `/` is treated as an executable file path.

#### Filesystem Commands

**`ls [options] [path]`**
- List directory contents with color-coded output
- Directories shown in **blue**, symlinks in **cyan**
- Options:
  - `-l, --long` - Show detailed listing with permissions and sizes
  - `-h` - Human-readable file sizes (use with `-l`)
  - `--help` - Show help message
- Flags can be combined: `ls -lh` or `ls -hl`

**`cd [directory]`**
- Change current working directory
- `cd` with no arguments goes to home directory
- Supports relative and absolute paths

**`pwd`**
- Print the current working directory

**`file <path> [path2 ...]`**
- Determine file type using magic number detection
- Detects:  PNG, JPEG, GIF, PDF, ZIP, gzip, bzip2, ELF executables, SQLite databases, XML, HTML, JSON, scripts (with shebang), and more
- Color-coded output for different file types
- Works with both regular and virtual encrypted paths
- Example: `file document.pdf image.png /secure/data.json`

**`cat <file>`**
- Display the contents of a file
- Works with both regular and virtual encrypted paths
- Example: `cat myfile.txt` or `cat /secure/data.txt`

**`mkdir <directory>`**
- Create a new directory
- Works with both regular and virtual encrypted paths
- Example: `mkdir mydir` or `mkdir /secure/newdir`

**`touch <file>`**
- Create a new empty file or update timestamp
- Works with both regular and virtual encrypted paths
- Example: `touch newfile.txt` or `touch /secure/note.txt`

**`rm <path>`**
- Remove files or directories recursively
- **Warning**: This operation is permanent!
- Works with both regular and virtual encrypted paths
- Example: `rm oldfile.txt` or `rm /secure/tempdir`

**`tree [path]`**
- Display directory tree structure with visual formatting
- Shows directories in **blue bold**, files in normal text
- Recursively displays all subdirectories and files
- Displays summary with total directory and file count
- Example: `tree` or `tree /secure`

**`find [path] [options]`**
- Search for files and directories recursively
- Supports pattern matching with wildcards (`*` and `?`)
- Options:
  - `-name <pattern>` - Search for files matching pattern (case-sensitive)
  - `-iname <pattern>` - Case-insensitive name search
  - `-type <f|d>` - Filter by type: `f`=file, `d`=directory
  - `-maxdepth <n>` - Limit search depth (0=current dir only, 1=current + children, etc.)
  - `--help` - Show detailed help message
- Examples:
  - `find` - List all files recursively from current directory
  - `find /tmp` - List all files in /tmp
  - `find . -name test.txt` - Find files named exactly "test.txt"
  - `find . -iname readme.md` - Case-insensitive search for readme.md
  - `find . -type d` - Find all directories
  - `find . -type f` - Find all files (not directories)
  - `find /home -maxdepth 2` - Search up to 2 levels deep
  - `find . -name data.json -type f` - Combine multiple criteria
- **Note**: Pattern matching works with literal names in the current implementation
- Color-coded output: directories in blue, files in default color

**`edit <filename>`**
- Interactive text editor with nano-style interface
- Features:
  - **Full-screen editing** using ncurses
  - **Arrow key navigation** with Page Up/Down support
  - **Insert/delete** characters and lines
  - **Save/exit** with confirmation for unsaved changes
  - **Status bar** showing filename and modification status
  - **Keyboard shortcuts:**
    - `Ctrl-X` - Exit (prompts to save if modified)
    - `Ctrl-O` - Save file
    - `Ctrl-K` - Cut current line to clipboard
    - `Ctrl-U` - Paste line from clipboard
    - `Ctrl-A` - Jump to start of line
    - `Ctrl-E` - Jump to end of line
    - Arrow keys - Navigate (with auto-scrolling)
    - Home/End - Jump to start/end of line
    - Backspace/Delete - Remove characters
    - Enter - Insert new line
- Works with both regular and virtual filesystem paths
- Creates new files if they don't exist
- Example: `edit /secure/notes.txt`, `edit config.json`

#### Archive Commands

**`zip <archive.zip> <file1> [file2 ...]`**
- Create a ZIP archive containing the specified files and directories
- Automatically handles directory recursion
- Preserves directory structure by default
- Example: `zip backup.zip file1.txt dir1/ file2.txt`

**`unzip <archive.zip> [-o <destination>] [-j]`**
- Extract files from a ZIP archive
- Options:
  - `-o <destination>` - Extract to specified directory (default: current directory)
  - `-j` - Junk paths (don't preserve directory structure, extract all files to destination)
- Automatically creates necessary parent directories
- Example: `unzip backup.zip`, `unzip backup.zip -o /tmp/extracted`, `unzip backup.zip -j -o flat/`

**`zipinfo <archive.zip>`**
- List contents and structure of a ZIP archive
- Displays:
  - Compressed and uncompressed sizes
  - Compression ratio for each file
  - Directory structure
  - Total statistics
- Example: `zipinfo backup.zip`

#### System Commands

**`top`**
- Display running processes (Linux only)
- Shows PID, state, CPU usage, memory usage, and command
- Processes sorted by CPU usage (highest first)
- Displays top 25 processes
- Color-coded CPU usage:
  - **Red**: > 50% CPU
  - **Yellow**: 20-50% CPU  
  - **White**: < 20% CPU
- Example: `top`

**`kill [-s SIGNAL] <pid> [pid2 ...]`**
- Send signal to one or more processes
- Options:
  - `-s SIGNAL` - Specify signal to send (TERM, KILL, HUP, INT, QUIT, STOP, CONT)
  - `-SIGNAL` - Alternative format (e.g., `-9` for SIGKILL, `-KILL`)
- Default signal: SIGTERM (graceful termination)
- Example: `kill 1234`, `kill -9 1234`, `kill -s KILL 1234 5678`

**`lsusb`**
- List all USB devices and controllers (Linux only)
- Shows Bus, Device, Vendor ID, Product ID, and device names
- Color-coded output with speed information
- Example: `lsusb`

**`lspci`**
- List all PCI devices (Linux only)
- Shows slot, device class, vendor, and device info
- Color-coded device classes (Display, Network, Storage, etc.)
- Example: `lspci`

**`lsblk`**
- List block devices and partitions (Linux only)
- Shows device name, size, type, read-only status, removable status, filesystem type, and mount point
- Tree view showing partition hierarchy
- Color-coded by device type and usage percentage
- Example: `lsblk`

**`sysinfo`**
- Show comprehensive system information (Linux only)
- Displays:
  - **Operating System**: Name, hostname, kernel, architecture, distribution, uptime
  - **CPU**: Model, processor count, cores, current speed
  - **Memory**: Total/used/free RAM, swap usage with percentage
  - **System Load**: 1, 5, and 15-minute load averages
  - **Graphics**: GPU information, NVIDIA driver version
  - **Disks**: All mounted filesystems with usage and capacity
  - **Network**: All network interfaces with status, MAC addresses, IPv4/IPv6 addresses and subnet masks (CIDR notation)
  - **Display**: X11 or Wayland display server detection
- Color-coded status indicators (green/yellow/red based on usage)
- Example: `sysinfo`

**`datetime`**
- Show current date and time in ISO 8601 format
- Format: `YYYY-MM-DDTHH:MM:SS.mmm`
- Example output: `2025-11-21T14:30:25.123`

**`chmod <mode> <file>...`**
- Change file permissions
- Supports both octal and symbolic notation
- **Octal mode:**
  - `chmod 755 script.sh` - rwxr-xr-x (executable by all)
  - `chmod 0644 file.txt` - rw-r--r-- (readable by all, writable by owner)
- **Symbolic mode:**
  - `chmod +x script.sh` - Add execute permission for all
  - `chmod u+x file` - Add execute for user (owner)
  - `chmod g-w file` - Remove write for group
  - `chmod o=r file` - Set others to read-only
  - `chmod a+rw file` - Add read/write for all (user, group, others)
- Multiple files: `chmod +x script1.sh script2.sh`
- Works with real filesystem only (not virtual filesystem paths)

**`version`**
- Display Homeshell version and all embedded library versions
- Shows versions for:
  - Homeshell (version and build flavor)
  - MicroPython interpreter
  - SQLCipher (database encryption)
  - miniz (ZIP compression)
  - fmt (text formatting)
  - CLI11 (command-line parsing)
  - nlohmann/json (JSON parsing)
  - replxx (line editing)
- Useful for debugging and verifying the build
- Example: `version`

**`python [options] [file]`**
- Embedded MicroPython interpreter (no Python installation required!)
- **Interactive REPL:**
  - Run `python` with no arguments to start the REPL
  - Supports multiline editing for functions, loops, classes
  - Persistent history and line editing
  - Type `exit()` or press Ctrl-D to quit
- **Execute script:**
  - `python script.py` - Run a Python script file
  - `python -c "print('Hello!')"` - Execute Python code directly
- **Features:**
  - Python 3 syntax support
  - Built-in modules: math, collections, json, re, hashlib, time
  - No file I/O (security restriction)
  - Statically linked - no external Python installation needed
- **Limitations:**
  - No file system access (import from files disabled)
  - No external packages/pip
  - Limited to core Python language features
- Examples:
  ```bash
  # Start interactive Python REPL
  python
  
  # Execute a Python script
  python /tmp/calculate.py
  
  # Run inline Python code
  python -c "for i in range(5): print(f'Count: {i}')"
  ```

**`./path/to/script` or `/absolute/path/to/binary`**
- Execute external scripts and binaries directly
- Any command containing a `/` is treated as an executable file path
- The file must have execute permissions (use `chmod +x` if needed)
- Examples:
  - `./build_project.sh` - Run script in current directory
  - `../tools/deploy.sh` - Run script in parent directory
  - `/usr/bin/python3 script.py` - Run system Python
  - `/tmp/my_program` - Run binary from /tmp
- Arguments are passed through: `./script.sh arg1 arg2`
- Supports any shebang: `#!/bin/bash`, `#!/usr/bin/env python3`, etc.

**`ping <host> [count]`**
- Ping a network host using ICMP echo (async command)
- **Built-in ICMP implementation** - no external ping utility required
- Default count: 4 packets
- Example: `ping google.com 5`
- **Supports cancellation** with CTRL-C
- **Note**: Requires root privileges or `CAP_NET_RAW` capability on Linux
  - Run with `sudo` or set capability: `sudo setcap cap_net_raw+ep ./homeshell-linux`

### Output Redirection

Homeshell supports redirecting command output to files, similar to bash:

**Operators:**
- `>` - Redirect stdout to file (truncate)
- `>>` - Redirect stdout to file (append)
- `2>` - Redirect stderr to file (truncate)
- `2>>` - Redirect stderr to file (append)
- `&>` - Redirect both stdout and stderr to file (truncate)
- `&>>` - Redirect both stdout and stderr to file (append)

**Examples:**
```bash
# Redirect stdout to file (overwrite)
ls -l > files.txt

# Append stdout to file
echo "New line" >> log.txt

# Redirect stderr to file
command 2> errors.txt

# Redirect both stdout and stderr
sysinfo &> system_report.txt

# Append both stdout and stderr
command &>> full_log.txt
```

**Notes:**
- Output redirection works with all commands
- Files are created automatically if they don't exist
- Redirection is applied before command execution
- Works with both regular and virtual filesystem paths
- **ANSI color codes are automatically stripped when redirecting to files**
- Color output is preserved when displaying to terminal

## Configuration

Configuration is loaded from JSON files. Example (`config/homeshell.json`):

```json
{
  "prompt_format": "homeshell> "
}
```

### Configuration Options

- `prompt_format` - Custom prompt string with token support
- `history_file` - Path to command history file (default: `~/.homeshell_history`)
- `motd` - Message of the Day displayed on shell startup (supports `\n` for newlines)

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
  "prompt_format": "homeshell> ",
  "history_file": "~/.homeshell_history",
  "motd": "Welcome to Homeshell!\nType 'help' for available commands."
}
```

```json
{
  "prompt_format": "[%user%@%folder%]> ",
  "history_file": "~/.my_shell_history",
  "motd": "🏠 Home Automation Shell v1.0\nReady for commands!"
}
```

```json
{
  "prompt_format": "[%time% %path%]$ ",
  "history_file": "/tmp/homeshell_history",
  "motd": ""
}
```

**Notes:**
- To disable the MOTD, set it to an empty string `""`
- Use `\n` for line breaks in the MOTD
- Emoji support detection message is shown if terminal supports it (unless already in MOTD)

### Encrypted Storage Configuration

Homeshell supports encrypted virtual filesystems using SQLCipher. Configure mounts in your JSON config:

```json
{
  "prompt_format": "homeshell> ",
  "history_file": "~/.homeshell_history",
  "motd": "Welcome to Homeshell!",
  "encrypted_mounts": [
    {
      "name": "secure",
      "db_path": "~/.homeshell/secure.db",
      "mount_point": "/secure",
      "password": "",
      "max_size_mb": 100,
      "auto_mount": true
    }
  ]
}
```

**Password Options:**
1. **Empty password** (recommended): Prompts securely on startup
   ```json
   "password": ""
   ```

2. **Plaintext password**: Stored in config (less secure but convenient)
   ```json
   "password": "my-secure-password"
   ```

3. **Omit password field**: Also triggers secure prompt
   ```json
   {
     "name": "secure",
     "db_path": "~/.homeshell/secure.db",
     "mount_point": "/secure"
   }
   ```

#### Mount Configuration Options

- `name` - Unique name for the mount
- `db_path` - Path to the encrypted database file (supports `~` expansion)
- `mount_point` - Virtual path where the mount appears (e.g., `/secure`)
- `password` - Encryption password (AES-256 via SQLCipher) - **optional, prompts if empty**
- `max_size_mb` - Maximum storage size in MB (default: 100)
- `auto_mount` - Mount automatically on startup (default: true)

**Automatic Setup:**
- Parent directories are **automatically created** if they don't exist
- No need to manually create `~/.homeshell/` or other mount directories
- First mount creates and encrypts a new database file

**Password Security:**
- If `password` is empty or omitted in config, you'll be prompted interactively
- Password input is hidden (no echo to terminal)
- Passwords are **never stored in shell history**
- For non-interactive use, password must be in config or environment

#### Working with Encrypted Storage

Once mounted, encrypted storage appears as a virtual filesystem path. All filesystem commands work transparently:

```
homeshell> vfs
Virtual Filesystems:

secure
  Mount Point: /secure
  Database:    /home/user/.homeshell/secure.db
  Used:        1.25 MB / 100.00 MB (1.2%)

homeshell> cd /secure
homeshell> pwd
/secure

homeshell> touch secret.txt
File 'secret.txt' created

homeshell> ls
secret.txt

homeshell> echo "This is encrypted" > secret.txt
homeshell> cat secret.txt
This is encrypted

homeshell> mkdir data
Directory 'data' created

homeshell> cd data
homeshell> touch file1.txt file2.txt
homeshell> ls -l
-          0  file1.txt
-          0  file2.txt
```

#### Manual Mount/Unmount

You can also mount encrypted storage manually during a session:

```bash
# With password on command line (NOT RECOMMENDED - visible in history)
homeshell> mount backup ~/.homeshell/backup.db /backup mypassword 50
Successfully mounted 'backup' at '/backup'

# Secure: Password prompted (recommended)
homeshell> mount backup ~/.homeshell/backup.db /backup
Enter password for mount 'backup': ********
Successfully mounted 'backup' at '/backup'

# Unmount
homeshell> unmount backup
Successfully unmounted 'backup'
```

**Security Note:** Omitting the password triggers a secure prompt where your input is hidden. This is the **recommended approach** to avoid passwords appearing in shell history.

#### Security Features

- **AES-256 Encryption** - Industry-standard encryption via SQLCipher
- **Block-level Encryption** - Efficient partial updates without re-encrypting entire file
- **Password-based Key Derivation** - PBKDF2 with configurable iterations
- **No Cleartext Storage** - Data is encrypted at rest in the database
- **In-process Only** - Encrypted mounts are not accessible outside the shell
- **Automatic Growth** - Storage grows dynamically up to the configured quota

#### Storage Limits

- Each mount has a configurable maximum size (`max_size_mb`)
- Storage grows automatically as files are added
- When quota is reached, write operations will fail
- Use `vfs` command to monitor storage usage

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

### Cancellable Commands

Commands can support cancellation (CTRL-C handling):

```cpp
class MyCommand : public ICommand
{
public:
    MyCommand() : cancelled_(false) {}

    bool supportsCancellation() const override
    {
        return true;
    }

    void cancel() override
    {
        cancelled_.store(true);
    }

    Status execute(const CommandContext& context) override
    {
        while (!cancelled_.load())
        {
            // Do work, checking cancellation periodically
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        return Status::error("Cancelled");
    }

private:
    std::atomic<bool> cancelled_;
};
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
│   ├── IcmpPing.hpp            # Raw ICMP ping implementation
│   ├── PromptFormatter.hpp     # Prompt token replacement
│   ├── Shell.hpp               # Interactive shell REPL with signal handling
│   ├── Status.hpp              # Status/error handling
│   ├── TerminalInfo.hpp        # Terminal capability detection
│   ├── version.h.in            # Version template
│   └── commands/               # Built-in commands
│       ├── CdCommand.hpp       # Change directory
│       ├── DateTimeCommand.hpp # Show date/time
│       ├── EchoCommand.hpp     # Echo arguments
│       ├── ExitCommand.hpp     # Exit shell
│       ├── HelpCommand.hpp     # Show help
│       ├── LsCommand.hpp       # List directory
│       ├── PingCommand.hpp     # Ping network host
│       ├── PwdCommand.hpp      # Print working dir
│       └── SleepCommand.hpp    # Async demo
├── src/
│   ├── Homeshell.cpp
│   └── main.cpp
├── tests/                      # Unit tests
│   ├── CMakeLists.txt
│   ├── test_config.cpp
│   ├── test_filesystem_helper.cpp
│   ├── test_ls_command.cpp
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
- **[SQLCipher](https://github.com/sqlcipher/sqlcipher)** - Encrypted database storage
- **[miniz](https://github.com/richgel999/miniz)** - ZIP archive compression/decompression (built directly from source)
- **[MicroPython](https://github.com/micropython/micropython)** - Embedded Python 3 interpreter (statically linked)
- **[Google Test](https://github.com/google/googletest)** - Unit testing framework

**Note on miniz**: The miniz library is built directly from its source files without modifying the submodule. A compatibility header (`include/miniz_compat/miniz_export.h`) is provided to satisfy miniz's build requirements while keeping the submodule clean and reproducible.

### Build Artifacts

The build produces several artifacts:

- **`homeshell-linux`** - Main executable (statically linked, ~12MB)
- **`libhomeshell.a`** - Core library (can be used in other projects)
- **`homeshell_tests`** - Test executable (151 comprehensive tests)

### Deployment

Thanks to static linking, deploying Homeshell is straightforward:

```bash
# Simply copy the binary to your target system
scp build/homeshell-linux user@remote:/usr/local/bin/

# Or create a minimal package
tar czf homeshell-1.0.0.tar.gz homeshell-linux config/homeshell.json

# Verify it works on the target system (no library installation needed)
./homeshell-linux --version
./homeshell-linux --help
```

**Requirements on target system:**
- Linux kernel 3.2+ (for basic syscalls)
- glibc 2.17+ or compatible (libc.so.6, libm.so.6)
- No other dependencies required

**Advantages:**
- ✅ No dependency installation needed
- ✅ Works across different Linux distributions (Ubuntu, Debian, RHEL, Arch, etc.)
- ✅ No library version conflicts
- ✅ Single file deployment
- ✅ Consistent behavior across environments

## Development

### Build Targets

Homeshell provides several CMake targets for building, testing, documentation, and quality assurance.

#### Standard Build Targets

**Build Everything:**
```bash
cd build
make -j$(nproc)
```

**Build and Run Tests:**
```bash
cd build
make -j$(nproc)
make test
# Or with verbose output:
ctest --output-on-failure
```

**Clean Build:**
```bash
cd build
make clean
# Or for a complete rebuild:
rm -rf *
cmake ..
make -j$(nproc)
```

#### Documentation Targets

**Generate HTML Documentation (`make docs`):**

Generates comprehensive HTML documentation using Doxygen.

Prerequisites: `sudo apt-get install doxygen`

```bash
cd build
make docs
# View documentation:
firefox build/docs/html/index.html
```

Features:
- Full API documentation from Doxygen comments
- Class hierarchy diagrams
- Call graphs (if Graphviz dot is installed)
- Source code browsing with syntax highlighting
- Search functionality
- README.md integrated as main page

**Check Documentation Coverage (`make docs-coverage`):**

Identifies classes, functions, and parameters that lack Doxygen documentation.

```bash
cd build
make docs-coverage
# Report saved to: build/docs-coverage/undocumented.txt
```

Use cases:
- Track documentation progress
- Identify missing documentation before releases
- Ensure consistent documentation coverage
- Find undocumented public APIs

#### Code Coverage Targets

**Generate Code Coverage Report (`make coverage`):**

Generates detailed code coverage reports showing which lines are executed by tests.

Prerequisites: `sudo apt-get install lcov`

```bash
# 1. Configure with coverage enabled
cd build
cmake -DENABLE_COVERAGE=ON ..

# 2. Build with coverage instrumentation
make -j$(nproc)

# 3. Generate coverage report
make coverage

# 4. View report
firefox build/coverage/html/index.html
```

Output:
- Text summary: `build/coverage/coverage_summary.txt`
- HTML report: `build/coverage/html/index.html`
- Raw coverage data: `build/coverage/coverage.info`

Coverage report features:
- Line-by-line coverage visualization
- Function coverage statistics
- File-level coverage percentages
- Color-coded display (green = covered, red = not covered)
- Automatic exclusion of external libraries and system headers

**Coverage Exclusions:**
- System headers (`/usr/*`)
- External libraries (`*/external/*`)
- Test files (`*/tests/*`)
- Build artifacts (`*/build/*`)

**Interpreting Results:**
- **Line Coverage**: Percentage of executable lines that were run
- **Function Coverage**: Percentage of functions that were called
- Aim for >80% line coverage for critical code

#### Code Quality Targets

**Format Code (`make format`):**

Automatically formats all source files according to the project's style guide.

Prerequisites: clang-format

```bash
cd build
make format
```

**Summary of All Targets:**

| Target | Purpose | Prerequisites | Output Location |
|--------|---------|---------------|-----------------|
| `make` | Build all binaries | - | `build/` |
| `make test` | Run unit tests | - | Console |
| `make docs` | Generate documentation | doxygen | `build/docs/html/` |
| `make docs-coverage` | Check doc coverage | doxygen | `build/docs-coverage/` |
| `make coverage` | Generate coverage report | lcov, `-DENABLE_COVERAGE=ON` | `build/coverage/` |
| `make format` | Format source code | clang-format | In-place |
| `make clean` | Clean build artifacts | - | - |

### Running Tests

The project includes comprehensive unit tests using Google Test:

```bash
cd build
make
./tests/homeshell_tests

# Or use CTest
ctest --output-on-failure
```

**Test Suite Statistics:**
- **Total tests**: 201 tests
- **Test pass rate**: 100% ✅
- **Test execution time**: ~8.5 seconds

**Test Coverage Breakdown:**
- Status codes and error handling
- Configuration loading and parsing
- Filesystem operations (cross-platform)
- Prompt token replacement
- Command parsing and execution
- Archive operations (ZIP)
- Virtual filesystem and encryption
- Output redirection
- Python interpreter integration
- File permissions (chmod)
- Find command with pattern matching
- System information commands

**Overall Code Coverage:**
- **Line Coverage**: 80.1% (2014/2516 lines)
- **Function Coverage**: 88.9%
- Exceeds industry standard (70-80%)

**Coverage by Component:**

Excellent Coverage (>85%):
- VirtualFilesystem.cpp: 89.8%
- OutputRedirection.cpp: 88.6%
- EncryptedMount.cpp: 87.1%
- ChmodCommand.cpp: 85.0%

Good Coverage (70-85%):
- FindCommand.cpp: 78.1%
- PythonCommand.cpp: 72.3%

### Documentation Status

**Documentation Coverage:**
- **Classes documented**: 34+ classes with comprehensive Doxygen comments
- **Documentation completeness**: ~80%
- **Remaining undocumented items**: 32 warnings (down from 72)

**Documentation Quality:**

All documented classes include:
- ✅ Comprehensive class-level `@brief` and `@details`
- ✅ Feature lists and capabilities
- ✅ Command syntax with `@code` examples
- ✅ Usage examples with expected output
- ✅ Method-level documentation with `@param` and `@return`
- ✅ Platform-specific notes (`@note`)
- ✅ Struct member documentation with inline comments
- ✅ Enum value documentation

**Fully Documented Components:**
- Core interfaces (Command, Status, Shell)
- Filesystem system (VirtualFilesystem, EncryptedMount)
- Configuration system (Config, TerminalInfo)
- Archive commands (Zip, Unzip, ZipInfo)
- System commands (Lsusb, Lspci, Lsblk, Sysinfo)
- Utility commands (Find, Chmod, Python, Version, Edit)
- Helper classes (OutputRedirection, PromptFormatter, FilesystemHelper)
- Basic commands (Echo, Help, Exit, Sleep, Ls, Cd, Pwd, Cat, Mkdir, Rm, Touch)

**Documentation Style Guide:**
- Follows Doxygen standards with `@brief`, `@details`, `@param`, `@return`, `@throws`, `@note`
- Inline member documentation with `///` comments
- Complete usage examples for complex features
- Cross-references using Doxygen `@ref`
- Platform-specific code marked appropriately

### Code Style

- **C++ Standard**: C++20
- **Brace Style**: Allman (braces on new lines)
- **Indentation**: 4 spaces
- **Line Length**: 100 characters max
- **Documentation**: Doxygen-style comments for all public APIs
- **Testing**: Unit tests required for all new features
- **Coverage**: Maintain >80% line coverage

### Static Analysis

The project integrates clang-tidy for static analysis. Configuration is in `config/.clang-tidy`.

### Best Practices

**Before committing:**
```bash
make format
make test
```

**Before releasing:**
```bash
make docs
make docs-coverage  # Ensure <10 warnings
make coverage       # Ensure >80% coverage
```

**For contributors:**
- New features must include tests
- Public APIs must be documented with Doxygen comments
- Maintain or improve coverage percentage
- Follow the established code style

### CI/CD Integration

Example workflow for GitHub Actions:

```yaml
name: CI

on: [push, pull_request]

jobs:
  build-and-test:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v2
      with:
        submodules: recursive
    
    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y \
          doxygen \
          lcov \
          libsqlcipher-dev \
          libncurses-dev \
          libtinfo-dev
    
    - name: Build with coverage
      run: |
        mkdir build && cd build
        cmake -DENABLE_COVERAGE=ON ..
        make -j$(nproc)
    
    - name: Run tests
      run: |
        cd build
        ctest --output-on-failure
    
    - name: Generate coverage
      run: |
        cd build
        make coverage
    
    - name: Generate documentation
      run: |
        cd build
        make docs
    
    - name: Check documentation coverage
      run: |
        cd build
        make docs-coverage
    
    - name: Upload coverage to Codecov
      uses: codecov/codecov-action@v2
      with:
        files: build/coverage/coverage_filtered.info
```

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
