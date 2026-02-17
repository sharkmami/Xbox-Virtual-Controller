#pragma once

// ============================================================================
/// @file Key.hpp
/// @brief Virtual key code management and conversion system
/// @provides Bidirectional mapping between key names and VK codes
// ============================================================================

// ============================================================================
// DEPENDENCIES
// ============================================================================
#include <windows.h>

#include <algorithm>
#include <array>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

#include "Logger.hpp"

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================
class Key;
class KeyManager;
struct KeyInfo;

// ============================================================================
/// @namespace KeyConstants
/// @brief Custom virtual key codes and special values
// ============================================================================
namespace KeyConstants {
    /// @name Custom Key Codes
    ///@{
    constexpr int VK_MOUSE_X = 901;  ///< Mouse X-axis movement
    constexpr int VK_MOUSE_Y = 902;  ///< Mouse Y-axis movement
    ///@}

    /// @name Special Values
    ///@{
    constexpr int VK_NONE = 0;    ///< No key mapped
    constexpr int VK_ERROR = -1;   ///< Error code for conversions
    ///@}
}

// ============================================================================
/// @struct KeyInfo
/// @brief Complete information about a virtual key
// ============================================================================
struct KeyInfo {
    int vkCode;               ///< Virtual key code
    std::string name;         ///< Human-readable name
    std::string category;     ///< Key category (SYSTEM, MOUSE, MODIFIER, etc.)

    /// @brief Constructor
    KeyInfo(int vk = 0, const std::string& n = "", const std::string& cat = "")
        : vkCode(vk), name(n), category(cat) {
    }
};

// ============================================================================
/// @class KeyManager
/// @brief Manages bidirectional mapping between key names and virtual key codes
/// @details Thread-safe singleton providing conversion and state detection
// ============================================================================
class KeyManager {
private:
    // ------------------------------------------------------------------------
    /// @struct KeyMaps
    /// @brief Internal bidirectional maps for key conversion
    // ------------------------------------------------------------------------
    struct KeyMaps {
        std::map<std::string, int> nameToVk;      ///< Name → VK code
        std::map<int, std::string> vkToName;      ///< VK code → Name
        std::map<int, std::string> vkToCategory;  ///< VK code → Category

        /// @brief Constructor - initializes all key mappings
        KeyMaps() {
            InitSpecial();
            InitAlphanumeric();
        }

    private:
        /// @brief Add a key mapping to all maps
        /// @param vk Virtual key code
        /// @param name Human-readable name
        /// @param cat Category name
        void Add(int vk, const std::string& name, const std::string& cat) {
            nameToVk[name] = vk;
            vkToName[vk] = name;
            vkToCategory[vk] = cat;
        }

