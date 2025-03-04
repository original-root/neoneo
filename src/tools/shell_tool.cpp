#include "../../include/neoneo/tools/tools.hpp"
#include "../../include/neoneo/terminal/terminal.hpp"
#include <iostream>
#include <chrono>
#include <array>
#include <cstdio>
#include <vector>
#include <algorithm>

namespace neoneo {
namespace tools {

ShellTool::ShellTool(ToolManager& manager) : ToolBase(manager) {}

std::string ShellTool::get_description() const {
    return "Execute a shell command and return the output";
}

nlohmann::json ShellTool::get_parameters() const {
    return {
        {"type", "object"},
        {"required", nlohmann::json::array({"command"})},
        {"properties", {
            {"command", {
                {"type", "string"}, 
                {"description", "The shell command to execute. Certain commands are blocked for security."}
            }},
            {"timeout", {
                {"type", "integer"}, 
                {"description", "Maximum execution time in seconds (1-30). Defaults to 5 seconds."}
            }}
        }}
    };
}

bool ShellTool::is_command_safe(const std::string& command, std::string& operation_found) {
    static const std::vector<std::string> blocked_commands = {
        "rm", "mkfs", "dd", ">", ">>", "|", "&", ";", "&&", "||", 
        "sudo", "su", "chmod", "chown", "passwd", "mv", "curl", "wget",
        "ssh", "scp", "ftp", "telnet", "nc", "ncat", "sleep", "perl",
        "python", "python3", "ruby", "bash", "sh", "zsh", "csh", "ksh"
    };
    
    for (const auto& blocked : blocked_commands) {
        if (command.find(blocked) != std::string::npos) {
            operation_found = blocked;
            return false;
        }
    }
    
    return true;
}

ToolResult ShellTool::execute(const nlohmann::json& args) {
    try {
        // Validate command argument
        if (!args.contains("command") || !args["command"].is_string()) {
            return ToolResult::error("Missing or invalid 'command' parameter");
        }
        
        std::string command = args["command"].get<std::string>();
        
        // Extract timeout if provided (default: 5 seconds)
        int timeout_seconds = 5;
        if (args.contains("timeout") && args["timeout"].is_number()) {
            timeout_seconds = args["timeout"].get<int>();
            // Limit maximum timeout to 30 seconds for safety
            if (timeout_seconds > 30) {
                timeout_seconds = 30;
            } else if (timeout_seconds < 1) {
                timeout_seconds = 1;
            }
        }
        
        // Check for potentially dangerous commands (unless safety checks are disabled)
        if (!tool_manager.get_config().is_shell_safety_ignored()) {
            std::string operation_found;
            if (!is_command_safe(command, operation_found)) {
                // Potentially dangerous command found - ask for confirmation
                bool confirmed = terminal::confirm_dialog(
                    terminal::ConfirmType::SHELL_COMMAND,
                    "The command contains a potentially dangerous operation:",
                    "'" + operation_found + "' found in: " + command,
                    "This could potentially harm your system or expose sensitive data.",
                    "Tip: Use --ignore-shell-safety to disable these warnings."
                );
                
                if (!confirmed) {
                    return ToolResult::error("Command execution aborted due to security concerns with operation: " + operation_found);
                }
                // If confirmed, continue with execution
                std::cout << "Proceeding with execution despite warning." << std::endl;
            }
        }

        // If auto-confirm is not enabled, require explicit confirmation for all shell commands
        if (!tool_manager.get_config().is_auto_confirm_shell()) {
            bool confirmed = terminal::confirm_dialog(
                terminal::ConfirmType::SHELL_COMMAND,
                "The AI is requesting to execute the following command:",
                command,
                "This could potentially modify your system.",
                ""
            );
            
            if (!confirmed) {
                return ToolResult::error("Command execution denied by user");
            }
        }
        
        // Add output redirection to capture command output
        command += " 2>&1";
        
        // Execute the command and capture output with basic timing
        auto start_time = std::chrono::steady_clock::now();
        
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            return ToolResult::error("Failed to execute command");
        }
        
        // Calculate timeout in microseconds
        auto timeout = std::chrono::seconds(timeout_seconds);
        
        char buffer[128];
        std::string result;
        bool timed_out = false;
        
        while (!feof(pipe)) {
            // Check if we've exceeded the timeout
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = current_time - start_time;
            
            if (elapsed > timeout) {
                timed_out = true;
                break;
            }
            
            // Try to read with a short timeout to check periodically
            if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                result += buffer;
                
                // Limit output size
                if (result.size() > 1000) {
                    result = result.substr(0, 1000) + "\n... (output truncated)";
                    break;
                }
            }
        }
        
        // Close the pipe
        pclose(pipe);
        
        // Handle timeout
        if (timed_out) {
            return ToolResult::error("Command execution timed out after " + 
                                   std::to_string(timeout_seconds) + " seconds");
        }
        
        // Return the command output
        return ToolResult::success(result.empty() ? 
                                 "Command executed successfully (no output)" : result);
    } catch (const std::exception& e) {
        std::cerr << "Error in shell command: " << e.what() << std::endl;
        return ToolResult::error("Error executing command: " + std::string(e.what()));
    }
}

} // namespace tools
} // namespace neoneo