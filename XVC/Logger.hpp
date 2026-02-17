#pragma once

// ============================================================================
/// @file Logger.hpp
/// @brief Thread-safe logging system with console color support
/// @provides File and console logging with multiple severity levels
// ============================================================================

// ============================================================================
// DEPENDENCIES
// ============================================================================
#include <windows.h>

#include <atomic>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================
enum class LogLevel;
class Logger;

// ============================================================================
/// @enum LogLevel
/// @brief Log severity levels (higher number = more severe)
// ============================================================================
enum class LogLevel {
    DEBUG_LEVEL = 0,  ///< Detailed debug information
    INFO_LEVEL = 1,  ///< General information messages
    WARNING_LEVEL = 2,  ///< Warning messages (non-critical)
    ERROR_LEVEL = 3,  ///< Error messages (recoverable)
    FATAL_LEVEL = 4   ///< Fatal errors (unrecoverable)
};

// ============================================================================
/// @class Logger
/// @brief Thread-safe logging singleton with file and console output
/// @details Uses mutex protection for thread safety and color-coded console output
// ============================================================================
class Logger {
private:
    // ------------------------------------------------------------------------
    /// @name Static Members
    /// @{
    static std::ofstream      logFile_;       ///< Log file output stream
    static std::mutex         logMutex_;      ///< Mutex for thread safety
    static std::atomic<bool>  initialized_;   ///< Initialization flag
    static LogLevel           currentLevel_;  ///< Current log level filter
    static std::string        logFileName_;   ///< Current log file name
    /// @}

public:
    // ------------------------------------------------------------------------
    /// @name Lifetime Management
    /// @{
    /// @brief Initialize the logging system
    /// @param file Log file path (default: "xvcpp_emulator.log")
    /// @param level Minimum log level to record (default: INFO)
    /// @return true if initialization successful
    static bool Initialize(const std::string& file = "xvcpp_emulator.log",
        LogLevel level = LogLevel::INFO_LEVEL) {
        std::lock_guard lock(logMutex_);
        if (initialized_.exchange(true)) return true;

        logFileName_ = file;
        currentLevel_ = level;

        logFile_.open(file, std::ios::out | std::ios::trunc);
        if (!logFile_.is_open()) {
            initialized_ = false;
            std::cerr << "[LOGGER] Cannot create " << file << "\n";
            return false;
        }

        WriteHeader();
        return true;
    }

    /// @brief Shutdown the logging system and close log file
    static void Shutdown() {
        std::lock_guard lock(logMutex_);
        if (!initialized_.load()) return;
        if (logFile_.is_open()) {
            WriteFooter();
            logFile_.close();
        }
        initialized_ = false;
    }

    /// @brief Check if logger is initialized
    [[nodiscard]] static bool IsInitialized() { return initialized_.load(); }

    /// @brief Get current log file name
    [[nodiscard]] static std::string GetLogFileName() { return logFileName_; }
    /// @}

    // ------------------------------------------------------------------------
    /// @name Core Logging
    /// @{
    /// @brief Log a message with specified level
    /// @param level Severity level of the message
    /// @param msg Message to log
    /// @param toConsole Force console output (ignores level filter)
    static void Log(LogLevel level, const std::string& msg, bool toConsole = true) {
        if (!initialized_.load() || level < currentLevel_) {
            if (level >= LogLevel::ERROR_LEVEL)
                std::cerr << "[PRE] " << msg << "\n";
            return;
        }

        std::lock_guard lock(logMutex_);

        try {
            std::string ts = Timestamp();
            std::string ls = LevelStr(level);
            std::string line = "[" + ts + "] [" + ls + "] " + msg;

            if (logFile_.is_open()) {
                logFile_ << line << "\n";
                logFile_.flush();
            }

            if (toConsole || level >= LogLevel::WARNING_LEVEL)
                ConsoleOutput(level, ls, msg);
        }
        catch (const std::exception& e) {
            std::cerr << "[LOGGER] " << e.what() << "\n"
                << "Original: " << msg << "\n";
        }
    }
    /// @}

    // ------------------------------------------------------------------------
    /// @name Convenience Methods
    /// @{
    /// @brief Log debug message (not shown in console by default)
    static void Debug(const std::string& m) { Log(LogLevel::DEBUG_LEVEL, m, false); }

