#include "../../include/neoneo/tools/tools.hpp"
#include "../../include/ollama_client.hpp"
#include <iostream>

namespace neoneo {
namespace tools {

ModelListTool::ModelListTool(ToolManager& manager) : ToolBase(manager) {}

std::string ModelListTool::get_description() const {
    return "List available models on the Ollama server";
}

nlohmann::json ModelListTool::get_parameters() const {
    return {
        {"type", "object"},
        {"properties", {
            {"host", {
                {"type", "string"}, 
                {"description", "Optional: The Ollama server URL (default: http://localhost:11434)"}
            }}
        }}
    };
}

ToolResult ModelListTool::execute(const nlohmann::json& args) {
    try {
        // Get the host parameter if provided
        std::string host = tool_manager.get_config().get_host();
        if (args.contains("host") && args["host"].is_string()) {
            host = args["host"].get<std::string>();
        }
        
        // Create a temporary OllamaClient
        OllamaClient client(host);
        
        // Check connection
        if (!client.connect()) {
            return ToolResult::error("Could not connect to Ollama server at " + host);
        }
        
        // List available models
        auto models = client.list_models();
        
        if (models.empty()) {
            return ToolResult::success("No models found on Ollama server at " + host);
        }
        
        // Format the output
        std::string result = "Available models on Ollama server at " + host + ":\n";
        for (size_t i = 0; i < models.size(); i++) {
            result += "  " + std::to_string(i+1) + ". " + models[i] + "\n";
        }
        
        return ToolResult::success(result);
    } catch (const std::exception& e) {
        std::cerr << "Error listing Ollama models: " << e.what() << std::endl;
        return ToolResult::error("Error listing Ollama models: " + std::string(e.what()));
    }
}

} // namespace tools
} // namespace neoneo