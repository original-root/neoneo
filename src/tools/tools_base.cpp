#include "../../include/neoneo/tools/tools.hpp"
#include <memory>

namespace neoneo {
namespace tools {

// Base class implementation
ToolBase::ToolBase(ToolManager& manager) : tool_manager(manager) {}

nlohmann::json ToolBase::get_definition() const {
    return {
        {"type", "function"},
        {"function", {
            {"name", get_name()},
            {"description", get_description()},
            {"parameters", get_parameters()}
        }}
    };
}

// Tool Manager implementation
ToolManager::ToolManager(config::Config& config) : config(config) {}

ToolManager::~ToolManager() = default;

void ToolManager::register_tool(std::unique_ptr<ToolBase> tool) {
    tools[tool->get_name()] = std::move(tool);
}

void ToolManager::register_default_tools() {
    // Register calculator tool
    register_tool(std::make_unique<CalculatorTool>(*this));
    
    // Register shell tools if enabled
    if (config.is_shell_enabled()) {
        register_tool(std::make_unique<ShellTool>(*this));
        register_tool(std::make_unique<BashTool>(*this));
    }
    
    // Register model listing tool if enabled
    if (config.is_model_list_enabled()) {
        register_tool(std::make_unique<ModelListTool>(*this));
    }
    
    // Register file operation tools if enabled
    if (config.is_file_ops_enabled()) {
        register_tool(std::make_unique<FileReadTool>(*this));
        register_tool(std::make_unique<FileWriteTool>(*this));
        register_tool(std::make_unique<FileEditTool>(*this));
    }
}

std::vector<nlohmann::json> ToolManager::get_tool_definitions() const {
    std::vector<nlohmann::json> definitions;
    for (const auto& [name, tool] : tools) {
        definitions.push_back(tool->get_definition());
    }
    return definitions;
}

bool ToolManager::has_tool(const std::string& name) const {
    return tools.find(name) != tools.end();
}

ToolResult ToolManager::execute_tool(const std::string& name, const nlohmann::json& args) {
    auto it = tools.find(name);
    if (it == tools.end()) {
        return ToolResult::error("Tool not found: " + name);
    }
    
    return it->second->execute(args);
}

} // namespace tools
} // namespace neoneo