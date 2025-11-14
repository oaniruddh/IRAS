#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

class Config {
private:
    std::map<std::string, std::string> settings;
    std::string configFile;
    
public:
    Config(const std::string& filename = "config.ini") : configFile(filename) {
        LoadConfig();
    }
    
    ~Config() {
        SaveConfig();
    }
    
    // Load configuration from file
    bool LoadConfig() {
        std::ifstream file(configFile);
        if (!file.is_open()) {
            // Create default config if file doesn't exist
            CreateDefaultConfig();
            return false;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#' || line[0] == ';') {
                continue;
            }
            
            // Find the = separator
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                
                // Trim whitespace
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                
                settings[key] = value;
            }
        }
        
        file.close();
        return true;
    }
    
    // Save configuration to file
    bool SaveConfig() {
        std::ofstream file(configFile);
        if (!file.is_open()) {
            return false;
        }
        
        file << "# IRAS System Monitor Configuration\n";
        file << "# Auto-generated configuration file\n\n";
        
        for (const auto& pair : settings) {
            file << pair.first << "=" << pair.second << "\n";
        }
        
        file.close();
        return true;
    }
    
    // Create default configuration
    void CreateDefaultConfig() {
        settings["monitoring_interval"] = "5";
        settings["max_history_size"] = "1000";
        settings["auto_start_monitoring"] = "false";
        settings["show_console_output"] = "true";
        settings["log_level"] = "INFO";
        settings["export_format"] = "JSON";
        settings["window_width"] = "900";
        settings["window_height"] = "600";
        settings["theme"] = "default";
        settings["cpu_alert_threshold"] = "80";
        settings["memory_alert_threshold"] = "85";
        settings["disk_alert_threshold"] = "90";
        settings["enable_notifications"] = "true";
        settings["refresh_rate_ms"] = "1000";
        
        SaveConfig();
    }
    
    // Get configuration value as string
    std::string Get(const std::string& key, const std::string& defaultValue = "") const {
        auto it = settings.find(key);
        return (it != settings.end()) ? it->second : defaultValue;
    }
    
    // Get configuration value as integer
    int GetInt(const std::string& key, int defaultValue = 0) const {
        std::string value = Get(key);
        if (value.empty()) {
            return defaultValue;
        }
        
        try {
            return std::stoi(value);
        } catch (const std::exception&) {
            return defaultValue;
        }
    }
    
    // Get configuration value as double
    double GetDouble(const std::string& key, double defaultValue = 0.0) const {
        std::string value = Get(key);
        if (value.empty()) {
            return defaultValue;
        }
        
        try {
            return std::stod(value);
        } catch (const std::exception&) {
            return defaultValue;
        }
    }
    
    // Get configuration value as boolean
    bool GetBool(const std::string& key, bool defaultValue = false) const {
        std::string value = Get(key);
        if (value.empty()) {
            return defaultValue;
        }
        
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
        return (value == "true" || value == "1" || value == "yes" || value == "on");
    }
    
    // Set configuration value
    void Set(const std::string& key, const std::string& value) {
        settings[key] = value;
    }
    
    void Set(const std::string& key, int value) {
        settings[key] = std::to_string(value);
    }
    
    void Set(const std::string& key, double value) {
        settings[key] = std::to_string(value);
    }
    
    void Set(const std::string& key, bool value) {
        settings[key] = value ? "true" : "false";
    }
    
    // Check if key exists
    bool HasKey(const std::string& key) const {
        return settings.find(key) != settings.end();
    }
    
    // Get all settings
    const std::map<std::string, std::string>& GetAllSettings() const {
        return settings;
    }
    
    // Print all settings (for debugging)
    void PrintSettings() const {
        std::cout << "Current Configuration:\n";
        std::cout << "=====================\n";
        for (const auto& pair : settings) {
            std::cout << pair.first << " = " << pair.second << "\n";
        }
        std::cout << "=====================\n";
    }
};

#endif // CONFIG_H
