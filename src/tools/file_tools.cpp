#include "../../include/neoneo/tools/tools.hpp"
#include "../../include/neoneo/terminal/terminal.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <filesystem>

namespace neoneo {
namespace tools {

namespace fs = std::filesystem;

// File Read Tool Implementation
FileReadTool::FileReadTool(ToolManager& manager) : ToolBase(manager) {}

std::string FileReadTool::get_description() const {
    return "Read the contents of a file";
}

nlohmann::json FileReadTool::get_parameters() const {
    return {
        {"type", "object"},
        {"required", nlohmann::json::array({"path"})},
        {"properties", {
            {"path", {
                {"type", "string"}, 
                {"description", "The path to the file to read"}
            }}
        }}
    };
}

ToolResult FileReadTool::execute(const nlohmann::json& args) {
    try {
        // Validate path parameter
        if (!args.contains("path") || !args["path"].is_string()) {
            return ToolResult::error("Missing or invalid 'path' parameter");
        }
        
        std::string file_path = args["path"].get<std::string>();
        
        // Security check: Prevent directory traversal
        if (file_path.find("..") != std::string::npos) {
            return ToolResult::error("Path contains forbidden '..' sequence");
        }
        
        // Check if the file exists
        if (!fs::exists(file_path)) {
            return ToolResult::error("File does not exist: " + file_path);
        }
        
        // Make sure it's a regular file
        if (!fs::is_regular_file(file_path)) {
            return ToolResult::error("Not a regular file: " + file_path);
        }
        
        // Open the file
        std::ifstream file(file_path);
        if (!file.is_open()) {
            return ToolResult::error("Could not open file: " + file_path);
        }
        
        // Read the file content
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        
        // If the file is too large, truncate it
        const size_t max_size = 50000; // 50KB limit
        if (content.size() > max_size) {
            content = content.substr(0, max_size) + "\n... (content truncated, file too large)";
        }
        
        return ToolResult::success(content);
    } catch (const std::exception& e) {
        return ToolResult::error("Error reading file: " + std::string(e.what()));
    }
}

// File Write Tool Implementation
FileWriteTool::FileWriteTool(ToolManager& manager) : ToolBase(manager) {}

std::string FileWriteTool::get_description() const {
    return "Write content to a file (creates or overwrites)";
}

nlohmann::json FileWriteTool::get_parameters() const {
    return {
        {"type", "object"},
        {"required", nlohmann::json::array({"path", "content"})},
        {"properties", {
            {"path", {
                {"type", "string"}, 
                {"description", "The path to the file to write"}
            }},
            {"content", {
                {"type", "string"}, 
                {"description", "The content to write to the file"}
            }}
        }}
    };
}

ToolResult FileWriteTool::execute(const nlohmann::json& args) {
    try {
        // Validate parameters
        if (!args.contains("path") || !args["path"].is_string()) {
            return ToolResult::error("Missing or invalid 'path' parameter");
        }
        
        if (!args.contains("content") || !args["content"].is_string()) {
            return ToolResult::error("Missing or invalid 'content' parameter");
        }
        
        std::string file_path = args["path"].get<std::string>();
        std::string content = args["content"].get<std::string>();
        
        // Security check: Prevent directory traversal
        if (file_path.find("..") != std::string::npos) {
            return ToolResult::error("Path contains forbidden '..' sequence");
        }
        
        // If auto-confirm is not enabled, require explicit confirmation
        if (!tool_manager.get_config().is_auto_confirm_file_ops()) {
            // Truncate content preview if too long
            std::string content_preview = content;
            if (content_preview.length() > 200) {
                content_preview = content_preview.substr(0, 200) + "... (truncated)";
            }
            
            bool confirmed = terminal::confirm_dialog(
                terminal::ConfirmType::FILE_OPERATION,
                "The AI is requesting to write to the file:",
                file_path,
                "Content (preview):\n" + content_preview,
                ""
            );
            
            if (!confirmed) {
                return ToolResult::error("File write operation denied by user");
            }
        }
        
        // Create parent directories if needed
        auto dir_path = fs::path(file_path).parent_path();
        if (!dir_path.empty() && !fs::exists(dir_path)) {
            fs::create_directories(dir_path);
        }
        
        // Write the file
        std::ofstream file(file_path);
        if (!file.is_open()) {
            return ToolResult::error("Could not open file for writing: " + file_path);
        }
        
        file << content;
        file.close();
        
        return ToolResult::success("File successfully written: " + file_path + 
                                 " (" + std::to_string(content.size()) + " bytes)");
    } catch (const std::exception& e) {
        return ToolResult::error("Error writing file: " + std::string(e.what()));
    }
}

// File Edit Tool Implementation
FileEditTool::FileEditTool(ToolManager& manager) : ToolBase(manager) {}

std::string FileEditTool::get_description() const {
    return "Edit an existing file (partial edits or replacement)";
}

nlohmann::json FileEditTool::get_parameters() const {
    return {
        {"type", "object"},
        {"required", nlohmann::json::array({"path"})},
        {"properties", {
            {"path", {
                {"type", "string"}, 
                {"description", "The path to the file to edit"}
            }},
            {"replace_all", {
                {"type", "string"}, 
                {"description", "If provided, replaces the entire file content"}
            }},
            {"old_text", {
                {"type", "string"}, 
                {"description", "The text to find and replace"}
            }},
            {"new_text", {
                {"type", "string"}, 
                {"description", "The new text to replace with"}
            }},
            {"append", {
                {"type", "string"}, 
                {"description", "Text to append to the end of the file"}
            }},
            {"prepend", {
                {"type", "string"}, 
                {"description", "Text to insert at the beginning of the file"}
            }},
            {"insert_at_line", {
                {"type", "integer"}, 
                {"description", "Line number where to insert text (0-based)"}
            }},
            {"text", {
                {"type", "string"}, 
                {"description", "Text to insert at the specified line"}
            }}
        }}
    };
}

ToolResult FileEditTool::execute(const nlohmann::json& args) {
    try {
        // Validate parameters
        if (!args.contains("path") || !args["path"].is_string()) {
            return ToolResult::error("Missing or invalid 'path' parameter");
        }
        
        std::string file_path = args["path"].get<std::string>();
        
        // Security check: Prevent directory traversal
        if (file_path.find("..") != std::string::npos) {
            return ToolResult::error("Path contains forbidden '..' sequence");
        }
        
        // Check if file exists and read it
        if (!fs::exists(file_path)) {
            return ToolResult::error("File does not exist: " + file_path);
        }
        
        std::ifstream in_file(file_path);
        if (!in_file.is_open()) {
            return ToolResult::error("Could not open file for reading: " + file_path);
        }
        
        std::stringstream buffer;
        buffer << in_file.rdbuf();
        std::string content = buffer.str();
        in_file.close();
        
        // Determine operation type and description for confirmation
        enum class EditOp { REPLACE_ALL, REPLACE_TEXT, APPEND, PREPEND, INSERT_LINE, UNKNOWN };
        EditOp operation = EditOp::UNKNOWN;
        std::string op_description;
        std::string preview_old;
        std::string preview_new;
        
        if (args.contains("replace_all") && args["replace_all"].is_string()) {
            operation = EditOp::REPLACE_ALL;
            op_description = "Replace entire file";
            preview_new = args["replace_all"].get<std::string>();
        } else if (args.contains("old_text") && args.contains("new_text") && 
                   args["old_text"].is_string() && args["new_text"].is_string()) {
            operation = EditOp::REPLACE_TEXT;
            op_description = "Replace text in file";
            preview_old = args["old_text"].get<std::string>();
            preview_new = args["new_text"].get<std::string>();
        } else if (args.contains("append") && args["append"].is_string()) {
            operation = EditOp::APPEND;
            op_description = "Append to file";
            preview_new = args["append"].get<std::string>();
        } else if (args.contains("prepend") && args["prepend"].is_string()) {
            operation = EditOp::PREPEND;
            op_description = "Prepend to file";
            preview_new = args["prepend"].get<std::string>();
        } else if (args.contains("insert_at_line") && args.contains("text") &&
                   args["insert_at_line"].is_number() && args["text"].is_string()) {
            operation = EditOp::INSERT_LINE;
            op_description = "Insert at line " + std::to_string(args["insert_at_line"].get<int>());
            preview_new = args["text"].get<std::string>();
        } else {
            return ToolResult::error("No valid edit operation specified. Use 'replace_all', "
                                   "'old_text'+'new_text', 'append', 'prepend', or "
                                   "'insert_at_line'+'text'");
        }
        
        // If auto-confirm is not enabled, require explicit confirmation
        if (!tool_manager.get_config().is_auto_confirm_file_ops()) {
            // Format preview strings
            auto format_preview = [](const std::string& text, size_t max_len = 100) -> std::string {
                if (text.length() > max_len) {
                    return text.substr(0, max_len) + "... (truncated)";
                }
                return text;
            };
            
            std::string details = "Operation: " + op_description;
            
            if (operation == EditOp::REPLACE_TEXT) {
                details += "\nOld Text: " + format_preview(preview_old);
                details += "\nNew Text: " + format_preview(preview_new);
            } else if (operation != EditOp::UNKNOWN) {
                details += "\nNew Content: " + format_preview(preview_new);
            }
            
            bool confirmed = terminal::confirm_dialog(
                terminal::ConfirmType::FILE_OPERATION,
                "The AI is requesting to edit the file:",
                file_path,
                details,
                ""
            );
            
            if (!confirmed) {
                return ToolResult::error("File edit operation denied by user");
            }
        }
        
        // Perform the requested edit
        switch (operation) {
            case EditOp::REPLACE_ALL:
                content = args["replace_all"].get<std::string>();
                break;
                
            case EditOp::REPLACE_TEXT: {
                std::string old_text = args["old_text"].get<std::string>();
                std::string new_text = args["new_text"].get<std::string>();
                
                size_t pos = content.find(old_text);
                if (pos == std::string::npos) {
                    return ToolResult::error("Could not find the text to replace in the file");
                }
                
                content.replace(pos, old_text.length(), new_text);
                break;
            }
                
            case EditOp::APPEND:
                content += args["append"].get<std::string>();
                break;
                
            case EditOp::PREPEND:
                content = args["prepend"].get<std::string>() + content;
                break;
                
            case EditOp::INSERT_LINE: {
                int line_number = args["insert_at_line"].get<int>();
                std::string text = args["text"].get<std::string>();
                
                std::stringstream ss(content);
                std::string line;
                std::vector<std::string> lines;
                
                while (std::getline(ss, line)) {
                    lines.push_back(line);
                }
                
                // Boundary checking for line number
                if (line_number < 0) {
                    line_number = 0;
                } else if (line_number > (int)lines.size()) {
                    line_number = (int)lines.size();
                }
                
                lines.insert(lines.begin() + line_number, text);
                
                // Reconstruct content
                content = "";
                for (size_t i = 0; i < lines.size(); ++i) {
                    content += lines[i];
                    if (i < lines.size() - 1) {
                        content += "\n";
                    }
                }
                break;
            }
                
            case EditOp::UNKNOWN:
                return ToolResult::error("Unknown edit operation");
        }
        
        // Write back to file
        std::ofstream out_file(file_path);
        if (!out_file.is_open()) {
            return ToolResult::error("Could not open file for writing: " + file_path);
        }
        
        out_file << content;
        out_file.close();
        
        return ToolResult::success("File successfully edited: " + file_path);
    } catch (const std::exception& e) {
        return ToolResult::error("Error editing file: " + std::string(e.what()));
    }
}

} // namespace tools
} // namespace neoneo