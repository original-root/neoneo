#include "../../include/neoneo/tools/tools.hpp"
#include "../../include/neoneo/terminal/terminal.hpp"
#include <iostream>
#include <chrono>
#include <array>
#include <cstdio>
#include <algorithm>

namespace neoneo {
namespace tools {

CalculatorTool::CalculatorTool(ToolManager& manager) : ToolBase(manager) {}

std::string CalculatorTool::get_description() const {
    return "Evaluate a mathematical expression using the bc calculator";
}

nlohmann::json CalculatorTool::get_parameters() const {
    return {
        {"type", "object"},
        {"required", nlohmann::json::array({"expression"})},
        {"properties", {
            {"expression", {
                {"type", "string"}, 
                {"description", "A mathematical expression to evaluate. "
                               "Supports basic operations (+, -, *, /), exponents (^), "
                               "parentheses, and functions (sqrt, sin, cos, etc.)"}
            }}
        }}
    };
}

bool CalculatorTool::is_expression_safe(const std::string& expression, std::string& pattern_found) {
    static const std::array<std::string, 8> blocked_patterns = {
        "system", "exec", "shell", "quit", "halt", "cd", "rm", "mv"
    };
    
    for (const auto& pattern : blocked_patterns) {
        if (expression.find(pattern) != std::string::npos) {
            pattern_found = pattern;
            return false;
        }
    }
    
    return true;
}

ToolResult CalculatorTool::execute(const nlohmann::json& args) {
    try {
        // Validate expression parameter
        if (!args.contains("expression") || !args["expression"].is_string()) {
            return ToolResult::error("Missing or invalid 'expression' parameter");
        }
        
        std::string expression = args["expression"].get<std::string>();
        
        // Security check: remove potentially dangerous characters
        const std::string allowed_chars = "0123456789.+-*/^%() \t\nabcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";
        std::string sanitized_expression;
        
        for (char c : expression) {
            if (allowed_chars.find(c) != std::string::npos) {
                sanitized_expression += c;
            }
        }
        
        // Check for potentially dangerous patterns if safety checks aren't ignored
        if (!tool_manager.get_config().is_calc_safety_ignored()) {
            std::string pattern_found;
            if (!is_expression_safe(sanitized_expression, pattern_found)) {
                // Suspicious pattern found - ask for confirmation
                bool confirmed = terminal::confirm_dialog(
                    terminal::ConfirmType::CALCULATION,
                    "The expression contains a potentially unsafe pattern:",
                    "'" + pattern_found + "' found in: " + expression,
                    "This might be a false positive, but could be an attempt to execute code.",
                    "Tip: Use --ignore-calc-safety to disable these warnings."
                );
                
                if (!confirmed) {
                    return ToolResult::error("Calculation aborted due to security concerns with pattern: " + pattern_found);
                }
                // If confirmed, continue with the calculation
                std::cout << "Proceeding with calculation despite warning." << std::endl;
            }
        }
        
        // Limit expression length for safety
        if (sanitized_expression.length() > 500) {
            sanitized_expression = sanitized_expression.substr(0, 500);
        }
        
        // Create a pipe to evaluate the expression using bc calculator with extra safety
        std::string cmd = "echo '" + sanitized_expression + "' | BC_LINE_LENGTH=0 bc -l";
        
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            return ToolResult::error("Failed to execute calculation");
        }
        
        char buffer[128];
        std::string result;
        
        // Add timeout to prevent infinite loops
        auto start_time = std::chrono::steady_clock::now();
        int timeout_ms = 2000; // 2 second timeout
        
        while (!feof(pipe)) {
            // Check for timeout
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time);
            
            if (elapsed.count() > timeout_ms) {
                pclose(pipe);
                return ToolResult::error("Calculation timed out (possible infinite loop or too complex)");
            }
            
            if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                result += buffer;
                
                // Limit result size
                if (result.length() > 1000) {
                    pclose(pipe);
                    return ToolResult::error("Result too large");
                }
            }
        }
        
        pclose(pipe);
        
        // Trim the result
        auto trim = [](std::string& s) {
            // Trim beginning
            s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
            
            // Trim end
            s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
                return !std::isspace(ch);
            }).base(), s.end());
        };
        
        trim(result);
        
        // Return error if empty result
        if (result.empty()) {
            return ToolResult::error("Invalid expression or no result");
        }
        
        // Check for error messages in the result
        if (result.find("syntax error") != std::string::npos ||
            result.find("error") != std::string::npos) {
            return ToolResult::error("Error: " + result);
        }
        
        return ToolResult::success(result);
    } catch (const std::exception& e) {
        std::cerr << "Error in calculator: " << e.what() << std::endl;
        return ToolResult::error("Error in calculation: " + std::string(e.what()));
    }
}

} // namespace tools
} // namespace neoneo