#pragma once

// ============================================================================
/// @file Config.hpp
/// @brief Thread-safe configuration management for XVC++ emulator
/// @provides Persistent settings, button mappings, and runtime configuration
// ============================================================================

// ============================================================================
// DEPENDENCIES
// ============================================================================
#include <windows.h>

#include <algorithm>
#include <atomic>
#include <fstream>
#include <iostream>
#include <map>
#include <shared_mutex>
#include <string>
#include <vector>

#include "Key.hpp"
#include "Logger.hpp"

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================
class EmulationState;
class ThreadSafeConfig;

// ============================================================================
/// @enum BoostDirection
/// @brief Direction modes for boost functionality in absolute mode
// ============================================================================
enum class BoostDirection {
    TO_CENTER = 0,  ///< Boost only when moving toward center
    TO_EDGE = 1,  ///< Boost only when moving toward edges
    BOTH = 2,  ///< Boost in all directions
    NONE = 3   ///< No boost (standard absolute mode)
};

// ============================================================================
/// @class ThreadSafeConfig
/// @brief Thread-safe configuration container with INI persistence
/// @details Uses shared_mutex for concurrent read access and unique_lock for writes
// ============================================================================
class ThreadSafeConfig {
private:
    // ------------------------------------------------------------------------
    /// @name Constants
    /// @{
    // ------------------------------------------------------------------------
    static constexpr const char* INI_FILE_PATH = ".\\Mapping.ini";

    /// Smoothing parameters
    static constexpr float DEFAULT_SMOOTHING = 0.15f;
    static constexpr float MIN_SMOOTHING = 0.01f;
    static constexpr float MAX_SMOOTHING = 1.0f;

    /// Spring mode parameters
    static constexpr float DEFAULT_SPRING_SPEED = 0.15f;
    static constexpr float MIN_SPRING_SPEED = 0.01f;
    static constexpr float MAX_SPRING_SPEED = 1.0f;

    /// Boost parameters
    static constexpr float DEFAULT_FAST_RETURN_BOOST = 2.5f;
    static constexpr float MIN_FAST_RETURN_BOOST = 1.1f;
    static constexpr float MAX_FAST_RETURN_BOOST = 5.0f;

    /// Dynamic boost thresholds
    static constexpr float DEFAULT_SLOW_SPEED_THRESHOLD = 10.0f;
    static constexpr float DEFAULT_FAST_SPEED_THRESHOLD = 50.0f;
    static constexpr float MIN_SPEED_THRESHOLD = 1.0f;
    static constexpr float MAX_SPEED_THRESHOLD = 100.0f;

    /// Boost factors
    static constexpr float DEFAULT_SLOW_BOOST_FACTOR = 0.7f;
    static constexpr float DEFAULT_FAST_BOOST_FACTOR = 1.5f;
    static constexpr float MIN_BOOST_FACTOR = 0.3f;
    static constexpr float MAX_BOOST_FACTOR = 2.0f;

    /// Direction (TO_CENTER = 0)
    static constexpr int DEFAULT_BOOST_DIRECTION = 0;
    /// @}

    // ------------------------------------------------------------------------
    /// @name Member Variables
    /// @{
    // ------------------------------------------------------------------------
    mutable std::shared_mutex configMutex_;

    /// Core settings
    float smoothingFactor_ = DEFAULT_SMOOTHING;
    int   screenWidth_ = 0;
    int   screenHeight_ = 0;
    bool  autoCenterEnabled_ = true;
    bool  mouseRelativeMode_ = false;
    bool  fastReturnEnabled_ = false;
    float springReturnSpeed_ = DEFAULT_SPRING_SPEED;
    float fastReturnBoost_ = DEFAULT_FAST_RETURN_BOOST;

    /// Dynamic boost settings
    float slowSpeedThreshold_ = DEFAULT_SLOW_SPEED_THRESHOLD;
    float fastSpeedThreshold_ = DEFAULT_FAST_SPEED_THRESHOLD;
    float slowBoostFactor_ = DEFAULT_SLOW_BOOST_FACTOR;
    float fastBoostFactor_ = DEFAULT_FAST_BOOST_FACTOR;

    /// Boost direction (0-3)
    int   boostDirection_ = DEFAULT_BOOST_DIRECTION;

