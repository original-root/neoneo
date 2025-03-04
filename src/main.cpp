#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <csignal>
#include <fstream>
#include <sstream>
#include <readline/readline.h>
#include <readline/history.h>
#include <nlohmann/json.hpp>
#include "../include/ollama_client.hpp"
#include "../include/neoneo/config/config.hpp"
#include "../include/neoneo/terminal/terminal.hpp"
#include "../include/neoneo/tools/tools.hpp"

using namespace neoneo;
using json = nlohmann::json;

// Global variables for signal handling
std::atomic<bool> running(true);

// Handle Ctrl+C
void signal_handler(int signal) {
    if (signal == SIGINT) {
        running = false;
        terminal::print("\nExiting...", terminal::MessageType::SYSTEM);
    }
}

// Display help message
void print_usage() {
    terminal::print("Usage: neoneo [options] [model]", terminal::MessageType::HEADER);
    terminal::print("Options:", terminal::MessageType::HEADER);
    std::cout << "  -h, --help          Show this help message\n"
              << "  -m, --model MODEL   Specify the model to use (default: llama3)\n"
              << "  -l, --list          List available models\n"
              << "  -t, --tools         Enable tool use with the model\n"
              << "  -d, --debug         Enable debug mode for detailed output\n"
              << "  -f, --file-ops      Enable file operations (read, write, edit)\n"
              << "  -s, --shell         Enable shell command execution tool (use with caution)\n"
              << "  --auto-confirm      Automatically confirm shell commands without prompting\n"
              << "  --auto-confirm-files  Automatically confirm file operations without prompting\n"
              << "  --ignore-calc-safety Ignore calculator safety checks for potentially dangerous patterns\n"
              << "  --ignore-shell-safety Ignore shell command safety checks for potentially dangerous operations\n"
              << "  --model-list        Enable model listing tool for the LLM\n"
              << "  --host URL          Specify Ollama host URL (default: http://localhost:11434)\n"
              << "  --config FILE       Use specified config file (default: ~/.config/neoneo/config.json)\n"
              << "  --save-config       Save current settings to config file\n"
              << "  --no-config         Ignore config file and use default settings\n";
    
    terminal::print("Examples:", terminal::MessageType::HEADER);
    std::cout << "  neoneo                 Start chat with default model (or config if available)\n"
              << "  neoneo -m llama3.3     Start chat with llama3.3 model\n"
              << "  neoneo -t              Start with tools enabled\n"
              << "  neoneo -d -t           Start with tools and debug mode\n"
              << "  neoneo -t -f           Start with tools and file operations\n"
              << "  neoneo -t -s           Start with tools and shell execution (with confirmation)\n"
              << "  neoneo -t -s --auto-confirm  Start with tools and shell execution (without confirmation)\n"
              << "  neoneo -t --model-list       Start with tools and model listing capability\n"
              << "  neoneo -t -f -s        Start with tools, file operations, and shell commands\n"
              << "  neoneo -l              List available models directly\n"
              << "  neoneo --save-config   Save current command-line settings to config file\n"
              << "  neoneo --config /path/to/config.json  Use custom config file\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    // Set up signal handler for Ctrl+C
    std::signal(SIGINT, signal_handler);
    
    // Initialize configuration with defaults
    config::Config config;
    
    // Config file variables
    std::string config_file_path = config::Config::get_default_config_path();
    bool use_config = true;
    bool save_config = false;
    
    // Command line options
    bool list_models = false; // Non-config option, just for command behavior
    
    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            print_usage();
            return 0;
        } else if (arg == "-m" || arg == "--model") {
            if (i + 1 < argc) {
                config.set_model(argv[++i]);
            } else {
                terminal::print("Error: --model requires a model name.", terminal::MessageType::ERROR);
                return 1;
            }
        } else if (arg == "-l" || arg == "--list") {
            list_models = true;
        } else if (arg == "-t" || arg == "--tools") {
            config.set_tools_enabled(true);
        } else if (arg == "-d" || arg == "--debug") {
            config.set_debug_mode(true);
        } else if (arg == "-s" || arg == "--shell") {
            config.set_shell_enabled(true);
        } else if (arg == "--auto-confirm") {
            config.set_auto_confirm_shell(true);
        } else if (arg == "--auto-confirm-files") {
            config.set_auto_confirm_file_ops(true);
        } else if (arg == "--ignore-calc-safety") {
            config.set_calc_safety_ignored(true);
        } else if (arg == "--ignore-shell-safety") {
            config.set_shell_safety_ignored(true);
        } else if (arg == "--model-list") {
            config.set_model_list_enabled(true);
        } else if (arg == "--file-ops" || arg == "-f") {
            config.set_file_ops_enabled(true);
        } else if (arg == "--host") {
            if (i + 1 < argc) {
                config.set_host(argv[++i]);
            } else {
                terminal::print("Error: --host requires a URL.", terminal::MessageType::ERROR);
                return 1;
            }
        } else if (arg == "--config") {
            if (i + 1 < argc) {
                config_file_path = argv[++i];
            } else {
                terminal::print("Error: --config requires a file path.", terminal::MessageType::ERROR);
                return 1;
            }
        } else if (arg == "--save-config") {
            save_config = true;
        } else if (arg == "--no-config") {
            use_config = false;
        } else if (i == argc - 1 && arg[0] != '-') {
            // Last argument without a flag is assumed to be the model
            config.set_model(arg);
        } else {
            terminal::print("Unknown option: " + arg, terminal::MessageType::ERROR);
            print_usage();
            return 1;
        }
    }
    
    // Load config file if enabled and no command-line options provided
    bool cli_options_provided = argc > 1 && !list_models && !save_config;
    
    if (use_config && (!cli_options_provided || save_config)) {
        bool config_loaded = config.load_from_file(config_file_path);
        
        if (config_loaded) {
            terminal::print("Loaded configuration from: " + config_file_path, terminal::MessageType::SUCCESS);
        } else if (save_config) {
            terminal::print("Creating new configuration file: " + config_file_path, terminal::MessageType::SYSTEM);
        } else {
            terminal::print("No configuration file found. Using default settings.", terminal::MessageType::SYSTEM);
        }
    }
    
    // Save config if requested
    if (save_config) {
        if (config.save_to_file(config_file_path)) {
            terminal::print("Configuration saved to: " + config_file_path, terminal::MessageType::SUCCESS);
            if (!cli_options_provided) {
                return 0; // Exit after saving if no other actions requested
            }
        } else {
            terminal::print("Failed to save configuration to: " + config_file_path, terminal::MessageType::ERROR);
            return 1;
        }
    }
    
    // Initialize the Ollama client
    OllamaClient client(config.get_host());
    
    // Check connection to Ollama server
    terminal::print("Connecting to Ollama server at " + config.get_host() + "...", terminal::MessageType::SYSTEM);
    if (!client.connect()) {
        terminal::print("Error: Could not connect to Ollama server. Is Ollama running?", terminal::MessageType::ERROR);
        return 1;
    }
    terminal::print("Connected to Ollama server.", terminal::MessageType::SUCCESS);
    
    // List models if requested
    if (list_models) {
        terminal::print("Available models:", terminal::MessageType::HEADER);
        auto models = client.list_models();
        if (models.empty()) {
            terminal::print("No models found. You may need to pull a model first.", terminal::MessageType::WARNING);
            terminal::print("Try running: ollama pull " + config.get_model(), terminal::MessageType::SYSTEM);
        } else {
            for (const auto& m : models) {
                terminal::print("  - " + m, terminal::MessageType::NORMAL);
            }
        }
        return 0;
    }
    
    // Initialize tool manager
    tools::ToolManager tool_manager(config);
    
    // Register all available tools based on configuration
    if (config.is_tools_enabled()) {
        tool_manager.register_default_tools();
        
        // Get tool definitions and print tool count
        auto tool_definitions = tool_manager.get_tool_definitions();
        
        // Show warnings for potentially dangerous tools
        if (config.is_shell_enabled()) {
            terminal::print("WARNING: Shell command execution is enabled. Use with caution.", terminal::MessageType::WARNING);
        }
        
        if (config.is_file_ops_enabled() && config.is_auto_confirm_file_ops()) {
            terminal::print("WARNING: Auto-confirmation for file operations is enabled.", terminal::MessageType::WARNING);
        }
        
        terminal::print("Tool usage enabled with " + std::to_string(tool_definitions.size()) + " available tools.", terminal::MessageType::SUCCESS);
    }
    
    // Start chat session
    terminal::print("Starting chat with model: " + config.get_model(), terminal::MessageType::HEADER);
    terminal::print("Type '/exit' to quit, '/reset' to reset the conversation.", terminal::MessageType::SYSTEM);
    terminal::print("Type '/help' for a list of available commands.", terminal::MessageType::SYSTEM);
    terminal::print(std::string(50, '-'), terminal::MessageType::NORMAL);
    
    // Initialize conversation with a system prompt to encourage planning and multiple tool use
    std::vector<ChatMessage> conversation;
    
    // Add system message to guide the model
    std::string system_prompt = 
        "You are a helpful assistant with access to various tools. "
        "When addressing complex problems, please follow these guidelines:\n\n"
        
        "1. PLAN FIRST: When tackling a complex task, first develop a clear plan with sequential steps.\n"
        "2. MULTIPLE TOOLS: Consider using multiple tools in sequence to solve problems efficiently.\n"
        "3. EXPLAIN YOUR APPROACH: Before executing any tools, briefly explain your plan.\n"
        "4. PROVIDE CONTEXT: For each tool call, explain what you're trying to accomplish.\n"
        "5. SUMMARIZE RESULTS: After tool execution, summarize what you've learned and what to do next.\n\n"
        
        "IMPORTANT: When you need to use multiple commands or operations, don't execute them one by one. "
        "Instead, provide a comprehensive plan with all needed commands so the user can review the entire "
        "approach before execution. This is especially important for complex tasks involving system changes.";
    
    conversation.push_back(ChatMessage("system", system_prompt));
    
    while (running) {
        // Display prompt and get user input
        char prompt_buffer[20];
        snprintf(prompt_buffer, sizeof(prompt_buffer), "\n%s> %s", terminal::get_message_color(terminal::MessageType::USER).c_str(), terminal::get_color_code(terminal::Color::RESET).c_str());
        char* input_cstr = readline(prompt_buffer);
        
        // Check if EOF (Ctrl+D) or error
        if (!input_cstr) {
            running = false;
            std::cout << std::endl;
            break;
        }
        
        std::string input(input_cstr);
        
        // Add to readline history
        if (!input.empty()) {
            add_history(input_cstr);
        }
        
        // Free readline allocated memory
        free(input_cstr);
        
        // Check for commands
        if (input == "/exit" || input == "/quit") {
            break;
        } else if (input == "/reset") {
            // Clear conversation but preserve the system prompt
            conversation.clear();
            conversation.push_back(ChatMessage("system", system_prompt));
            terminal::print("Conversation reset.", terminal::MessageType::SUCCESS);
            continue;
        } else if (input == "/tools") {
            if (config.is_tools_enabled()) {
                terminal::print("Available tools:", terminal::MessageType::HEADER);
                auto tool_definitions = tool_manager.get_tool_definitions();
                for (const auto& tool_def : tool_definitions) {
                    const auto& function = tool_def["function"];
                    terminal::print("  - " + function["name"].get<std::string>() + ": " 
                              + function["description"].get<std::string>(), terminal::MessageType::TOOL);
                    
                    // Print parameters
                    if (function["parameters"].contains("properties")) {
                        terminal::print("    Parameters:", terminal::MessageType::NORMAL);
                        for (auto& [param_name, param_info] : function["parameters"]["properties"].items()) {
                            std::string desc = param_info.contains("description") ? 
                                               param_info["description"].get<std::string>() : "";
                            terminal::print("      * " + param_name + ": " + desc, terminal::MessageType::NORMAL);
                        }
                    }
                    std::cout << std::endl;
                }
            } else {
                terminal::print("No tools are available. Start the application with -t to enable tools.", terminal::MessageType::WARNING);
            }
            continue;
        } else if (input == "/help") {
            terminal::print("Available commands:", terminal::MessageType::HEADER);
            terminal::print("  /exit, /quit   - Exit the application", terminal::MessageType::NORMAL);
            terminal::print("  /reset         - Reset the conversation history", terminal::MessageType::NORMAL);
            terminal::print("  /help          - Show this help message", terminal::MessageType::NORMAL);
            terminal::print("  /models        - List available models on the Ollama server", terminal::MessageType::NORMAL);
            terminal::print("  /config        - Show current configuration", terminal::MessageType::NORMAL);
            terminal::print("  /template      - Show the conversation template being sent to the LLM", terminal::MessageType::NORMAL);
            terminal::print("  /prompt        - Show the current system prompt", terminal::MessageType::NORMAL);
            terminal::print("  /setprompt     - Set a new system prompt", terminal::MessageType::NORMAL);
            if (config.is_tools_enabled()) {
                terminal::print("  /tools         - List available tools", terminal::MessageType::TOOL);
            }
            std::cout << std::endl;
            continue;
        } else if (input == "/config") {
            terminal::print("Current configuration:", terminal::MessageType::HEADER);
            terminal::print("  Model:           " + config.get_model(), terminal::MessageType::NORMAL);
            terminal::print("  Host:            " + config.get_host(), terminal::MessageType::NORMAL);
            
            // Use colors based on status for these settings
            terminal::MessageType toolsType = config.is_tools_enabled() ? terminal::MessageType::SUCCESS : terminal::MessageType::NORMAL;
            terminal::MessageType debugType = config.is_debug_mode() ? terminal::MessageType::SUCCESS : terminal::MessageType::NORMAL;
            terminal::MessageType shellType = config.is_shell_enabled() ? terminal::MessageType::WARNING : terminal::MessageType::NORMAL;
            terminal::MessageType autoShellType = config.is_auto_confirm_shell() ? terminal::MessageType::WARNING : terminal::MessageType::NORMAL;
            terminal::MessageType fileOpsType = config.is_file_ops_enabled() ? terminal::MessageType::WARNING : terminal::MessageType::NORMAL;
            terminal::MessageType autoFilesType = config.is_auto_confirm_file_ops() ? terminal::MessageType::WARNING : terminal::MessageType::NORMAL;
            terminal::MessageType ignoreCalcType = config.is_calc_safety_ignored() ? terminal::MessageType::WARNING : terminal::MessageType::NORMAL;
            terminal::MessageType ignoreShellType = config.is_shell_safety_ignored() ? terminal::MessageType::WARNING : terminal::MessageType::NORMAL;
            
            terminal::print("  Tools enabled:   " + std::string(config.is_tools_enabled() ? "Yes" : "No"), toolsType);
            terminal::print("  Debug mode:      " + std::string(config.is_debug_mode() ? "Yes" : "No"), debugType);
            terminal::print("  Shell enabled:   " + std::string(config.is_shell_enabled() ? "Yes" : "No"), shellType);
            terminal::print("  Auto-confirm shell: " + std::string(config.is_auto_confirm_shell() ? "Yes" : "No"), autoShellType);
            terminal::print("  Model list tool: " + std::string(config.is_model_list_enabled() ? "Yes" : "No"), terminal::MessageType::NORMAL);
            terminal::print("  File ops enabled:" + std::string(config.is_file_ops_enabled() ? "Yes" : "No"), fileOpsType);
            terminal::print("  Auto-confirm files: " + std::string(config.is_auto_confirm_file_ops() ? "Yes" : "No"), autoFilesType);
            terminal::print("  Ignore calc safety: " + std::string(config.is_calc_safety_ignored() ? "Yes" : "No"), ignoreCalcType);
            terminal::print("  Ignore shell safety: " + std::string(config.is_shell_safety_ignored() ? "Yes" : "No"), ignoreShellType);
            
            if (!config.get_config_file_path().empty()) {
                terminal::print("  Config file:     " + config.get_config_file_path(), terminal::MessageType::NORMAL);
            }
            
            std::cout << std::endl;
            terminal::print("To save this configuration, run with --save-config", terminal::MessageType::SYSTEM);
            std::cout << std::endl;
            continue;
        } else if (input == "/models") {
            // Manually list available models
            terminal::print("Available models on Ollama server at " + config.get_host() + ":", terminal::MessageType::HEADER);
            auto models = client.list_models();
            if (models.empty()) {
                terminal::print("No models found.", terminal::MessageType::WARNING);
            } else {
                for (size_t i = 0; i < models.size(); i++) {
                    terminal::print("  " + std::to_string(i+1) + ". " + models[i], terminal::MessageType::NORMAL);
                }
            }
            std::cout << std::endl;
            continue;
        } else if (input == "/prompt") {
            // Display the current system prompt
            terminal::print("Current system prompt:", terminal::MessageType::HEADER);
            terminal::print("==========================", terminal::MessageType::NORMAL);
            
            // Find the system message in the conversation
            bool found = false;
            for (const auto& msg : conversation) {
                if (msg.role == "system") {
                    terminal::print(msg.content, terminal::MessageType::SYSTEM);
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                terminal::print("No system prompt found in the conversation.", terminal::MessageType::WARNING);
            }
            
            terminal::print("==========================", terminal::MessageType::NORMAL);
            continue;
        } else if (input == "/setprompt") {
            terminal::print("Enter new system prompt (type '/end' on a new line when finished):", terminal::MessageType::HEADER);
            
            std::string new_prompt;
            std::string line;
            while (true) {
                char* prompt_line = readline("> ");
                if (!prompt_line) break;
                
                line = prompt_line;
                free(prompt_line);
                
                if (line == "/end") break;
                
                new_prompt += line + "\n";
            }
            
            if (!new_prompt.empty()) {
                // Remove trailing newline
                if (new_prompt.back() == '\n') {
                    new_prompt.pop_back();
                }
                
                // Update the system prompt
                system_prompt = new_prompt;
                
                // Find and replace the system message in the conversation
                bool found = false;
                for (auto& msg : conversation) {
                    if (msg.role == "system") {
                        msg.content = system_prompt;
                        found = true;
                        break;
                    }
                }
                
                // If no system message was found, add one
                if (!found) {
                    conversation.insert(conversation.begin(), ChatMessage("system", system_prompt));
                }
                
                terminal::print("System prompt updated.", terminal::MessageType::SUCCESS);
            } else {
                terminal::print("No changes made to system prompt.", terminal::MessageType::WARNING);
            }
            continue;
        } else if (input == "/template") {
            if (conversation.empty()) {
                terminal::print("Conversation is empty. No template to display.", terminal::MessageType::WARNING);
                continue;
            }
            
            terminal::print("Current conversation template:", terminal::MessageType::HEADER);
            terminal::print("==========================", terminal::MessageType::NORMAL);
            
            for (const auto& msg : conversation) {
                terminal::print("ROLE: " + msg.role, terminal::MessageType::SYSTEM);
                
                if (!msg.name.empty()) {
                    terminal::print("NAME: " + msg.name, terminal::MessageType::SYSTEM);
                }
                
                terminal::print("CONTENT:", terminal::MessageType::SYSTEM);
                if (msg.role == "user") {
                    terminal::print(msg.content, terminal::MessageType::USER);
                } else if (msg.role == "assistant") {
                    terminal::print(msg.content, terminal::MessageType::MODEL);
                } else {
                    terminal::print(msg.content, terminal::MessageType::NORMAL);
                }
                
                if (!msg.tool_calls.empty()) {
                    terminal::print("TOOL CALLS:", terminal::MessageType::SYSTEM);
                    for (const auto& tool : msg.tool_calls) {
                        std::string tool_info = "  - " + tool.name;
                        if (!tool.id.empty()) {
                            tool_info += " (ID: " + tool.id + ")";
                        }
                        terminal::print(tool_info, terminal::MessageType::TOOL);
                        
                        terminal::print("    Arguments: " + tool.arguments.dump(2), terminal::MessageType::NORMAL);
                    }
                }
                
                terminal::print("--------------------------", terminal::MessageType::NORMAL);
            }
            
            terminal::print("==========================", terminal::MessageType::NORMAL);
            
            // Print tools if enabled
            if (config.is_tools_enabled()) {
                auto tool_definitions = tool_manager.get_tool_definitions();
                if (!tool_definitions.empty()) {
                    terminal::print("Tools provided with this template:", terminal::MessageType::HEADER);
                    for (const auto& tool : tool_definitions) {
                        terminal::print("  - " + tool["function"]["name"].get<std::string>() + ": " 
                                  + tool["function"]["description"].get<std::string>(), terminal::MessageType::TOOL);
                    }
                    std::cout << std::endl;
                }
            }
            
            continue;
        } else if (input.empty()) {
            continue;
        }
        
        // Add user message to conversation
        conversation.push_back(ChatMessage("user", input));
        
        // Send to Ollama and get response
        std::cout << std::endl;
        
        // Variable to control whether we're using tools or not
        bool using_tools = config.is_tools_enabled();
        bool streaming_enabled = true; // User preference flag (could be added to config)
        
        // Get tool definitions if tools are enabled
        auto tool_definitions = using_tools ? 
                              tool_manager.get_tool_definitions() : 
                              std::vector<nlohmann::json>{};
        
        // Create a response object to hold the result
        ChatMessage response("assistant", "");
        
        if (streaming_enabled) {
            // For capturing the entire streamed response
            std::string full_content;
            
            // Create lambda to handle print_streaming_response with MessageType
            // and accumulate the full response
            auto callback = [&full_content](const std::string& chunk) {
                full_content += chunk;
                terminal::print_streaming_response(chunk, terminal::MessageType::MODEL);
            };
            
            // Stream the response for better UX
            terminal::print("Streaming response from " + config.get_model() + ":", terminal::MessageType::SYSTEM);
            client.chat_stream(config.get_model(), conversation, callback, tool_definitions);
            std::cout << std::endl;
            
            // If we're using tools, we need to get the complete response with tool calls
            if (using_tools) {
                // Get the full response with tool calls
                response = client.chat(config.get_model(), conversation, tool_definitions);
                
                // If there are no tool calls, use the streamed content instead
                if (response.tool_calls.empty()) {
                    response.content = full_content;
                }
            } else {
                // If not using tools, just use the accumulated content
                response = ChatMessage("assistant", full_content);
            }
        } else {
            // Non-streaming mode - just get the complete response
            response = client.chat(config.get_model(), conversation, tool_definitions);
        }
        
        // Handle tool calls if present
        if (using_tools && !response.tool_calls.empty()) {
            terminal::print("Model " + config.get_model() + " is using tools to respond...", terminal::MessageType::SYSTEM);
            
            for (const auto& tool_call : response.tool_calls) {
                terminal::print("Model " + config.get_model() + " is calling tool: " + tool_call.name, terminal::MessageType::TOOL);
                
                // Check if tool exists
                if (!tool_manager.has_tool(tool_call.name)) {
                    terminal::print("Tool not found: " + tool_call.name, terminal::MessageType::ERROR);
                    continue;
                }
                
                // Print tool arguments
                if (config.is_debug_mode()) {
                    terminal::print("Tool arguments (detailed):", terminal::MessageType::SYSTEM);
                    terminal::print(tool_call.arguments.dump(2), terminal::MessageType::NORMAL);
                } else {
                    terminal::print("Tool arguments: " + tool_call.arguments.dump(), terminal::MessageType::NORMAL);
                }
                
                // Execute the tool and get result
                tools::ToolResult result = tool_manager.execute_tool(tool_call.name, tool_call.arguments);
                
                // Display result
                if (result.is_success) {
                    terminal::print("Tool result:", terminal::MessageType::SUCCESS);
                    terminal::print(result.content, terminal::MessageType::TOOL);
                } else {
                    terminal::print("Tool error:", terminal::MessageType::ERROR);
                    terminal::print(result.error_message, terminal::MessageType::ERROR);
                }
                
                // Add tool result to conversation
                conversation.push_back(ChatMessage::make_tool_response(
                    result.is_success ? result.content : result.error_message, 
                    tool_call.name
                ));
            }
            
            // Get the final response after tool execution
            if (config.is_debug_mode()) {
                terminal::print("Getting final response with tool results...", terminal::MessageType::SYSTEM);
            }
            
            // Stream the final response if enabled
            if (streaming_enabled) {
                std::string final_content;
                auto final_callback = [&final_content](const std::string& chunk) {
                    final_content += chunk;
                    terminal::print_streaming_response(chunk, terminal::MessageType::MODEL);
                };
                
                terminal::print("Final response after tool execution:", terminal::MessageType::HEADER);
                client.chat_stream(config.get_model(), conversation, final_callback, tool_definitions);
                std::cout << std::endl;
                
                // Update the response with the final content
                response = ChatMessage("assistant", final_content);
            } else {
                // Non-streaming final response
                response = client.chat(config.get_model(), conversation, tool_definitions);
                terminal::print("Final response after tool execution:", terminal::MessageType::HEADER);
                terminal::print(response.content, terminal::MessageType::MODEL);
            }
        }
        
        // Add assistant response to conversation
        conversation.push_back(response);
    }
    
    terminal::print("Goodbye!", terminal::MessageType::SUCCESS);
    return 0;
}