        /// @brief Initialize all special keys (non-alphanumeric)
        void InitSpecial() {
            // System custom codes
            Add(KeyConstants::VK_MOUSE_X, "MOUSE_X", "SYSTEM");
            Add(KeyConstants::VK_MOUSE_Y, "MOUSE_Y", "SYSTEM");
            Add(KeyConstants::VK_NONE, "NONE", "SYSTEM");
            Add(KeyConstants::VK_NONE, "NULL", "SYSTEM");

            // Mouse buttons
            Add(VK_LBUTTON, "LBUTTON", "MOUSE");
            Add(VK_RBUTTON, "RBUTTON", "MOUSE");
            Add(VK_MBUTTON, "MBUTTON", "MOUSE");
            Add(VK_XBUTTON1, "XBUTTON1", "MOUSE");
            Add(VK_XBUTTON2, "XBUTTON2", "MOUSE");

            // Modifier keys
            Add(VK_LSHIFT, "LSHIFT", "MODIFIER");
            Add(VK_RSHIFT, "RSHIFT", "MODIFIER");
            Add(VK_LCONTROL, "LCONTROL", "MODIFIER");
            Add(VK_RCONTROL, "RCONTROL", "MODIFIER");
            Add(VK_LMENU, "LALT", "MODIFIER");
            Add(VK_RMENU, "RALT", "MODIFIER");
            Add(VK_LWIN, "LWIN", "MODIFIER");
            Add(VK_RWIN, "RWIN", "MODIFIER");
            Add(VK_APPS, "APPS", "MODIFIER");
            Add(VK_APPS, "CONTEXT", "MODIFIER");

            // Navigation keys
            Add(VK_SPACE, "SPACE", "NAVIGATION");
            Add(VK_RETURN, "ENTER", "NAVIGATION");
            Add(VK_RETURN, "RETURN", "NAVIGATION");
            Add(VK_ESCAPE, "ESCAPE", "NAVIGATION");
            Add(VK_TAB, "TAB", "NAVIGATION");
            Add(VK_CAPITAL, "CAPSLOCK", "NAVIGATION");
            Add(VK_NUMLOCK, "NUMLOCK", "NAVIGATION");
            Add(VK_SCROLL, "SCROLLLOCK", "NAVIGATION");
            Add(VK_BACK, "BACKSPACE", "NAVIGATION");
            Add(VK_DELETE, "DELETE", "NAVIGATION");
            Add(VK_INSERT, "INSERT", "NAVIGATION");
            Add(VK_HOME, "HOME", "NAVIGATION");
            Add(VK_END, "END", "NAVIGATION");
            Add(VK_PRIOR, "PAGEUP", "NAVIGATION");
            Add(VK_NEXT, "PAGEDOWN", "NAVIGATION");

            // Arrow keys
            Add(VK_UP, "UP", "ARROW");
            Add(VK_DOWN, "DOWN", "ARROW");
            Add(VK_LEFT, "LEFT", "ARROW");
            Add(VK_RIGHT, "RIGHT", "ARROW");

            // Function keys
            Add(VK_F1, "F1", "FUNCTION"); Add(VK_F2, "F2", "FUNCTION");
            Add(VK_F3, "F3", "FUNCTION"); Add(VK_F4, "F4", "FUNCTION");
            Add(VK_F5, "F5", "FUNCTION"); Add(VK_F6, "F6", "FUNCTION");
            Add(VK_F7, "F7", "FUNCTION"); Add(VK_F8, "F8", "FUNCTION");
            Add(VK_F9, "F9", "FUNCTION"); Add(VK_F10, "F10", "FUNCTION");
            Add(VK_F11, "F11", "FUNCTION"); Add(VK_F12, "F12", "FUNCTION");

            // Numpad keys
            Add(VK_NUMPAD0, "NUMPAD0", "NUMPAD"); Add(VK_NUMPAD1, "NUMPAD1", "NUMPAD");
            Add(VK_NUMPAD2, "NUMPAD2", "NUMPAD"); Add(VK_NUMPAD3, "NUMPAD3", "NUMPAD");
            Add(VK_NUMPAD4, "NUMPAD4", "NUMPAD"); Add(VK_NUMPAD5, "NUMPAD5", "NUMPAD");
            Add(VK_NUMPAD6, "NUMPAD6", "NUMPAD"); Add(VK_NUMPAD7, "NUMPAD7", "NUMPAD");
            Add(VK_NUMPAD8, "NUMPAD8", "NUMPAD"); Add(VK_NUMPAD9, "NUMPAD9", "NUMPAD");
            Add(VK_ADD, "NUMPAD_ADD", "NUMPAD");
            Add(VK_SUBTRACT, "NUMPAD_SUBTRACT", "NUMPAD");
            Add(VK_MULTIPLY, "NUMPAD_MULTIPLY", "NUMPAD");
            Add(VK_DIVIDE, "NUMPAD_DIVIDE", "NUMPAD");
            Add(VK_DECIMAL, "NUMPAD_DECIMAL", "NUMPAD");

            // Additional system keys
            Add(VK_SNAPSHOT, "PRINTSCREEN", "SYSTEM");
            Add(VK_PAUSE, "PAUSE", "SYSTEM");
        }

        /// @brief Initialize alphanumeric keys (0-9, A-Z)
        void InitAlphanumeric() {
            for (char c = '0'; c <= '9'; ++c)
                Add(c, std::string(1, c), "ALPHANUMERIC");
            for (char c = 'A'; c <= 'Z'; ++c)
                Add(c, std::string(1, c), "ALPHANUMERIC");
        }
    };

    /// @brief Get the singleton maps instance
    [[nodiscard]] static KeyMaps& GetMaps() {
        static KeyMaps instance;
        return instance;
    }