    /// Button to VK code mappings
    std::map<std::string, int> mapping_;
    /// @}

public:
    // ------------------------------------------------------------------------
    /// @name Public Constants
    /// @{
    // ------------------------------------------------------------------------
    static const std::vector<std::string> VALID_BUTTONS;
    /// @}

    // ------------------------------------------------------------------------
    /// @name Lifetime Management
    /// @{
    // ------------------------------------------------------------------------
    /// @brief Constructor - loads configuration from INI
    ThreadSafeConfig() {
        screenWidth_ = GetSystemMetrics(SM_CXSCREEN);
        screenHeight_ = GetSystemMetrics(SM_CYSCREEN);
        LoadConfig();
        std::cout << "[ThreadSafeConfig] Screen: " << screenWidth_ << "x" << screenHeight_ << "\n";
    }

    /// @brief Destructor
    ~ThreadSafeConfig() {
        std::cout << "[ThreadSafeConfig] Destructor\n";
    }
    /// @}

    // ------------------------------------------------------------------------
    /// @name Read Access (Shared Lock)
    /// @{
    // ------------------------------------------------------------------------
    /// @brief Get current smoothing factor
    [[nodiscard]] float GetSmoothingFactor() const {
        std::shared_lock lock(configMutex_);
        return smoothingFactor_;
    }

    /// @brief Get screen dimensions
    /// @param[out] w Width in pixels
    /// @param[out] h Height in pixels
    void GetScreenDimensions(int& w, int& h) const noexcept {
        w = screenWidth_;
        h = screenHeight_;
    }

    /// @brief Check if an action is currently triggered
    /// @param action Action name (e.g., "A", "RT", "LS_X")
    /// @return true if the mapped key is pressed
    [[nodiscard]] bool IsTriggered(const std::string& action) const {
        std::shared_lock lock(configMutex_);
        auto it = mapping_.find(action);
        if (it == mapping_.end() || it->second == 0) return false;
        int vk = it->second;
        lock.unlock();
        return (GetAsyncKeyState(vk) & 0x8000) != 0;
    }

    /// @brief Get VK code for an action
    /// @param action Action name
    /// @return Virtual key code or 0 if not mapped
    [[nodiscard]] int GetVKForAction(const std::string& action) const {
        std::shared_lock lock(configMutex_);
        auto it = mapping_.find(action);
        return (it != mapping_.end()) ? it->second : 0;
    }

    /// @brief Get axis value from positive/negative actions
    /// @param pos Action for positive direction
    /// @param neg Action for negative direction
    /// @return Value in [-1.0, 1.0]
    [[nodiscard]] float GetAxis(const std::string& pos, const std::string& neg) const {
        float val = 0.0f;
        if (IsTriggered(pos)) val += 1.0f;
        if (IsTriggered(neg)) val -= 1.0f;
        return val;
    }

    /// @brief Get snapshot of all mappings
    [[nodiscard]] std::map<std::string, int> GetMappingSnapshot() const {
        std::shared_lock lock(configMutex_);
        return mapping_;
    }

    /// @brief Check if auto-center is enabled
    [[nodiscard]] bool IsAutoCenterEnabled() const {
        std::shared_lock lock(configMutex_);
        return autoCenterEnabled_;
    }

    /// @brief Check if spring/relative mode is active
    [[nodiscard]] bool IsMouseRelativeMode() const {
        std::shared_lock lock(configMutex_);
        return mouseRelativeMode_;
    }

    /// @brief Check if absolute boost mode is active
    [[nodiscard]] bool IsFastReturnMode() const {
        std::shared_lock lock(configMutex_);
        return fastReturnEnabled_;
    }

    /// @brief Get spring return speed
    [[nodiscard]] float GetSpringReturnSpeed() const {
        std::shared_lock lock(configMutex_);
        return springReturnSpeed_;
    }

    /// @brief Get base boost multiplier
    [[nodiscard]] float GetFastReturnBoost() const {
        std::shared_lock lock(configMutex_);
        return fastReturnBoost_;
    }

    /// @brief Get slow speed threshold
    [[nodiscard]] float GetSlowSpeedThreshold() const {
        std::shared_lock lock(configMutex_);
        return slowSpeedThreshold_;
    }

