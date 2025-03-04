#include "../../include/neoneo/tools/tools.hpp"
#include "../../include/neoneo/terminal/terminal.hpp"
#include <iostream>
#include <chrono>
#include <array>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <regex>
#include <sstream>
#include <memory>

namespace neoneo {
namespace tools {

BashTool::BashTool(ToolManager& manager) : ToolBase(manager) {}

std::string BashTool::get_description() const {
    return "Execute bash commands with advanced output handling and formatting. This tool is more powerful than execute_shell_command, with better error detection, command validation, and more comprehensive output.";
}

nlohmann::json BashTool::get_parameters() const {
    return {
        {"type", "object"},
        {"required", nlohmann::json::array({"command"})},
        {"properties", {
            {"command", {
                {"type", "string"}, 
                {"description", "The bash command to execute. Must be a valid bash command."}
            }},
            {"timeout", {
                {"type", "integer"}, 
                {"description", "Maximum execution time in seconds (1-60). Defaults to 10 seconds."}
            }},
            {"working_directory", {
                {"type", "string"},
                {"description", "Working directory to execute the command in. Defaults to current directory."}
            }}
        }}
    };
}

bool BashTool::is_command_safe(const std::string& command, std::string& operation_found) {
    static const std::vector<std::string> blocked_commands = {
        "rm -rf", "mkfs", "dd if=", "> /dev", "echo > /dev", ">/dev",
        "sudo rm", "sudo mv", "sudo cp", "reboot", "shutdown",
        "passwd", "chmod 777", "chmod -R 777", ":(){ :|:& };:", "fork bomb"
    };
    
    for (const auto& blocked : blocked_commands) {
        if (command.find(blocked) != std::string::npos) {
            operation_found = blocked;
            return false;
        }
    }
    
    return true;
}

std::string BashTool::execute_command(const std::string& command, int timeout_seconds, bool& timed_out) {
    timed_out = false;
    
    // Use popen to execute the command and read its output
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return "Error: Failed to execute command";
    }
    
    // Start the timer
    auto start_time = std::chrono::steady_clock::now();
    auto timeout = std::chrono::seconds(timeout_seconds);
    
    // Buffer for reading output
    std::array<char, 4096> buffer;
    std::string result;
    size_t max_output_size = 1000000; // 1MB max output
    
    // Read until EOF, timeout, or size limit reached
    while (!feof(pipe)) {
        // Check for timeout
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = current_time - start_time;
        
        if (elapsed > timeout) {
            timed_out = true;
            pclose(pipe);
            return "Error: Command execution timed out after " + std::to_string(timeout_seconds) + " seconds.";
        }
        
        // Read a chunk of output
        if (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            result += buffer.data();
            
            // Check size limit
            if (result.size() > max_output_size) {
                result = result.substr(0, max_output_size);
                result += "\n... (output truncated due to size limit)";
                break;
            }
        }
    }
    
    // Close the pipe and get the exit status
    int status = pclose(pipe);
    int exit_code = WEXITSTATUS(status);
    
    // Prepare the result string
    std::stringstream formatted_result;
    
    // Format the result with exit code info
    if (exit_code == 0) {
        // Command succeeded
        if (result.empty()) {
            formatted_result << "Command executed successfully (no output)";
        } else {
            formatted_result << result;
        }
    } else {
        // Command failed
        formatted_result << "Command failed with exit code: " << exit_code << "\n";
        if (!result.empty()) {
            formatted_result << "Output:\n" << result;
        }
    }
    
    return formatted_result.str();
}

ToolResult BashTool::execute(const nlohmann::json& args) {
    try {
        // Validate command argument
        if (!args.contains("command") || !args["command"].is_string()) {
            return ToolResult::error("Missing or invalid 'command' parameter");
        }
        
        std::string command = args["command"].get<std::string>();
        
        // Extract timeout if provided (default: 10 seconds)
        int timeout_seconds = 10;
        if (args.contains("timeout") && args["timeout"].is_number()) {
            timeout_seconds = args["timeout"].get<int>();
            // Limit maximum timeout to 60 seconds
            if (timeout_seconds > 60) {
                timeout_seconds = 60;
            } else if (timeout_seconds < 1) {
                timeout_seconds = 1;
            }
        }
        
        // Extract working directory if provided
        if (args.contains("working_directory") && args["working_directory"].is_string()) {
            std::string working_dir = args["working_directory"].get<std::string>();
            // Prepend cd command
            command = "cd \"" + working_dir + "\" && " + command;
        }
        
        // Check for potentially dangerous commands (unless safety checks are disabled)
        if (!tool_manager.get_config().is_shell_safety_ignored()) {
            std::string operation_found;
            if (!is_command_safe(command, operation_found)) {
                // Potentially dangerous command found - ask for confirmation
                bool confirmed = terminal::confirm_dialog(
                    terminal::ConfirmType::SHELL_COMMAND,
                    "The bash command contains a potentially dangerous operation:",
                    "'" + operation_found + "' found in: " + command,
                    "This operation could potentially harm your system or delete data.",
                    "Tip: Use --ignore-shell-safety to disable these warnings."
                );
                
                if (!confirmed) {
                    return ToolResult::error("Command execution aborted due to security concerns with operation: " + operation_found);
                }
                // If confirmed, continue with execution
                terminal::print("Proceeding with execution despite warning.", terminal::MessageType::WARNING);
            }
        }
        
        // If auto-confirm is not enabled, require explicit confirmation for all bash commands
        if (!tool_manager.get_config().is_auto_confirm_shell()) {
            bool confirmed = terminal::confirm_dialog(
                terminal::ConfirmType::SHELL_COMMAND,
                "The AI is requesting to execute the following bash command:",
                command,
                "This command will be executed with your user permissions.",
                "Use with caution. Some commands may modify your system."
            );
            
            if (!confirmed) {
                return ToolResult::error("Command execution denied by user");
            }
        }
        
        // Add output redirection to capture both stdout and stderr
        command += " 2>&1";
        
        // Execute the command
        bool timed_out = false;
        std::string result = execute_command(command, timeout_seconds, timed_out);
        
        // Handle timeout
        if (timed_out) {
            return ToolResult::error("Command execution timed out after " + 
                                  std::to_string(timeout_seconds) + " seconds");
        }
        
        // Return the command output
        return ToolResult::success(result);
    } catch (const std::exception& e) {
        terminal::print("Error in bash command: " + std::string(e.what()), terminal::MessageType::ERROR);
        return ToolResult::error("Error executing command: " + std::string(e.what()));
    }
}

} // namespace tools
} // namespace neoneo