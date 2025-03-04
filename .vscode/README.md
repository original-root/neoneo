# VSCode Debugging Setup for neoneo

This folder contains configuration files for debugging neoneo in Visual Studio Code.

## Prerequisites

To use these debugging configurations, you need to install the following extensions:

1. **C/C++** (ms-vscode.cpptools)
2. **CMake Tools** (ms-vscode.cmake-tools)

## Debug Configurations

Two debug configurations are provided:

1. **Debug neoneo**: Launches neoneo with tools, shell, and file operations enabled
2. **Debug neoneo (no tools)**: Launches neoneo without any tools

## How to Debug

1. Open the project in VSCode
2. Set breakpoints in the code where you want to pause execution
3. Press `F5` or go to `Run > Start Debugging` to start debugging
4. Select one of the available debug configurations

## Build Tasks

The following tasks are available:

- **cmake-configure**: Configures the project with debug symbols
- **build-debug**: Builds the project in Debug mode (default task)
- **clean**: Cleans the build directory

You can run these tasks from the Command Palette (`Ctrl+Shift+P` or `Cmd+Shift+P`) by typing `Tasks: Run Task` and selecting the task you want to run.

## Important Notes

- The build configuration ensures debug symbols are included
- External console is enabled for better handling of terminal input/output
- LLDB is used as the debugger on macOS
- If you encounter issues with breakpoints, you may need to add the `--with-debug-info` flag to the LLDB command in your launch configuration