    /// @brief Get fast speed threshold
    [[nodiscard]] float GetFastSpeedThreshold() const {
        std::shared_lock lock(configMutex_);
        return fastSpeedThreshold_;
    }

    /// @brief Get slow movement boost factor
    [[nodiscard]] float GetSlowBoostFactor() const {
        std::shared_lock lock(configMutex_);
        return slowBoostFactor_;
    }

    /// @brief Get fast movement boost factor
    [[nodiscard]] float GetFastBoostFactor() const {
        std::shared_lock lock(configMutex_);
        return fastBoostFactor_;
    }

    /// @brief Get current boost direction
    [[nodiscard]] BoostDirection GetBoostDirection() const {
        std::shared_lock lock(configMutex_);
        return static_cast<BoostDirection>(boostDirection_);
    }

    /// @brief Get string representation of boost direction
    [[nodiscard]] std::string GetBoostDirectionString() const {
        std::shared_lock lock(configMutex_);
        switch (boostDirection_) {
        case 0:  return "TO CENTER";
        case 1:  return "TO EDGE";
        case 2:  return "BOTH";
        case 3:  return "NONE";
        default: return "UNKNOWN";
        }
    }

    /// @brief Get key name for an action
    /// @param action Action name
    /// @return Human-readable key name or "NONE"
    [[nodiscard]] std::string GetKeyNameForAction(const std::string& action) const {
        std::shared_lock lock(configMutex_);
        auto it = mapping_.find(action);
        if (it != mapping_.end() && it->second != 0) {
            lock.unlock();
            return Key::VKToString(it->second);
        }
        return "NONE";
    }
    /// @}

    // ------------------------------------------------------------------------
    /// @name Write Access (Unique Lock)
    /// @{
    // ------------------------------------------------------------------------
    /// @brief Update a button mapping
    /// @param action Action to map
    /// @param newKey New key name to assign
    void UpdateMapping(const std::string& action, const std::string& newKey) {
        if (!IsValidButton(action)) {
            std::cerr << "[ThreadSafeConfig] Invalid action: " << action << "\n";
            return;
        }

        std::unique_lock lock(configMutex_);
        const char* section = GetSectionForAction(action);
        WriteIniString(section, action.c_str(), newKey);

        int oldVk = mapping_[action];
        mapping_[action] = Key::StringToVK(newKey);
        lock.unlock();

        std::cout << "[ThreadSafeConfig] " << action << ": "
            << Key::VKToString(oldVk) << " -> " << newKey << "\n";
        if (Logger::IsInitialized())
            Logger::Info("Mapping: " + action + " -> " + newKey);
    }

    /// @brief Toggle auto-center setting
    void ToggleAutoCenter() {
        std::unique_lock lock(configMutex_);
        autoCenterEnabled_ = !autoCenterEnabled_;
        WriteIniString("Settings", "AutoCenter", autoCenterEnabled_ ? "1" : "0");
        Logger::Info(autoCenterEnabled_ ? "Auto-Center ON" : "Auto-Center OFF");
    }

    /// @brief Toggle between spring/relative and absolute modes
    void ToggleMouseMode() {
        std::unique_lock lock(configMutex_);
        mouseRelativeMode_ = !mouseRelativeMode_;
        if (mouseRelativeMode_) fastReturnEnabled_ = false;

        WriteIniString("Settings", "MouseRelativeMode", mouseRelativeMode_ ? "1" : "0");
        WriteIniString("Settings", "FastReturnMode", fastReturnEnabled_ ? "1" : "0");
        lock.unlock();

        if (mouseRelativeMode_)
            Logger::Info("Mode: SPRING/RELATIVE");
        else
            Logger::Info("Mode: ABSOLUTE");
    }

    /// @brief Toggle absolute boost mode (only available in absolute mode)
    void ToggleFastReturnMode() {
        std::unique_lock lock(configMutex_);
        if (mouseRelativeMode_) {
            lock.unlock();
            Logger::Warning("Cannot enable BOOST in SPRING mode");
            return;
        }
        fastReturnEnabled_ = !fastReturnEnabled_;
        WriteIniString("Settings", "FastReturnMode", fastReturnEnabled_ ? "1" : "0");
        lock.unlock();
        Logger::Info(fastReturnEnabled_ ? "Mode: ABSOLUTE BOOST" : "Mode: ABSOLUTE");
    }

