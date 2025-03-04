#include "../include/ollama_client.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>

using json = nlohmann::json;
using namespace neoneo;

// Callback function for CURL to write response data
static size_t write_callback(char* ptr, size_t size, size_t nmemb, std::string* data) {
    data->append(ptr, size * nmemb);
    return size * nmemb;
}

// Callback function for streaming responses
static size_t stream_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto callback_data = static_cast<std::pair<std::string*, std::function<void(const std::string&)>*>*>(userdata);
    std::string* response_buffer = callback_data->first;
    auto callback_func = callback_data->second;
    
    std::string chunk(ptr, size * nmemb);
    response_buffer->append(chunk);
    
    // Process complete lines
    size_t pos = 0;
    size_t line_end;
    
    while ((line_end = response_buffer->find('\n', pos)) != std::string::npos) {
        std::string line = response_buffer->substr(pos, line_end - pos);
        
        // Parse JSON line
        if (!line.empty()) {
            try {
                json j = json::parse(line);
                if (j.contains("message") && j["message"].contains("content")) {
                    std::string resp = j["message"]["content"].get<std::string>();
                    (*callback_func)(resp);
                }
            } catch (json::parse_error& e) {
                std::cerr << "JSON parse error: " << e.what() << std::endl;
            }
        }
        
        pos = line_end + 1;
    }
    
    // Keep any incomplete line in the buffer
    *response_buffer = response_buffer->substr(pos);
    
    return size * nmemb;
}

// Parse tool calls from JSON response
static std::vector<ToolCall> parse_tool_calls(const json& message_json) {
    std::vector<ToolCall> tool_calls;
    
    if (message_json.contains("tool_calls") && message_json["tool_calls"].is_array()) {
        for (const auto& tool_call_json : message_json["tool_calls"]) {
            ToolCall tool_call;
            
            if (tool_call_json.contains("id")) {
                tool_call.id = tool_call_json["id"].get<std::string>();
            }
            
            if (tool_call_json.contains("function")) {
                auto& function = tool_call_json["function"];
                if (function.contains("name")) {
                    tool_call.name = function["name"].get<std::string>();
                }
                
                if (function.contains("arguments")) {
                    // Handle arguments which could be a JSON string or already a JSON object
                    if (function["arguments"].is_string()) {
                        // Parse arguments as JSON
                        try {
                            tool_call.arguments = json::parse(function["arguments"].get<std::string>());
                        } catch (json::parse_error& e) {
                            // If parse fails, store as raw string
                            tool_call.arguments = function["arguments"].get<std::string>();
                        }
                    } else {
                        // Already a JSON object
                        tool_call.arguments = function["arguments"];
                    }
                }
            }
            
            tool_calls.push_back(tool_call);
        }
    }
    
    return tool_calls;
}

// Implementation class (PIMPL pattern)
class OllamaClient::Impl {
public:
    Impl(const std::string& host) : host_(host) {
        curl_global_init(CURL_GLOBAL_ALL);
    }
    
    ~Impl() {
        curl_global_cleanup();
    }
    
    bool connect() {
        CURL* curl = curl_easy_init();
        if (!curl) return false;
        
        std::string url = host_ + "/api/version";
        std::string response;
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        
        if (res != CURLE_OK) {
            return false;
        }
        
        try {
            json j = json::parse(response);
            if (j.contains("version")) {
                return true;
            }
        } catch (...) {
            return false;
        }
        
        return false;
    }
    
    std::vector<std::string> list_models() {
        std::vector<std::string> models;
        
        CURL* curl = curl_easy_init();
        if (!curl) return models;
        
        std::string url = host_ + "/api/tags";
        std::string response;
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        
        if (res == CURLE_OK) {
            try {
                json j = json::parse(response);
                if (j.contains("models") && j["models"].is_array()) {
                    for (const auto& model : j["models"]) {
                        if (model.contains("name")) {
                            models.push_back(model["name"].get<std::string>());
                        }
                    }
                }
            } catch (json::parse_error& e) {
                std::cerr << "JSON parse error: " << e.what() << std::endl;
            }
        }
        
        return models;
    }
    
    ChatMessage chat(const std::string& model, 
                     const std::vector<ChatMessage>& messages,
                     const std::vector<Tool>& tools,
                     std::function<void(const std::string&)> stream_callback) {
        if (stream_callback) {
            // For streaming with tools, we currently just return the content
            // without proper tool handling in streaming mode
            std::string full_response;
            chat_stream(model, messages, [&](const std::string& chunk) {
                full_response += chunk;
                if (stream_callback) stream_callback(chunk);
            }, tools);
            
            return ChatMessage("assistant", full_response);
        }
        
        CURL* curl = curl_easy_init();
        if (!curl) return ChatMessage("assistant", "");
        
        std::string url = host_ + "/api/chat";
        std::string response;
        
        // Prepare JSON payload
        json j_messages = json::array();
        for (const auto& msg : messages) {
            json message = {
                {"role", msg.role},
                {"content", msg.content}
            };
            
            // Add name for tool responses
            if (msg.role == "tool" && !msg.name.empty()) {
                message["name"] = msg.name;
            }
            
            j_messages.push_back(message);
        }
        
        json payload = {
            {"model", model},
            {"messages", j_messages},
            {"stream", false}
        };
        
        // Add tools if provided
        if (!tools.empty()) {
            json j_tools = json::array();
            for (const auto& tool : tools) {
                json j_tool = {
                    {"type", tool.type},
                    {"function", {
                        {"name", tool.function.name},
                        {"description", tool.function.description},
                        {"parameters", tool.function.parameters}
                    }}
                };
                j_tools.push_back(j_tool);
            }
            payload["tools"] = j_tools;
        }
        
        std::string payload_str = payload.dump();
        
        // Set up HTTP request
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload_str.c_str());
        
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        CURLcode res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        if (res == CURLE_OK) {
            try {
                json j = json::parse(response);
                if (j.contains("message")) {
                    ChatMessage chat_message("assistant", "");
                    
                    // Extract content if available
                    if (j["message"].contains("content")) {
                        chat_message.content = j["message"]["content"].get<std::string>();
                    }
                    
                    // Extract tool calls if available
                    chat_message.tool_calls = parse_tool_calls(j["message"]);
                    
                    return chat_message;
                }
            } catch (json::parse_error& e) {
                std::cerr << "JSON parse error: " << e.what() << std::endl;
            }
        }
        
