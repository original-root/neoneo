#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <map>
#include <nlohmann/json.hpp>

namespace neoneo {

// Tool call information
struct ToolCall {
    std::string id;
    std::string name;
    nlohmann::json arguments;
};

// Define callback for tool execution
using ToolFunction = std::function<std::string(const nlohmann::json&)>;

// Message structure to handle tool calls and responses
struct ChatMessage {
    std::string role;
    std::string content;
    std::string name; // For tool responses
    std::vector<ToolCall> tool_calls;
    
    // Constructor for regular messages
    ChatMessage(std::string role, std::string content)
        : role(std::move(role)), content(std::move(content)) {}
    
    // Constructor for tool response
    ChatMessage(const std::string& role, std::string content, std::string name)
        : role(role), content(std::move(content)), name(std::move(name)) {}
        
    // Static method to create a tool response
    static ChatMessage make_tool_response(std::string content, std::string name) {
        return ChatMessage("tool", std::move(content), std::move(name));
    }
};

// Tool definition
struct Tool {
    std::string type = "function";
    struct {
        std::string name;
        std::string description;
        nlohmann::json parameters;
    } function;
};

class OllamaClient {
public:
    OllamaClient(const std::string& host = "http://localhost:11434");
    ~OllamaClient();

    // Initialize connection to Ollama server
    bool connect();
    
    // List available models
    std::vector<std::string> list_models();
    
    // Generate a chat completion with tool support
    ChatMessage chat(const std::string& model, 
                     const std::vector<ChatMessage>& messages,
                     const std::vector<Tool>& tools = {},
                     std::function<void(const std::string&)> stream_callback = nullptr);
                     
    // Generate a chat completion with tool support using tool definitions
    ChatMessage chat(const std::string& model, 
                     const std::vector<ChatMessage>& messages,
                     const std::vector<nlohmann::json>& tool_definitions,
                     std::function<void(const std::string&)> stream_callback = nullptr);
    
    // Generate a streaming chat completion
    void chat_stream(const std::string& model,
                   const std::vector<ChatMessage>& messages,
                   std::function<void(const std::string&)> callback,
                   const std::vector<Tool>& tools = {});
                   
    // Generate a streaming chat completion using tool definitions
    void chat_stream(const std::string& model,
                   const std::vector<ChatMessage>& messages,
                   std::function<void(const std::string&)> callback,
                   const std::vector<nlohmann::json>& tool_definitions = {});

private:
    class Impl;
    std::unique_ptr<Impl> pimpl;
};

} // namespace neoneo