    /// @brief Cycle through boost direction modes
    void CycleBoostDirection() {
        std::unique_lock lock(configMutex_);
        boostDirection_ = (boostDirection_ + 1) % 4;
        WriteIniString("Settings", "BoostDirection", std::to_string(boostDirection_));

        std::string dir;
        switch (boostDirection_) {
        case 0: dir = "TO CENTER"; break;
        case 1: dir = "TO EDGE";   break;
        case 2: dir = "BOTH";      break;
        case 3: dir = "NONE";      break;
        }
        lock.unlock();
        Logger::Info("Boost direction: " + dir);
    }

    /// @brief Update smoothing factor
    /// @param val New smoothing value (clamped to [MIN_SMOOTHING, MAX_SMOOTHING])
    void UpdateSmoothing(float val) {
        val = std::clamp(val, MIN_SMOOTHING, MAX_SMOOTHING);
        std::unique_lock lock(configMutex_);
        smoothingFactor_ = val;
        lock.unlock();
        WriteIniString("Settings", "Smoothing", std::to_string(val));
        if (Logger::IsInitialized())
            Logger::Info("Smoothing: " + std::to_string(val));
    }

    /// @brief Update spring return speed
    /// @param val New speed (clamped to [MIN_SPRING_SPEED, MAX_SPRING_SPEED])
    void UpdateSpringSpeed(float val) {
        val = std::clamp(val, MIN_SPRING_SPEED, MAX_SPRING_SPEED);
        std::unique_lock lock(configMutex_);
        springReturnSpeed_ = val;
        lock.unlock();
        WriteIniString("Settings", "SpringReturnSpeed", std::to_string(val));
        if (Logger::IsInitialized())
            Logger::Info("Spring speed: " + std::to_string(val));
    }

    /// @brief Update base boost multiplier
    /// @param val New boost value (clamped to [MIN_FAST_RETURN_BOOST, MAX_FAST_RETURN_BOOST])
    void UpdateFastReturnBoost(float val) {
        val = std::clamp(val, MIN_FAST_RETURN_BOOST, MAX_FAST_RETURN_BOOST);
        std::unique_lock lock(configMutex_);
        fastReturnBoost_ = val;
        lock.unlock();
        WriteIniString("Settings", "FastReturnBoost", std::to_string(val));
        if (Logger::IsInitialized())
            Logger::Info("Base boost: " + std::to_string(val) + "x");
    }

    /// @brief Update slow speed threshold
    /// @param val New threshold (clamped to [MIN_SPEED_THRESHOLD, MAX_SPEED_THRESHOLD])
    void UpdateSlowSpeedThreshold(float val) {
        val = std::clamp(val, MIN_SPEED_THRESHOLD, MAX_SPEED_THRESHOLD);
        std::unique_lock lock(configMutex_);
        slowSpeedThreshold_ = val;
        lock.unlock();
        WriteIniString("Settings", "SlowSpeedThreshold", std::to_string(val));
        if (Logger::IsInitialized())
            Logger::Info("Slow threshold: " + std::to_string(val));
    }

    /// @brief Update fast speed threshold
    /// @param val New threshold (clamped to [MIN_SPEED_THRESHOLD, MAX_SPEED_THRESHOLD])
    void UpdateFastSpeedThreshold(float val) {
        val = std::clamp(val, MIN_SPEED_THRESHOLD, MAX_SPEED_THRESHOLD);
        std::unique_lock lock(configMutex_);
        fastSpeedThreshold_ = val;
        lock.unlock();
        WriteIniString("Settings", "FastSpeedThreshold", std::to_string(val));
        if (Logger::IsInitialized())
            Logger::Info("Fast threshold: " + std::to_string(val));
    }

    /// @brief Update slow movement boost factor
    /// @param val New factor (clamped to [MIN_BOOST_FACTOR, MAX_BOOST_FACTOR])
    void UpdateSlowBoostFactor(float val) {
        val = std::clamp(val, MIN_BOOST_FACTOR, MAX_BOOST_FACTOR);
        std::unique_lock lock(configMutex_);
        slowBoostFactor_ = val;
        lock.unlock();
        WriteIniString("Settings", "SlowBoostFactor", std::to_string(val));
        if (Logger::IsInitialized())
            Logger::Info("Slow factor: " + std::to_string(val) + "x");
    }