        return ChatMessage("assistant", "");
    }
    
    void chat_stream(const std::string& model,
                   const std::vector<ChatMessage>& messages,
                   std::function<void(const std::string&)> callback,
                   const std::vector<Tool>& tools) {
        CURL* curl = curl_easy_init();
        if (!curl) return;
        
        std::string url = host_ + "/api/chat";
        std::string response_buffer;
        
        // Prepare JSON payload
        json j_messages = json::array();
        for (const auto& msg : messages) {
            json message = {
                {"role", msg.role},
                {"content", msg.content}
            };
            
            // Add name for tool responses
            if (msg.role == "tool" && !msg.name.empty()) {
                message["name"] = msg.name;
            }
            
            j_messages.push_back(message);
        }
        
        json payload = {
            {"model", model},
            {"messages", j_messages},
            {"stream", true}
        };
        
        // Add tools if provided
        if (!tools.empty()) {
            json j_tools = json::array();
            for (const auto& tool : tools) {
                json j_tool = {
                    {"type", tool.type},
                    {"function", {
                        {"name", tool.function.name},
                        {"description", tool.function.description},
                        {"parameters", tool.function.parameters}
                    }}
                };
                j_tools.push_back(j_tool);
            }
            payload["tools"] = j_tools;
        }
        
        std::string payload_str = payload.dump();
        
        // Set up HTTP request
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload_str.c_str());
        
        // Set up callback for streaming
        auto callback_data = std::make_pair(&response_buffer, &callback);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, stream_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &callback_data);
        
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        CURLcode res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        if (res != CURLE_OK) {
            std::cerr << "CURL error: " << curl_easy_strerror(res) << std::endl;
        }
    }
    
private:
    std::string host_;
};

// OllamaClient implementation
OllamaClient::OllamaClient(const std::string& host) : pimpl(std::make_unique<Impl>(host)) {}

OllamaClient::~OllamaClient() = default;

bool OllamaClient::connect() {
    return pimpl->connect();
}

std::vector<std::string> OllamaClient::list_models() {
    return pimpl->list_models();
}

ChatMessage OllamaClient::chat(const std::string& model, 
                             const std::vector<ChatMessage>& messages,
                             const std::vector<Tool>& tools,
                             std::function<void(const std::string&)> stream_callback) {
    return pimpl->chat(model, messages, tools, stream_callback);
}

// Convert tool definitions to tools and call the original method
ChatMessage OllamaClient::chat(const std::string& model, 
                             const std::vector<ChatMessage>& messages,
                             const std::vector<nlohmann::json>& tool_definitions,
                             std::function<void(const std::string&)> stream_callback) {
    // Convert tool_definitions to tools
    std::vector<Tool> tools;
    for (const auto& def : tool_definitions) {
        Tool tool;
        tool.type = "function";
        
        if (def.contains("function")) {
            auto& function = def["function"];
            if (function.contains("name")) {
                tool.function.name = function["name"].get<std::string>();
            }
            if (function.contains("description")) {
                tool.function.description = function["description"].get<std::string>();
            }
            if (function.contains("parameters")) {
                tool.function.parameters = function["parameters"];
            }
        }
        
        tools.push_back(tool);
    }
    
    return pimpl->chat(model, messages, tools, stream_callback);
}

void OllamaClient::chat_stream(const std::string& model,
                           const std::vector<ChatMessage>& messages,
                           std::function<void(const std::string&)> callback,
                           const std::vector<Tool>& tools) {
    pimpl->chat_stream(model, messages, callback, tools);
}

// Convert tool definitions to tools and call the original method
void OllamaClient::chat_stream(const std::string& model,
                           const std::vector<ChatMessage>& messages,
                           std::function<void(const std::string&)> callback,
                           const std::vector<nlohmann::json>& tool_definitions) {
    // Convert tool_definitions to tools
    std::vector<Tool> tools;
    for (const auto& def : tool_definitions) {
        Tool tool;
        tool.type = "function";
        
        if (def.contains("function")) {
            auto& function = def["function"];
            if (function.contains("name")) {
                tool.function.name = function["name"].get<std::string>();
            }
            if (function.contains("description")) {
                tool.function.description = function["description"].get<std::string>();
            }
            if (function.contains("parameters")) {
                tool.function.parameters = function["parameters"];
            }
        }
        
        tools.push_back(tool);
    }
    
    pimpl->chat_stream(model, messages, callback, tools);
}