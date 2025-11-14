#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <mutex>

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    CRITICAL = 4
};

class Logger {
private:
    std::string logFile;
    LogLevel currentLevel;
    bool consoleOutput;
    std::mutex logMutex;
    
    std::string GetLogLevelString(LogLevel level) const {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARNING: return "WARNING";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::CRITICAL: return "CRITICAL";
            default: return "UNKNOWN";
        }
    }
    
    std::string GetCurrentTimestamp() const {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }
    
public:
    Logger(const std::string& filename = "system_monitor.log", 
           LogLevel level = LogLevel::INFO, 
           bool enableConsoleOutput = true) 
        : logFile(filename), currentLevel(level), consoleOutput(enableConsoleOutput) {
    }
    
    void SetLogLevel(LogLevel level) {
        currentLevel = level;
    }
    
    void SetConsoleOutput(bool enable) {
        consoleOutput = enable;
    }
    
    void SetLogFile(const std::string& filename) {
        logFile = filename;
    }
    
    void Log(LogLevel level, const std::string& message) {
        if (level < currentLevel) {
            return; // Skip if log level is below current threshold
        }
        
        std::lock_guard<std::mutex> lock(logMutex);
        
        std::string timestamp = GetCurrentTimestamp();
        std::string levelStr = GetLogLevelString(level);
        
        std::stringstream logEntry;
        logEntry << "[" << timestamp << "] [" << levelStr << "] " << message;
        
        // Write to console if enabled
        if (consoleOutput) {
            std::cout << logEntry.str() << std::endl;
        }
        
        // Write to file
        std::ofstream file(logFile, std::ios::app);
        if (file.is_open()) {
            file << logEntry.str() << std::endl;
            file.close();
        }
    }
    
    // Convenience methods
    void Debug(const std::string& message) {
        Log(LogLevel::DEBUG, message);
    }
    
    void Info(const std::string& message) {
        Log(LogLevel::INFO, message);
    }
    
    void Warning(const std::string& message) {
        Log(LogLevel::WARNING, message);
    }
    
    void Error(const std::string& message) {
        Log(LogLevel::ERROR, message);
    }
    
    void Critical(const std::string& message) {
        Log(LogLevel::CRITICAL, message);
    }
    
    // Template for formatted logging
    template<typename... Args>
    void LogF(LogLevel level, const std::string& format, Args... args) {
        std::stringstream ss;
        LogFormatHelper(ss, format, args...);
        Log(level, ss.str());
    }
    
    template<typename... Args>
    void InfoF(const std::string& format, Args... args) {
        LogF(LogLevel::INFO, format, args...);
    }
    
    template<typename... Args>
    void WarningF(const std::string& format, Args... args) {
        LogF(LogLevel::WARNING, format, args...);
    }
    
    template<typename... Args>
    void ErrorF(const std::string& format, Args... args) {
        LogF(LogLevel::ERROR, format, args...);
    }
    
private:
    // Simple format helper (basic replacement)
    template<typename T>
    void LogFormatHelper(std::stringstream& ss, const std::string& format, T&& arg) {
        size_t pos = format.find("{}");
        if (pos != std::string::npos) {
            ss << format.substr(0, pos) << arg << format.substr(pos + 2);
        } else {
            ss << format;
        }
    }
    
    template<typename T, typename... Args>
    void LogFormatHelper(std::stringstream& ss, const std::string& format, T&& arg, Args&&... args) {
        size_t pos = format.find("{}");
        if (pos != std::string::npos) {
            std::string newFormat = format.substr(0, pos) + std::to_string(arg) + format.substr(pos + 2);
            LogFormatHelper(ss, newFormat, args...);
        } else {
            ss << format;
        }
    }
};

// Global logger instance
extern Logger g_logger;

// Convenience macros
#define LOG_DEBUG(msg) g_logger.Debug(msg)
#define LOG_INFO(msg) g_logger.Info(msg)
#define LOG_WARNING(msg) g_logger.Warning(msg)
#define LOG_ERROR(msg) g_logger.Error(msg)
#define LOG_CRITICAL(msg) g_logger.Critical(msg)

#define LOG_INFO_F(format, ...) g_logger.InfoF(format, __VA_ARGS__)
#define LOG_WARNING_F(format, ...) g_logger.WarningF(format, __VA_ARGS__)
#define LOG_ERROR_F(format, ...) g_logger.ErrorF(format, __VA_ARGS__)

#endif // LOGGER_H