    /// @brief Update fast movement boost factor
    /// @param val New factor (clamped to [MIN_BOOST_FACTOR, MAX_BOOST_FACTOR])
    void UpdateFastBoostFactor(float val) {
        val = std::clamp(val, MIN_BOOST_FACTOR, MAX_BOOST_FACTOR);
        std::unique_lock lock(configMutex_);
        fastBoostFactor_ = val;
        lock.unlock();
        WriteIniString("Settings", "FastBoostFactor", std::to_string(val));
        if (Logger::IsInitialized())
            Logger::Info("Fast factor: " + std::to_string(val) + "x");
    }
    /// @}

    // ------------------------------------------------------------------------
    /// @name Utilities
    /// @{
    // ------------------------------------------------------------------------
    /// @brief Check if a button name is valid
    /// @param btn Button name to check
    /// @return true if button exists in VALID_BUTTONS
    [[nodiscard]] static bool IsValidButton(const std::string& btn) {
        return std::find(VALID_BUTTONS.begin(), VALID_BUTTONS.end(), btn) != VALID_BUTTONS.end();
    }

    /// @brief Print current configuration to console
    void PrintSummary() const {
        std::shared_lock lock(configMutex_);
        std::cout << "\n=== CONFIG SUMMARY ===\n"
            << "Smoothing: " << smoothingFactor_ << "\n"
            << "Mode: ";
        if (mouseRelativeMode_)      std::cout << "SPRING/RELATIVE";
        else if (fastReturnEnabled_) std::cout << "ABSOLUTE BOOST";
        else                         std::cout << "ABSOLUTE";
        std::cout << "\n";

        if (mouseRelativeMode_)
            std::cout << "Spring speed: " << springReturnSpeed_ << "\n";

        if (fastReturnEnabled_) {
            std::cout << "Boost dir: " << GetBoostDirectionString() << "\n"
                << "Base boost: " << fastReturnBoost_ << "x\n"
                << "Slow thresh: " << slowSpeedThreshold_ << "\n"
                << "Fast thresh: " << fastSpeedThreshold_ << "\n"
                << "Slow factor: " << slowBoostFactor_ << "x\n"
                << "Fast factor: " << fastBoostFactor_ << "x\n";
        }

        std::cout << "Resolution: " << screenWidth_ << "x" << screenHeight_ << "\n"
            << "Mappings: " << mapping_.size() << "\n"
            << "====================\n";
    }
    /// @}

private:
    // ------------------------------------------------------------------------
    /// @name Private Helpers
    /// @{
    // ------------------------------------------------------------------------
    /// @brief Write a string to the INI file
    void WriteIniString(const char* section, const char* key, const std::string& val) {
        WritePrivateProfileStringA(section, key, val.c_str(), INI_FILE_PATH);
    }

    /// @brief Get INI section name for an action
    /// @param action Action name
    /// @return Section name constant
    [[nodiscard]] static const char* GetSectionForAction(const std::string& action) {
        if (action.find("LS_") == 0 || action.find("RS_") == 0) return "Sticks";
        if (action.find("DPAD_") == 0) return "DPAD";
        if (action == "LT" || action == "RT") return "Triggers";
        return "Buttons";
    }