    /// @brief Log info message (not shown in console by default)
    static void Info(const std::string& m) { Log(LogLevel::INFO_LEVEL, m, false); }

    /// @brief Log warning message (shown in console)
    static void Warning(const std::string& m) { Log(LogLevel::WARNING_LEVEL, m, true); }

    /// @brief Log error message (shown in console)
    static void Error(const std::string& m) { Log(LogLevel::ERROR_LEVEL, m, true); }

    /// @brief Log fatal error message (shown in console)
    static void Fatal(const std::string& m) { Log(LogLevel::FATAL_LEVEL, m, true); }

    /// @brief Set minimum log level
    static void SetLogLevel(LogLevel l) {
        std::lock_guard lock(logMutex_);
        currentLevel_ = l;
    }

    /// @brief Get current minimum log level
    [[nodiscard]] static LogLevel GetLogLevel() { return currentLevel_; }

    /// @brief Log a section header (decorative)
    static void LogSection(const std::string& name) {
        Log(LogLevel::INFO_LEVEL, "=== " + name + " ===", true);
    }
    /// @}

private:
    // ------------------------------------------------------------------------
    /// @name File Operations
    /// @{
    /// @brief Write log file header
    static void WriteHeader() {
        logFile_ << "==================================================\n"
            << "XVC++ LOG - Started: " << Timestamp() << "\n"
            << "Level: " << LevelStr(currentLevel_) << "\n"
            << "==================================================\n";
        logFile_.flush();
    }

    /// @brief Write log file footer
    static void WriteFooter() {
        logFile_ << "==================================================\n"
            << "XVC++ LOG - Finished: " << Timestamp() << "\n"
            << "==================================================\n";
    }
    /// @}

    // ------------------------------------------------------------------------
    /// @name Formatting Helpers
    /// @{
    /// @brief Get current timestamp string (YYYY-MM-DD HH:MM:SS.mmm)
    [[nodiscard]] static std::string Timestamp() {
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::tm tm;
        localtime_s(&tm, &t);

        std::stringstream ss;
        ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
            << "." << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }

    /// @brief Convert log level to string
    [[nodiscard]] static std::string LevelStr(LogLevel l) {
        switch (l) {
        case LogLevel::DEBUG_LEVEL:   return "DEBUG";
        case LogLevel::INFO_LEVEL:    return "INFO";
        case LogLevel::WARNING_LEVEL: return "WARN";
        case LogLevel::ERROR_LEVEL:   return "ERROR";
        case LogLevel::FATAL_LEVEL:   return "FATAL";
        default:                       return "UNKNOWN";
        }
    }
    /// @}

    // ------------------------------------------------------------------------
    /// @name Console Output
    /// @{
    /// @brief Set console text color based on log level
    static void SetColor(LogLevel l) {
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        if (h == INVALID_HANDLE_VALUE) return;

        WORD color = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
        switch (l) {
        case LogLevel::DEBUG_LEVEL:
            color = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
            break;
        case LogLevel::INFO_LEVEL:
            color = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
            break;
        case LogLevel::WARNING_LEVEL:
            color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
            break;
        case LogLevel::ERROR_LEVEL:
        case LogLevel::FATAL_LEVEL:
            color = FOREGROUND_RED | FOREGROUND_INTENSITY;
            break;
        }
        SetConsoleTextAttribute(h, color);
    }

    /// @brief Output message to console with appropriate color
    static void ConsoleOutput(LogLevel l, const std::string& lstr, const std::string& msg) {
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO info;
        WORD orig = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
        if (GetConsoleScreenBufferInfo(h, &info))
            orig = info.wAttributes;

        SetColor(l);
        if (l >= LogLevel::ERROR_LEVEL)
            std::cerr << "[" << lstr << "] " << msg << "\n";
        else
            std::cout << "[" << lstr << "] " << msg << "\n";
        SetConsoleTextAttribute(h, orig);
    }
    /// @}
};

// ============================================================================
/// @name Static Initialization
/// @{
// ============================================================================
inline std::ofstream      Logger::logFile_;
inline std::mutex         Logger::logMutex_;
inline std::atomic<bool>  Logger::initialized_{ false };
inline LogLevel           Logger::currentLevel_ = LogLevel::INFO_LEVEL;
inline std::string        Logger::logFileName_ = "xvcpp_emulator.log";
/// @}