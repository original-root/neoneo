# NeoNeo

A powerful interactive Ollama chat client with tool support written in C++.

## Features

- Interactive CLI chat interface with streaming responses
- Support for all Ollama models
- Rich tool integrations for enhanced capabilities:
  - Advanced calculator for mathematical expressions
  - Shell command execution with safety controls
  - File operations (read, write, edit)
  - Model listing
- Comprehensive security features with configurable safeguards
- Conversation context management
- Command history with readline support
- User-friendly configuration system
- Single-key confirmations for security-sensitive operations

## Requirements

- C++17 compiler
- CMake 3.15+
- libcurl
- readline
- Ollama server running (locally or remotely)
- `bc` calculator (for the calculator tool)

## Build Instructions

### Clone and Build

```bash
git clone https://github.com/yourusername/neoneo.git
cd neoneo
mkdir build
cd build
cmake ..
make
```

### Install (Optional)

```bash
sudo make install
```

## Basic Usage

```
Usage: neoneo [options] [model]
Options:
  -h, --help          Show this help message
  -m, --model MODEL   Specify the model to use (default: llama3)
  -l, --list          List available models
  -t, --tools         Enable tool use with the model
  -d, --debug         Enable debug mode for detailed output
  -f, --file-ops      Enable file operations (read, write, edit)
  -s, --shell         Enable shell command execution tool (use with caution)
  --host URL          Specify Ollama host URL (default: http://localhost:11434)
```

## Tool Options

```
  --auto-confirm      Automatically confirm shell commands without prompting
  --auto-confirm-files  Automatically confirm file operations without prompting
  --ignore-calc-safety Ignore calculator safety checks for potentially dangerous patterns
  --ignore-shell-safety Ignore shell command safety checks for potentially dangerous operations
  --model-list        Enable model listing tool for the LLM
```

## Configuration Options

```
  --config FILE       Use specified config file (default: ~/.config/neoneo/config.json)
  --save-config       Save current settings to config file
  --no-config         Ignore config file and use default settings
```

## Examples

```bash
# Start basic chat with default model
neoneo

# Use a specific model
neoneo -m llama3.3

# Enable tools and debug mode
neoneo -d -t

# Enable tools with file operations
neoneo -t -f

# Enable tools with shell command execution (requires confirmation)
neoneo -t -s

# Enable automatic confirmation for shell commands (use with caution)
neoneo -t -s --auto-confirm

# Enable all tools with auto-confirmation
neoneo -t -f -s --auto-confirm --auto-confirm-files

# Save your preferred configuration
neoneo -t -f -s --model-list --save-config

# List available models
neoneo -l
```

## Chat Commands

During chat, you can use the following commands:

- `/exit` or `/quit` - Exit the program
- `/reset` - Reset the conversation history
- `/help` - Show available commands
- `/models` - List available models on the Ollama server
- `/config` - Show current configuration
- `/template` - Show the conversation template being sent to the LLM
- `/tools` - List available tools (when tools are enabled)

## Available Tools

When tools are enabled, the model can utilize various capabilities:

1. **Calculator**: Evaluate mathematical expressions using the bc calculator
   - Supports basic operations (+, -, *, /), exponents, functions (sqrt, sin, cos, etc.)
   - Includes safety checks to prevent command injection

2. **Shell Command Execution**: Run system commands and capture output 
   - Includes safety checks for potentially dangerous operations
   - Requires confirmation for each command (unless auto-confirm is enabled)
   - Supports timeout parameter to prevent long-running commands

3. **File Operations**:
   - Read files: Read the contents of files
   - Write files: Create new files or overwrite existing ones
   - Edit files: Multiple edit operations (replace text, append, prepend, insert at line)
   - All operations have security checks and confirmations

4. **Model Listing**: List available models on the Ollama server

## Security Features

NeoNeo includes several security features:

- Pattern detection for potentially dangerous expressions in the calculator
- Blocked command detection for shell operations
- Single-key confirmation dialogs for sensitive operations (Enter to confirm, ESC to cancel)
- Configuration options to bypass safety checks when needed
- Timeout controls for long-running operations

## Configuration System

NeoNeo uses a JSON configuration file stored at `~/.config/neoneo/config.json`. Configuration can be:

- Loaded automatically from the default location
- Specified with `--config PATH`
- Saved with current command-line options using `--save-config`
- Bypassed with `--no-config`

## Dependencies

- [libcurl](https://curl.haxx.se/libcurl/) - HTTP requests
- [nlohmann/json](https://github.com/nlohmann/json) - JSON parsing
- [readline](https://tiswww.case.edu/php/chet/readline/rltop.html) - Command line editing
- [bc](https://www.gnu.org/software/bc/) - Command-line calculator

## License

MIT