    /// @brief Load all settings from INI file
    void LoadConfig() {
        std::unique_lock lock(configMutex_);
        char buffer[256];

        // Lambda helpers for reading different types
        auto readFloat = [&](const char* key, float def, float minv, float maxv) {
            GetPrivateProfileStringA("Settings", key, std::to_string(def).c_str(),
                buffer, sizeof(buffer), INI_FILE_PATH);
            try {
                float v = std::stof(buffer);
                return std::clamp(v, minv, maxv);
            }
            catch (...) {
                return def;
            }
            };

        auto readInt = [&](const char* key, int def, int minv, int maxv) {
            GetPrivateProfileStringA("Settings", key, std::to_string(def).c_str(),
                buffer, sizeof(buffer), INI_FILE_PATH);
            try {
                int v = std::stoi(buffer);
                return (v < minv || v > maxv) ? def : v;
            }
            catch (...) {
                return def;
            }
            };

        auto readBool = [&](const char* key, bool def) {
            GetPrivateProfileStringA("Settings", key, def ? "1" : "0",
                buffer, sizeof(buffer), INI_FILE_PATH);
            return std::string(buffer) == "1";
            };

        // Read all numeric settings
        smoothingFactor_ = readFloat("Smoothing", DEFAULT_SMOOTHING, MIN_SMOOTHING, MAX_SMOOTHING);
        springReturnSpeed_ = readFloat("SpringReturnSpeed", DEFAULT_SPRING_SPEED, MIN_SPRING_SPEED, MAX_SPRING_SPEED);
        fastReturnBoost_ = readFloat("FastReturnBoost", DEFAULT_FAST_RETURN_BOOST, MIN_FAST_RETURN_BOOST, MAX_FAST_RETURN_BOOST);
        slowSpeedThreshold_ = readFloat("SlowSpeedThreshold", DEFAULT_SLOW_SPEED_THRESHOLD, MIN_SPEED_THRESHOLD, MAX_SPEED_THRESHOLD);
        fastSpeedThreshold_ = readFloat("FastSpeedThreshold", DEFAULT_FAST_SPEED_THRESHOLD, MIN_SPEED_THRESHOLD, MAX_SPEED_THRESHOLD);
        slowBoostFactor_ = readFloat("SlowBoostFactor", DEFAULT_SLOW_BOOST_FACTOR, MIN_BOOST_FACTOR, MAX_BOOST_FACTOR);
        fastBoostFactor_ = readFloat("FastBoostFactor", DEFAULT_FAST_BOOST_FACTOR, MIN_BOOST_FACTOR, MAX_BOOST_FACTOR);

        // Read boolean settings
        autoCenterEnabled_ = readBool("AutoCenter", true);
        mouseRelativeMode_ = readBool("MouseRelativeMode", false);
        fastReturnEnabled_ = readBool("FastReturnMode", false);

        // Read integer settings
        boostDirection_ = readInt("BoostDirection", DEFAULT_BOOST_DIRECTION, 0, 3);

        // Read button mappings
        for (const auto& action : VALID_BUTTONS) {
            char val[256];
            GetPrivateProfileStringA(GetSectionForAction(action), action.c_str(), "NONE",
                val, sizeof(val), INI_FILE_PATH);
            mapping_[action] = Key::StringToVK(val);
        }

        Logger::Info("Config loaded");
    }
    /// @}
};

// ============================================================================
/// @name Static Data
/// @{
// ============================================================================
inline const std::vector<std::string> ThreadSafeConfig::VALID_BUTTONS = {
    "LS_X", "LS_Y", "LS_UP", "LS_DOWN", "LS_LEFT", "LS_RIGHT",
    "RS_X", "RS_Y", "RS_UP", "RS_DOWN", "RS_LEFT", "RS_RIGHT",
    "A", "B", "X", "Y",
    "LB", "RB",
    "START", "BACK",
    "LS_CLICK", "RS_CLICK",
    "DPAD_UP", "DPAD_DOWN", "DPAD_LEFT", "DPAD_RIGHT",
    "LT", "RT"
};
/// @}

// ============================================================================
/// @class EmulationState
/// @brief Simple atomic state for emulation control
// ============================================================================
class EmulationState {
private:
    std::atomic<bool> active_{ false };      ///< Emulation running state
    std::atomic<bool> shouldExit_{ false };  ///< Exit requested flag

public:
    /// @brief Start emulation
    void Start() {
        active_.store(true, std::memory_order_release);
        Logger::Info("Emulation ON");
    }

    /// @brief Stop emulation
    void Stop() {
        active_.store(false, std::memory_order_release);
        Logger::Info("Emulation OFF");
    }

    /// @brief Check if emulation is active
    [[nodiscard]] bool IsActive() const {
        return active_.load(std::memory_order_acquire);
    }

    /// @brief Request program exit
    void RequestExit() {
        shouldExit_.store(true, std::memory_order_release);
    }

    /// @brief Check if exit was requested
    [[nodiscard]] bool ShouldExit() const {
        return shouldExit_.load(std::memory_order_acquire);
    }
};