    /// @brief Get mutex for thread safety (currently unused but reserved)
    [[nodiscard]] static std::mutex& GetMutex() {
        static std::mutex mtx;
        return mtx;
    }

public:
    // ------------------------------------------------------------------------
    /// @name Key Conversion
    /// @{
    // ------------------------------------------------------------------------
    /// @brief Convert a key name string to virtual key code
    /// @param name Key name (case-insensitive, e.g., "A", "SPACE", "F1")
    /// @return Virtual key code or VK_NONE/VK_ERROR on failure
    [[nodiscard]] static int StringToVK(const std::string& name) {
        try {
            if (name.empty()) return KeyConstants::VK_NONE;

            std::string up = name;
            std::transform(up.begin(), up.end(), up.begin(), ::toupper);

            const auto& maps = GetMaps();
            auto it = maps.nameToVk.find(up);
            if (it != maps.nameToVk.end()) return it->second;

            if (up.length() == 1) {
                SHORT r = VkKeyScanA(up[0]);
                if (r != -1) return LOBYTE(r);
            }

            try {
                size_t pos = 0;
                int vk = std::stoi(up, &pos);
                if (pos == up.length() && vk >= 0 && vk <= 255) return vk;
            }
            catch (...) {}

            return KeyConstants::VK_NONE;
        }
        catch (const std::exception& e) {
            Logger::Error("StringToVK: " + std::string(e.what()));
            return KeyConstants::VK_ERROR;
        }
    }

    /// @brief Convert virtual key code to human-readable name
    /// @param vk Virtual key code
    /// @return Key name or "UNKNOWN_VK_xxx" on failure
    [[nodiscard]] static std::string VKToString(int vk) {
        try {
            if (vk == KeyConstants::VK_MOUSE_X) return "MOUSE_X";
            if (vk == KeyConstants::VK_MOUSE_Y) return "MOUSE_Y";
            if (vk == KeyConstants::VK_NONE)    return "NONE";
            if (vk == KeyConstants::VK_ERROR)   return "ERROR";

            const auto& maps = GetMaps();
            auto it = maps.vkToName.find(vk);
            if (it != maps.vkToName.end()) return it->second;

            return NameFromWindows(vk);
        }
        catch (const std::exception& e) {
            Logger::Error("VKToString: " + std::string(e.what()));
            return "ERROR";
        }
    }

    /// @brief Get complete information about a key
    /// @param vk Virtual key code
    /// @return KeyInfo structure with name and category
    [[nodiscard]] static KeyInfo GetKeyInfo(int vk) {
        KeyInfo info(vk, VKToString(vk));
        const auto& maps = GetMaps();
        auto it = maps.vkToCategory.find(vk);
        info.category = (it != maps.vkToCategory.end()) ? it->second : "UNKNOWN";
        return info;
    }
    /// @}

    // ------------------------------------------------------------------------
    /// @name Key State Detection
    /// @{
    // ------------------------------------------------------------------------
    /// @brief Check if a key is currently pressed
    /// @param vk Virtual key code
    /// @return true if key is pressed
    [[nodiscard]] static bool IsPressed(int vk) {
        if (vk <= 0 || vk == KeyConstants::VK_MOUSE_X || vk == KeyConstants::VK_MOUSE_Y)
            return false;
        return (GetAsyncKeyState(vk) & 0x8000) != 0;
    }

    /// @brief Check if a key was just pressed (transition from up to down)
    /// @param vk Virtual key code
    /// @return true if key was pressed since last check
    [[nodiscard]] static bool WasJustPressed(int vk) {
        if (vk <= 0) return false;
        SHORT state = GetAsyncKeyState(vk);
        return (state & 0x8000) && (state & 0x0001);
    }

    /// @brief Wait for a key press with timeout
    /// @param vk Virtual key code
    /// @param timeout Timeout in milliseconds (0 = infinite)
    /// @return true if key was pressed within timeout
    [[nodiscard]] static bool WaitForKeyPress(int vk, DWORD timeout = 0) {
        if (vk <= 0) return false;
        DWORD start = GetTickCount();
        while (true) {
            if (IsPressed(vk)) return true;
            if (timeout && (GetTickCount() - start >= timeout)) return false;
            Sleep(10);
        }
    }
    /// @}

    // ------------------------------------------------------------------------
    /// @name Key Classification Utilities
    /// @{
    // ------------------------------------------------------------------------
    /// @brief Check if code is a special mouse axis code
    [[nodiscard]] static bool IsSpecialCode(int vk) {
        return vk == KeyConstants::VK_MOUSE_X || vk == KeyConstants::VK_MOUSE_Y;
    }

    /// @brief Check if code corresponds to a physical mouse button
    [[nodiscard]] static bool IsMouseButton(int vk) {
        return vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON ||
            vk == VK_XBUTTON1 || vk == VK_XBUTTON2;
    }

