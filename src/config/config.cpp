#include "../../include/neoneo/config/config.hpp"
#include <fstream>
#include <iostream>

namespace neoneo {
namespace config {

namespace fs = std::filesystem;

Config::Config() = default;

Config Config::load_from_path(const std::string& path) {
    Config config;
    config.load_from_file(path);
    return config;
}

Config Config::create_default() {
    return Config();
}

bool Config::load_from_file(const std::string& path) {
    try {
        // Check if file exists
        if (!fs::exists(path)) {
            return false;
        }
        
        // Read the file
        std::ifstream file(path);
        if (!file.is_open()) {
            return false;
        }
        
        // Parse JSON
        nlohmann::json config_json;
        file >> config_json;
        
        // Update configuration from json
        *this = Config::from_json(config_json);
        config_file_path = path;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading config file: " << e.what() << std::endl;
        return false;
    }
}

bool Config::save_to_file(const std::string& path) const {
    try {
        // Create directory if it doesn't exist
        auto dir_path = fs::path(path).parent_path();
        if (!dir_path.empty() && !fs::exists(dir_path)) {
            fs::create_directories(dir_path);
        }
        
        // Write to file
        std::ofstream file(path);
        if (!file.is_open()) {
            return false;
        }
        
        file << to_json().dump(4); // Pretty print with 4 spaces
        file.close();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving config file: " << e.what() << std::endl;
        return false;
    }
}

std::string Config::get_default_config_path() {
    std::string home_dir;
    
    // Try to get home directory
    const char* home = std::getenv("HOME");
    if (home) {
        home_dir = home;
    } else {
        // Fallback to current directory
        home_dir = fs::current_path().string();
    }
    
    return home_dir + "/.config/neoneo/config.json";
}

nlohmann::json Config::to_json() const {
    return {
        {"model", model},
        {"host", host},
        {"enable_tools", enable_tools},
        {"debug_mode", debug_mode},
        {"enable_shell", enable_shell},
        {"auto_confirm_shell", auto_confirm_shell},
        {"enable_model_list", enable_model_list},
        {"enable_file_ops", enable_file_ops},
        {"auto_confirm_file_ops", auto_confirm_file_ops},
        {"ignore_calc_safety", ignore_calc_safety},
        {"ignore_shell_safety", ignore_shell_safety}
    };
}

Config Config::from_json(const nlohmann::json& json) {
    Config config;
    
    // Update configuration
    if (json.contains("model") && json["model"].is_string()) {
        config.model = json["model"].get<std::string>();
    }
    
    if (json.contains("host") && json["host"].is_string()) {
        config.host = json["host"].get<std::string>();
    }
    
    if (json.contains("enable_tools") && json["enable_tools"].is_boolean()) {
        config.enable_tools = json["enable_tools"].get<bool>();
    }
    
    if (json.contains("debug_mode") && json["debug_mode"].is_boolean()) {
        config.debug_mode = json["debug_mode"].get<bool>();
    }
    
    if (json.contains("enable_shell") && json["enable_shell"].is_boolean()) {
        config.enable_shell = json["enable_shell"].get<bool>();
    }
    
    if (json.contains("auto_confirm_shell") && json["auto_confirm_shell"].is_boolean()) {
        config.auto_confirm_shell = json["auto_confirm_shell"].get<bool>();
    }
    
    if (json.contains("enable_model_list") && json["enable_model_list"].is_boolean()) {
        config.enable_model_list = json["enable_model_list"].get<bool>();
    }
    
    if (json.contains("enable_file_ops") && json["enable_file_ops"].is_boolean()) {
        config.enable_file_ops = json["enable_file_ops"].get<bool>();
    }
    
    if (json.contains("auto_confirm_file_ops") && json["auto_confirm_file_ops"].is_boolean()) {
        config.auto_confirm_file_ops = json["auto_confirm_file_ops"].get<bool>();
    }
    
    if (json.contains("ignore_calc_safety") && json["ignore_calc_safety"].is_boolean()) {
        config.ignore_calc_safety = json["ignore_calc_safety"].get<bool>();
    }
    
    if (json.contains("ignore_shell_safety") && json["ignore_shell_safety"].is_boolean()) {
        config.ignore_shell_safety = json["ignore_shell_safety"].get<bool>();
    }
    
    return config;
}

} // namespace config
} // namespace neoneo