    /// @brief Check if code is a modifier key (Shift, Ctrl, Alt, Win)
    [[nodiscard]] static bool IsModifier(int vk) {
        return vk == VK_LSHIFT || vk == VK_RSHIFT ||
            vk == VK_LCONTROL || vk == VK_RCONTROL ||
            vk == VK_LMENU || vk == VK_RMENU ||
            vk == VK_LWIN || vk == VK_RWIN;
    }

    /// @brief Get all known key names (for UI dropdowns, etc.)
    [[nodiscard]] static std::vector<std::string> GetAllKeyNames() {
        const auto& maps = GetMaps();
        std::vector<std::string> names;
        names.reserve(maps.nameToVk.size());
        for (const auto& p : maps.nameToVk) {
            if (std::find(names.begin(), names.end(), p.first) == names.end())
                names.push_back(p.first);
        }
        return names;
    }

    /// @brief Print detailed key information to console (debugging)
    static void PrintKeyInfo(int vk) {
        auto info = GetKeyInfo(vk);
        std::cout << "\n=== KEY INFO ===\n"
            << "VK: " << info.vkCode << "\n"
            << "Name: " << info.name << "\n"
            << "Cat: " << info.category << "\n"
            << "State: " << (IsPressed(vk) ? "PRESSED" : "RELEASED") << "\n"
            << "Special: " << (IsSpecialCode(vk) ? "YES" : "NO") << "\n"
            << "================\n";
    }
    /// @}

private:
    // ------------------------------------------------------------------------
    /// @name Windows API Helpers
    /// @{
    // ------------------------------------------------------------------------
    /// @brief Get key name from Windows API when not in maps
    /// @param vk Virtual key code
    /// @return Windows key name or fallback string
    [[nodiscard]] static std::string NameFromWindows(int vk) {
        if (vk < 0 || vk > 255) return "INVALID_VK_" + std::to_string(vk);

        UINT scan = MapVirtualKeyA(static_cast<UINT>(vk), MAPVK_VK_TO_VSC);
        if (scan == 0) return "UNKNOWN_VK_" + std::to_string(vk);

        LONG lParam = static_cast<LONG>(scan) << 16;
        static const std::vector<int> ext = {
            VK_LEFT, VK_RIGHT, VK_UP, VK_DOWN,
            VK_PRIOR, VK_NEXT, VK_HOME, VK_END,
            VK_INSERT, VK_DELETE
        };
        if (std::find(ext.begin(), ext.end(), vk) != ext.end())
            lParam |= 0x01000000;

        char name[256] = { 0 };
        if (GetKeyNameTextA(lParam, name, sizeof(name)))
            return std::string(name);

        return "UNKNOWN_VK_" + std::to_string(vk);
    }
    /// @}
};

// ============================================================================
/// @class Key
/// @brief Simple wrapper class providing static access to KeyManager functionality
/// @note Maintained for backward compatibility
// ============================================================================
class Key {
public:
    /// @name Constants
    ///@{
    static constexpr int VK_MOUSE_X = KeyConstants::VK_MOUSE_X;
    static constexpr int VK_MOUSE_Y = KeyConstants::VK_MOUSE_Y;
    static constexpr int VK_NONE = KeyConstants::VK_NONE;
    ///@}

    /// @name Conversion
    ///@{
    [[nodiscard]] static int StringToVK(const std::string& s) { return KeyManager::StringToVK(s); }
    [[nodiscard]] static std::string VKToString(int vk) { return KeyManager::VKToString(vk); }
    ///@}

    /// @name State Detection
    ///@{
    [[nodiscard]] static bool isPressed(int vk) { return KeyManager::IsPressed(vk); }
    [[nodiscard]] static bool wasJustPressed(int vk) { return KeyManager::WasJustPressed(vk); }
    [[nodiscard]] static bool waitForKeyPress(int vk, DWORD t) { return KeyManager::WaitForKeyPress(vk, t); }
    ///@}

    /// @name Utilities
    ///@{
    [[nodiscard]] static std::vector<std::string> GetAllKeyNames() { return KeyManager::GetAllKeyNames(); }
    [[nodiscard]] static bool IsSpecialCode(int vk) { return KeyManager::IsSpecialCode(vk); }
    [[nodiscard]] static bool IsMouseButton(int vk) { return KeyManager::IsMouseButton(vk); }
    static void PrintKeyInfo(int vk) { KeyManager::PrintKeyInfo(vk); }
    ///@}
};