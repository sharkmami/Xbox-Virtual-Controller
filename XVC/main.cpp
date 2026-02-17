// ============================================================================
/// @file main.cpp
/// @brief XVC++ - Xbox 360 Controller Emulator Main Entry Point
/// @version 2.0.0
/// @provides Mouse to Xbox 360 controller emulation with advanced boost modes
// ============================================================================

// ============================================================================
// DEPENDENCIES
// ============================================================================
#include <windows.h>
#include <conio.h>
#include <timeapi.h>
#pragma comment(lib, "winmm.lib")

#include <atomic>
#include <algorithm>
#include <chrono>
#include <exception>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "Config.hpp"
#include "Emulator.hpp"
#include "Key.hpp"
#include "Logger.hpp"
#include "Processor.hpp"

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================
class Application;
class ConsoleUI;
class EmulationEngine;
class InputMapper;

// ============================================================================
/// @class InputMapper
/// @brief Converts mouse movements and key presses to controller axes
/// @details Handles absolute, spring, and fast return modes with dynamic boost
// ============================================================================
class InputMapper {
private:
    // ------------------------------------------------------------------------
    /// @name Constants
    /// @{
    static constexpr int IDLE_TIMEOUT_MS = 5000;   ///< Time before considering idle
    static constexpr int CENTER_THRESHOLD = 2;      ///< Pixels from center to snap
    static constexpr int SPRING_DELAY_MS = 0;      ///< Delay before spring return

    static constexpr float FAST_RETURN_ACTIVATION_DIST = 0.2f;  ///< Min distance for fast return
    static constexpr float FAST_RETURN_MIN_SPEED = 0.1f;  ///< Min speed for fast return
    /// @}

    // ------------------------------------------------------------------------
    /// @name Mouse State
    /// @{
    POINT       lastMousePos_ = { 0, 0 };        ///< Last cursor position
    ULONGLONG   lastMovementTime_ = 0;                ///< Last movement timestamp
    bool        isCentering_ = false;            ///< Auto-center in progress
    /// @}

    // ------------------------------------------------------------------------
    /// @name Spring Mode State
    /// @{
    bool        wasRelativeMode_ = false;           ///< Previous mode flag
    bool        isReturningToCenter_ = false;           ///< Spring return active
    bool        delayActive_ = false;           ///< Spring delay active
    float       currentStickX_ = 0.0f;            ///< Current X axis value
    float       currentStickY_ = 0.0f;            ///< Current Y axis value
    ULONGLONG   lastInputTime_ = 0;                ///< Last input timestamp
    /// @}

    // ------------------------------------------------------------------------
    /// @name Fast Return Mode State
    /// @{
    bool        fastReturnActive_ = false;           ///< Fast return in progress
    POINT       fastReturnStartPos_ = { 0, 0 };        ///< Start position for return
    ULONGLONG   fastReturnStartTime_ = 0;                ///< Start time for return
    /// @}

public:
    // ------------------------------------------------------------------------
    /// @struct AxisValue
    /// @brief Result structure for mouse axis calculations
    // ------------------------------------------------------------------------
    struct AxisValue {
        float x = 0.0f;              ///< Normalized X axis value [-1.0, 1.0]
        float y = 0.0f;              ///< Normalized Y axis value [-1.0, 1.0]
        bool isIdle = false;         ///< Mouse idle state flag
        bool isRelativeMode = false; ///< Relative mode active flag
    };

    // ------------------------------------------------------------------------
    /// @name Public Methods
    /// @{
    /// @brief Get mouse axis based on current mode
    /// @param config Thread-safe configuration reference
    /// @return AxisValue with current stick positions
    [[nodiscard]] AxisValue GetMouseAxis(const ThreadSafeConfig& config) {
        if (config.IsMouseRelativeMode()) {
            return GetMouseHybridSpring(config);
        }
        if (config.IsFastReturnMode()) {
            wasRelativeMode_ = false;
            isReturningToCenter_ = false;
            delayActive_ = false;
            auto result = GetMouseFastReturn(config);
            result.isRelativeMode = false;
            return result;
        }
        wasRelativeMode_ = false;
        isReturningToCenter_ = false;
        delayActive_ = false;
        auto result = GetMousePosition(config);
        result.isRelativeMode = false;
        return result;
    }

    /// @brief Reset spring mode state
    void ResetSpringState() noexcept {
        currentStickX_ = 0.0f;
        currentStickY_ = 0.0f;
        isReturningToCenter_ = false;
        delayActive_ = false;
        lastInputTime_ = 0;
        wasRelativeMode_ = false;
        fastReturnActive_ = false;
    }

    /// @brief Calculate trigger value (analog)
    /// @param config Thread-safe configuration
    /// @param action Action name ("LT" or "RT")
    /// @return Trigger value 0-255
    [[nodiscard]] unsigned char CalculateTrigger(const ThreadSafeConfig& config,
        const std::string& action) {
        int vk = config.GetVKForAction(action);
        if (vk == 0) return 0;

        // Mouse as trigger (analog)
        if (vk == Key::VK_MOUSE_X || vk == Key::VK_MOUSE_Y) {
            auto mouse = GetMouseAxis(config);
            float pos = (vk == Key::VK_MOUSE_X) ? mouse.x : mouse.y;
            float finalVal = 0.0f;

            if (action == "RT" && pos > 0) finalVal = pos;
            else if (action == "LT" && pos < 0) finalVal = -pos;

            return static_cast<unsigned char>(std::clamp(finalVal, 0.0f, 1.0f) * 255.0f);
        }

        // Digital key (on/off)
        return config.IsTriggered(action) ? 255 : 0;
    }

    /// @brief Calculate stick values from keyboard and mouse
    /// @param config Thread-safe configuration
    /// @param prefix Stick prefix ("LS" or "RS")
    /// @param[out] outX Calculated X value
    /// @param[out] outY Calculated Y value
    /// @param mouseIdle Mouse idle state
    void CalculateStick(const ThreadSafeConfig& config, const std::string& prefix,
        float& outX, float& outY, bool mouseIdle) {
        float targetX = 0.0f;
        float targetY = 0.0f;

        // Keyboard input (digital)
        if (config.IsTriggered(prefix + "_UP"))    targetY += 1.0f;
        if (config.IsTriggered(prefix + "_DOWN"))  targetY -= 1.0f;
        if (config.IsTriggered(prefix + "_LEFT"))  targetX -= 1.0f;
        if (config.IsTriggered(prefix + "_RIGHT")) targetX += 1.0f;

        // Mouse input (analog)
        if (!mouseIdle) {
            int vkX = config.GetVKForAction(prefix + "_X");
            int vkY = config.GetVKForAction(prefix + "_Y");

            if (vkX == Key::VK_MOUSE_X || vkX == Key::VK_MOUSE_Y) {
                auto mouse = GetMouseAxis(config);
                if (vkX == Key::VK_MOUSE_X) targetX = mouse.x;
                if (vkY == Key::VK_MOUSE_Y) targetY = mouse.y;
            }
        }

        outX = std::clamp(targetX, -1.0f, 1.0f);
        outY = std::clamp(targetY, -1.0f, 1.0f);
    }
    /// @}

private:
    // ------------------------------------------------------------------------
    /// @name Mouse Positioning Modes
    /// @{
    /// @brief Absolute mode - direct mouse to stick mapping
    [[nodiscard]] AxisValue GetMousePosition(const ThreadSafeConfig& config) {
        AxisValue result;
        POINT currentPos;

        if (!GetCursorPos(&currentPos)) return result;

        ULONGLONG currentTime = GetTickCount64();

        if (currentPos.x != lastMousePos_.x || currentPos.y != lastMousePos_.y) {
            if (!isCentering_) lastMovementTime_ = currentTime;
        }

        result.isIdle = (currentTime - lastMovementTime_) > IDLE_TIMEOUT_MS;

        if (result.isIdle && config.IsAutoCenterEnabled()) {
            CenterMouseIfIdle();
        }
        else {
            isCentering_ = false;
        }

        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);

        if (screenW > 0 && screenH > 0) {
            float rawX = (static_cast<float>(currentPos.x) / screenW) * 2.0f - 1.0f;
            float rawY = -((static_cast<float>(currentPos.y) / screenH) * 2.0f - 1.0f);

            result.x = std::clamp(rawX, -1.0f, 1.0f);
            result.y = std::clamp(rawY, -1.0f, 1.0f);
        }

        lastMousePos_ = currentPos;
        return result;
    }

    /// @brief Spring mode - returns to center when idle
    [[nodiscard]] AxisValue GetMouseHybridSpring(const ThreadSafeConfig& config) {
        AxisValue result;
        POINT currentPos;

        if (!GetCursorPos(&currentPos)) return result;

        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        if (screenW <= 0 || screenH <= 0) return result;

        if (!wasRelativeMode_) {
            isReturningToCenter_ = false;
            delayActive_ = false;
            lastInputTime_ = GetTickCount64();
            wasRelativeMode_ = true;
        }

        bool mouseMoved = (currentPos.x != lastMousePos_.x || currentPos.y != lastMousePos_.y);
        ULONGLONG currentTime = GetTickCount64();

        if (mouseMoved) {
            isReturningToCenter_ = false;
            delayActive_ = false;
            lastMovementTime_ = currentTime;
            lastInputTime_ = currentTime;

            float targetX = (static_cast<float>(currentPos.x) / screenW) * 2.0f - 1.0f;
            float targetY = -((static_cast<float>(currentPos.y) / screenH) * 2.0f - 1.0f);

            currentStickX_ = std::clamp(targetX, -1.0f, 1.0f);
            currentStickY_ = std::clamp(targetY, -1.0f, 1.0f);

            result.isIdle = false;
        }
        else {
            ULONGLONG timeSinceLastInput = currentTime - lastInputTime_;

            if (timeSinceLastInput < SPRING_DELAY_MS) {
                delayActive_ = true;
                result.isIdle = false;
            }
            else {
                delayActive_ = false;
                isReturningToCenter_ = true;

                float returnSpeed = config.GetSpringReturnSpeed() * 0.1f;

                currentStickX_ = Processor::Lerp(currentStickX_, 0.0f, returnSpeed);
                currentStickY_ = Processor::Lerp(currentStickY_, 0.0f, returnSpeed);

                if (std::abs(currentStickX_) < 0.001f) currentStickX_ = 0.0f;
                if (std::abs(currentStickY_) < 0.001f) currentStickY_ = 0.0f;

                if (std::abs(currentStickX_) > 0.01f || std::abs(currentStickY_) > 0.01f) {
                    POINT center = { screenW / 2, screenH / 2 };

                    int targetCursorX = static_cast<int>((currentStickX_ + 1.0f) / 2.0f * screenW);
                    int targetCursorY = static_cast<int>((-currentStickY_ + 1.0f) / 2.0f * screenH);

                    SetCursorPos(targetCursorX, targetCursorY);
                    currentPos.x = targetCursorX;
                    currentPos.y = targetCursorY;
                }

                result.isIdle = (currentTime - lastMovementTime_) > IDLE_TIMEOUT_MS;
            }
        }

        result.x = currentStickX_;
        result.y = currentStickY_;
        result.isRelativeMode = true;

        lastMousePos_ = currentPos;
        return result;
    }

    /// @brief Fast Return mode - boosted movement towards center/edges
    [[nodiscard]] AxisValue GetMouseFastReturn(const ThreadSafeConfig& config) {
        AxisValue result;
        POINT currentPos;

        if (!GetCursorPos(&currentPos)) return result;

        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        POINT center = { screenW / 2, screenH / 2 };

        ULONGLONG currentTime = GetTickCount64();
        bool mouseMoved = (currentPos.x != lastMousePos_.x || currentPos.y != lastMousePos_.y);

        if (mouseMoved) {
            lastMovementTime_ = currentTime;

            int deltaX = currentPos.x - lastMousePos_.x;
            int deltaY = currentPos.y - lastMousePos_.y;

            int toCenterX = center.x - currentPos.x;
            int toCenterY = center.y - currentPos.y;

            bool goingToCenterX = (deltaX > 0 && toCenterX > 0) || (deltaX < 0 && toCenterX < 0);
            bool goingToCenterY = (deltaY > 0 && toCenterY > 0) || (deltaY < 0 && toCenterY < 0);
            bool goingToEdgeX = !goingToCenterX && (deltaX != 0);
            bool goingToEdgeY = !goingToCenterY && (deltaY != 0);

            int boostedX = currentPos.x;
            int boostedY = currentPos.y;

            float moveSpeed = std::sqrt(float(deltaX * deltaX + deltaY * deltaY));
            float baseBoost = config.GetFastReturnBoost();

            float slowThreshold = config.GetSlowSpeedThreshold();
            float fastThreshold = config.GetFastSpeedThreshold();
            float slowFactor = config.GetSlowBoostFactor();
            float fastFactor = config.GetFastBoostFactor();

            float dynamicBoost = baseBoost;
            if (moveSpeed > fastThreshold) {
                dynamicBoost = baseBoost * fastFactor;
            }
            else if (moveSpeed < slowThreshold) {
                dynamicBoost = baseBoost * slowFactor;
            }
            dynamicBoost = std::clamp(dynamicBoost, 1.1f, 5.0f);

            BoostDirection boostDir = config.GetBoostDirection();

            // Apply X boost based on direction
            if ((boostDir == BoostDirection::TO_CENTER && goingToCenterX) ||
                (boostDir == BoostDirection::TO_EDGE && goingToEdgeX) ||
                (boostDir == BoostDirection::BOTH && (goingToCenterX || goingToEdgeX))) {

                boostedX = lastMousePos_.x + static_cast<int>(deltaX * dynamicBoost);

                if (goingToCenterX) {
                    if ((deltaX > 0 && boostedX > center.x) || (deltaX < 0 && boostedX < center.x)) {
                        boostedX = center.x;
                    }
                }
                else if (goingToEdgeX) {
                    if (boostedX < 0) boostedX = 0;
                    if (boostedX > screenW) boostedX = screenW;
                }
            }

            // Apply Y boost based on direction
            if ((boostDir == BoostDirection::TO_CENTER && goingToCenterY) ||
                (boostDir == BoostDirection::TO_EDGE && goingToEdgeY) ||
                (boostDir == BoostDirection::BOTH && (goingToCenterY || goingToEdgeY))) {

                boostedY = lastMousePos_.y + static_cast<int>(deltaY * dynamicBoost);

                if (goingToCenterY) {
                    if ((deltaY > 0 && boostedY > center.y) || (deltaY < 0 && boostedY < center.y)) {
                        boostedY = center.y;
                    }
                }
                else if (goingToEdgeY) {
                    if (boostedY < 0) boostedY = 0;
                    if (boostedY > screenH) boostedY = screenH;
                }
            }

            if (boostedX != currentPos.x || boostedY != currentPos.y) {
                SetCursorPos(boostedX, boostedY);
                currentPos.x = boostedX;
                currentPos.y = boostedY;

                // Reset idle timer when we move the cursor
                lastMovementTime_ = currentTime;
            }

            float targetX = (static_cast<float>(currentPos.x) / screenW) * 2.0f - 1.0f;
            float targetY = -((static_cast<float>(currentPos.y) / screenH) * 2.0f - 1.0f);

            currentStickX_ = std::clamp(targetX, -1.0f, 1.0f);
            currentStickY_ = std::clamp(targetY, -1.0f, 1.0f);

            result.isIdle = false;
        }
        else {
            result.isIdle = (currentTime - lastMovementTime_) > IDLE_TIMEOUT_MS;
        }

        // Auto-center only when idle and not in fast return
        if (result.isIdle && config.IsAutoCenterEnabled() && !fastReturnActive_) {
            CenterMouseIfIdle();
            GetCursorPos(&lastMousePos_);
        }

        result.x = currentStickX_;
        result.y = currentStickY_;
        result.isRelativeMode = false;

        lastMousePos_ = currentPos;

        return result;
    }
    /// @}

    // ------------------------------------------------------------------------
    /// @name Helper Methods
    /// @{
    /// @brief Center mouse when idle
    void CenterMouseIfIdle() {
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);

        POINT center = { screenW / 2, screenH / 2 };
        POINT currentPos;
        GetCursorPos(&currentPos);

        if (abs(currentPos.x - center.x) < CENTER_THRESHOLD &&
            abs(currentPos.y - center.y) < CENTER_THRESHOLD) {
            return;
        }

        SetCursorPos(center.x, center.y);
        lastMousePos_.x = center.x;
        lastMousePos_.y = center.y;
        Sleep(1);
    }
    /// @}
};

// ============================================================================
/// @class EmulationEngine
/// @brief Main emulation thread managing controller updates
// ============================================================================
class EmulationEngine {
private:
    // ------------------------------------------------------------------------
    /// @name Internal Structures
    /// @{
    struct StickState {
        float curX = 0.0f, curY = 0.0f;
        float lastX = 0.0f, lastY = 0.0f;
    };

    struct ButtonState {
        unsigned short lastButtons = 0;
        unsigned char  lastLT = 0, lastRT = 0;
    };
    /// @}

    // ------------------------------------------------------------------------
    /// @name Members
    /// @{
    std::thread                     thread_;
    std::atomic<bool>               running_{ false };
    std::chrono::steady_clock::time_point lastFrameTime_;
    const ThreadSafeConfig& config_;
    EmulationState& state_;
    InputMapper                      inputMapper_;

    StickState   leftStick_, rightStick_;
    ButtonState  lastState_;
    bool         lastMouseMode_ = false;
    /// @}

public:
    // ------------------------------------------------------------------------
    /// @name Lifetime
    /// @{
    EmulationEngine(const ThreadSafeConfig& config, EmulationState& state)
        : config_(config), state_(state) {
    }

    ~EmulationEngine() { Stop(); }
    /// @}

    // ------------------------------------------------------------------------
    /// @name Control
    /// @{
    /// @brief Start the emulation thread
    void Start() {
        if (running_.exchange(true)) {
            Logger::Warning("EmulationEngine already running");
            return;
        }

        state_.Start();
        thread_ = std::thread(&EmulationEngine::Run, this);

        HANDLE handle = thread_.native_handle();
        SetThreadPriority(handle, THREAD_PRIORITY_TIME_CRITICAL);

        Logger::Info("EmulationEngine started");
    }

    /// @brief Stop the emulation thread
    void Stop() {
        if (!running_.exchange(false)) return;

        state_.Stop();

        if (thread_.joinable()) thread_.join();

        Logger::Info("EmulationEngine stopped");
    }
    /// @}

private:
    // ------------------------------------------------------------------------
    /// @name Main Loop
    /// @{
    /// @brief Thread entry point
    void Run() {
        try {
            Emulator emu;
            if (!emu.IsReady()) {
                throw std::runtime_error("Failed to initialize emulator: " +
                    emu.GetLastError());
            }

            Logger::Info("Emulation loop started");

            while (running_ && !state_.ShouldExit()) {
                if (state_.IsActive()) ProcessFrame(emu);
                std::this_thread::sleep_for(std::chrono::milliseconds(8));
            }

            emu.ResetState();
            Logger::Info("Emulation loop ended gracefully");
        }
        catch (const std::exception& e) {
            Logger::Error(std::string("Emulation thread crashed: ") + e.what());
            running_ = false;
            state_.Stop();
        }
        catch (...) {
            Logger::Error("Emulation thread crashed: unknown exception");
            running_ = false;
            state_.Stop();
        }
    }

    /// @brief Process a single frame of emulation
    void ProcessFrame(Emulator& emu) {
        bool currentMode = config_.IsMouseRelativeMode();
        bool fastReturnMode = config_.IsFastReturnMode();

        if (currentMode != lastMouseMode_) {
            inputMapper_.ResetSpringState();
            lastMouseMode_ = currentMode;

            std::string modeStr = "ABSOLUTE";
            if (currentMode)       modeStr = "SPRING/RELATIVE";
            else if (fastReturnMode) modeStr = "ABSOLUTE BOOST";

            Logger::Info("Mouse mode switched to: " + modeStr);
        }

        auto mouse = inputMapper_.GetMouseAxis(config_);

        float targetLSX = 0.0f, targetLSY = 0.0f;
        float targetRSX = 0.0f, targetRSY = 0.0f;

        inputMapper_.CalculateStick(config_, "LS", targetLSX, targetLSY, mouse.isIdle);
        inputMapper_.CalculateStick(config_, "RS", targetRSX, targetRSY, mouse.isIdle);

        float smoothing = config_.GetSmoothingFactor();
        leftStick_.curX = Processor::Lerp(leftStick_.curX, targetLSX, smoothing);
        leftStick_.curY = Processor::Lerp(leftStick_.curY, targetLSY, smoothing);
        rightStick_.curX = Processor::Lerp(rightStick_.curX, targetRSX, smoothing);
        rightStick_.curY = Processor::Lerp(rightStick_.curY, targetRSY, smoothing);

        emu.report.sThumbLX = Processor::ToThumb(leftStick_.curX);
        emu.report.sThumbLY = Processor::ToThumb(leftStick_.curY);
        emu.report.sThumbRX = Processor::ToThumb(rightStick_.curX);
        emu.report.sThumbRY = Processor::ToThumb(rightStick_.curY);

        emu.report.wButtons = 0;
        UpdateButtons(emu);

        emu.report.bLeftTrigger = inputMapper_.CalculateTrigger(config_, "LT");
        emu.report.bRightTrigger = inputMapper_.CalculateTrigger(config_, "RT");

        if (!emu.SendUpdate()) {
            Logger::Warning("Failed to send update to ViGEm");
        }
    }

    /// @brief Update button states in the report
    void UpdateButtons(Emulator& emu) {
        static const std::pair<const char*, unsigned short> buttonMap[] = {
            {"A", XUSB_GAMEPAD_A},
            {"B", XUSB_GAMEPAD_B},
            {"X", XUSB_GAMEPAD_X},
            {"Y", XUSB_GAMEPAD_Y},
            {"LB", XUSB_GAMEPAD_LEFT_SHOULDER},
            {"RB", XUSB_GAMEPAD_RIGHT_SHOULDER},
            {"START", XUSB_GAMEPAD_START},
            {"BACK", XUSB_GAMEPAD_BACK},
            {"LS_CLICK", XUSB_GAMEPAD_LEFT_THUMB},
            {"RS_CLICK", XUSB_GAMEPAD_RIGHT_THUMB},
            {"DPAD_UP", XUSB_GAMEPAD_DPAD_UP},
            {"DPAD_DOWN", XUSB_GAMEPAD_DPAD_DOWN},
            {"DPAD_LEFT", XUSB_GAMEPAD_DPAD_LEFT},
            {"DPAD_RIGHT", XUSB_GAMEPAD_DPAD_RIGHT}
        };

        for (const auto& mapping : buttonMap) {
            if (config_.IsTriggered(mapping.first)) {
                emu.report.wButtons |= mapping.second;
            }
        }
    }
    /// @}
};

// ============================================================================
/// @class ConsoleUI
/// @brief Console-based user interface for configuration
// ============================================================================
class ConsoleUI {
private:
    // ------------------------------------------------------------------------
    /// @name Members
    /// @{
    ThreadSafeConfig& config_;
    EmulationState& state_;
    /// @}

public:
    // ------------------------------------------------------------------------
    /// @name Lifetime
    /// @{
    ConsoleUI(ThreadSafeConfig& config, EmulationState& state)
        : config_(config), state_(state) {
    }
    /// @}

    // ------------------------------------------------------------------------
    /// @name Main Menu
    /// @{
    /// @brief Show main menu and handle user input
    void ShowMainMenu() {
        SetConsoleTitleA("XVC++ - Xbox 360 Emulator");

        while (!state_.ShouldExit()) {
            ClearScreen();
            PrintHeader();
            PrintMenu();

            std::string input;
            std::cout << "\n  >> Select: ";

            if (!(std::cin >> input)) {
                HandleInputError();
                continue;
            }

            int selection = 0;
            try { selection = std::stoi(input); }
            catch (...) { selection = 0; }

            if (!ProcessMenuSelection(selection)) break;
        }
    }

    /// @brief Show exit message
    void ShowExitMessage() {
        ClearScreen();
        std::cout << "\n";
        PrintSeparator('=', 50);
        PrintCentered("THANK YOU FOR USING XVC++!", 50);
        PrintSeparator('=', 50);
        std::cout << "\n";
        if (Logger::IsInitialized()) {
            std::cout << "  Log: " << Logger::GetLogFileName() << "\n";
        }
        std::cout << "\n";
        Sleep(1000);
    }
    /// @}

private:
    // ------------------------------------------------------------------------
    /// @name Helpers
    /// @{
    [[nodiscard]] static std::string ToUpper(std::string str) {
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        return str;
    }

    static void ClearScreen() {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hConsole == INVALID_HANDLE_VALUE) return;

        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) return;

        DWORD consoleSize = csbi.dwSize.X * csbi.dwSize.Y;
        COORD topLeft = { 0, 0 };
        DWORD written;

        FillConsoleOutputCharacter(hConsole, ' ', consoleSize, topLeft, &written);
        FillConsoleOutputAttribute(hConsole, csbi.wAttributes, consoleSize, topLeft, &written);
        SetConsoleCursorPosition(hConsole, topLeft);
    }

    static void Pause() {
        std::cout << "\nPress any key...";
        _getch();
    }

    [[nodiscard]] static std::string VKToNameSafe(int vk) {
        if (vk == Key::VK_MOUSE_X) return "MOUSE_X";
        if (vk == Key::VK_MOUSE_Y) return "MOUSE_Y";
        if (vk == 0) return "NONE";

        char name[64] = { 0 };
        UINT scanCode = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
        if (scanCode != 0 && GetKeyNameTextA(scanCode << 16, name, sizeof(name))) {
            return std::string(name);
        }
        return "CODE_" + std::to_string(vk);
    }
    /// @}

    // ------------------------------------------------------------------------
    /// @name UI Utilities
    /// @{
    void SetConsoleColor(int color) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, color);
    }

    void PrintColored(const std::string& text, int color) {
        SetConsoleColor(color);
        std::cout << text;
        SetConsoleColor(7); // reset to default
    }

    void PrintSeparator(char c = '-', int length = 50) {
        std::cout << "  ";
        for (int i = 0; i < length; ++i) std::cout << c;
        std::cout << "\n";
    }

    void PrintCentered(const std::string& text, int width = 50) {
        int padding = (width - static_cast<int>(text.length())) / 2;
        std::cout << "  ";
        for (int i = 0; i < padding; ++i) std::cout << ' ';
        std::cout << text;
        for (int i = 0; i < padding; ++i) std::cout << ' ';
        if ((width - static_cast<int>(text.length())) % 2) std::cout << ' ';
        std::cout << "\n";
    }

    void HandleInputError() {
        std::cin.clear();
        std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
        Logger::Warning("Input error in menu");
    }
    /// @}

    // ------------------------------------------------------------------------
    /// @name Display Helpers
    /// @{
    [[nodiscard]] std::string GetModeString() const {
        if (config_.IsMouseRelativeMode()) return "SPRING/RELATIVE";
        if (config_.IsFastReturnMode())    return "ABSOLUTE BOOST";
        return "ABSOLUTE";
    }

    [[nodiscard]] int GetModeColor() const {
        if (config_.IsMouseRelativeMode()) return 13; // magenta
        if (config_.IsFastReturnMode())    return 10; // green
        return 11; // cyan
    }

    [[nodiscard]] std::string GetBoostDirectionString() const {
        return config_.IsFastReturnMode() ? config_.GetBoostDirectionString() : "N/A";
    }

    void PrintHeader() {
        std::cout << "\n";
        PrintSeparator('=', 50);
        PrintCentered("XVC++ CONTROL PANEL", 50);
        PrintSeparator('=', 50);
        std::cout << "\n";

        std::cout << "  System Status:\n";
        PrintSeparator('-', 50);

        std::cout << "    ENGINE: ";
        PrintColored(state_.IsActive() ? "ON" : "OFF", state_.IsActive() ? 10 : 12);
        std::cout << "\n";

        printf("    SMOOTHING: %.2f\n", config_.GetSmoothingFactor());

        std::cout << "    AUTO-CENTER: ";
        PrintColored(config_.IsAutoCenterEnabled() ? "ENABLED" : "DISABLED",
            config_.IsAutoCenterEnabled() ? 10 : 12);
        std::cout << "\n";

        std::cout << "    MOUSE MODE: ";
        PrintColored(GetModeString(), GetModeColor());
        std::cout << "\n";

        if (config_.IsFastReturnMode()) {
            std::cout << "    BOOST DIRECTION: ";
            PrintColored(GetBoostDirectionString(), 14);
            std::cout << "\n";
            printf("    BASE BOOST: %.1fx\n", config_.GetFastReturnBoost());
            printf("    SLOW THRESHOLD: %.1f | FAST THRESHOLD: %.1f\n",
                config_.GetSlowSpeedThreshold(), config_.GetFastSpeedThreshold());
            printf("    SLOW BOOST: %.2fx | FAST BOOST: %.2fx\n",
                config_.GetSlowBoostFactor(), config_.GetFastBoostFactor());
        }

        if (config_.IsMouseRelativeMode()) {
            printf("    SPRING SPEED: %.2f\n", config_.GetSpringReturnSpeed());
        }

        if (Logger::IsInitialized()) {
            std::cout << "    LOG: " << Logger::GetLogFileName() << "\n";
        }

        PrintSeparator('-', 50);
    }

    void PrintMenu() {
        std::cout << "\n";
        PrintSeparator('=', 50);
        PrintCentered("OPTIONS", 50);
        PrintSeparator('=', 50);
        std::cout << "\n";

        std::cout << "    [1] "; PrintColored("TOGGLE ENGINE", 14); std::cout << "\n";
        std::cout << "    [2] "; PrintColored("VIEW CONFIGURATION", 14); std::cout << "\n";
        std::cout << "    [3] "; PrintColored("MAP CONTROLS", 14); std::cout << "\n";
        std::cout << "    [4] "; PrintColored("ADJUST SMOOTHING", 14); std::cout << "\n";
        std::cout << "    [5] "; PrintColored("TOGGLE AUTO-CENTER", 14); std::cout << "\n";
        std::cout << "    [6] "; PrintColored("CYCLE MOUSE MODE", 14);
        std::cout << " ["; PrintColored(GetModeString(), GetModeColor()); std::cout << "]\n";

        if (config_.IsFastReturnMode()) {
            std::cout << "    [7] "; PrintColored("CYCLE BOOST DIRECTION", 14);
            std::cout << " ["; PrintColored(GetBoostDirectionString(), 14); std::cout << "]\n";
            std::cout << "    [8] "; PrintColored("ADJUST BASE BOOST", 14);
            std::cout << " ["; printf("%.1fx", config_.GetFastReturnBoost()); std::cout << "]\n";
            std::cout << "    [9] "; PrintColored("ADJUST SLOW THRESHOLD", 14);
            std::cout << " ["; printf("%.1f", config_.GetSlowSpeedThreshold()); std::cout << "]\n";
            std::cout << "   [10] "; PrintColored("ADJUST FAST THRESHOLD", 14);
            std::cout << " ["; printf("%.1f", config_.GetFastSpeedThreshold()); std::cout << "]\n";
            std::cout << "   [11] "; PrintColored("ADJUST SLOW BOOST", 14);
            std::cout << " ["; printf("%.2fx", config_.GetSlowBoostFactor()); std::cout << "]\n";
            std::cout << "   [12] "; PrintColored("ADJUST FAST BOOST", 14);
            std::cout << " ["; printf("%.2fx", config_.GetFastBoostFactor()); std::cout << "]\n";
        }
        else if (config_.IsMouseRelativeMode()) {
            std::cout << "    [7] "; PrintColored("ADJUST SPRING SPEED", 14);
            std::cout << " ["; printf("%.2f", config_.GetSpringReturnSpeed()); std::cout << "]\n";
        }

        std::cout << "\n";
        std::cout << "   [13] "; PrintColored("EXIT PROGRAM", 12); std::cout << "\n";
        std::cout << "\n";
        PrintSeparator('-', 50);
    }
    /// @}

    // ------------------------------------------------------------------------
    /// @name Menu Actions
    /// @{
    bool ProcessMenuSelection(int sel) {
        switch (sel) {
        case 1: ToggleEngine(); return true;
        case 2: ShowCurrentMappings(); return true;
        case 3: MapControls(); return true;
        case 4: AdjustSmoothing(); return true;
        case 5:
            config_.ToggleAutoCenter();
            PrintColored("\n  [OK] Auto-center " +
                std::string(config_.IsAutoCenterEnabled() ? "enabled" : "disabled") + "\n", 10);
            Sleep(500); return true;
        case 6: CycleMouseMode(); return true;
        case 7:
            if (config_.IsFastReturnMode()) CycleBoostDirection();
            else if (config_.IsMouseRelativeMode()) AdjustSpringSpeed();
            else { PrintColored("\n  [!] Not available\n", 12); Sleep(1000); }
            return true;
        case 8:
            if (config_.IsFastReturnMode()) AdjustFastReturnBoost();
            else { PrintColored("\n  [!] Only in BOOST mode\n", 12); Sleep(1000); }
            return true;
        case 9:
            if (config_.IsFastReturnMode()) AdjustSlowThreshold();
            else { PrintColored("\n  [!] Only in BOOST mode\n", 12); Sleep(1000); }
            return true;
        case 10:
            if (config_.IsFastReturnMode()) AdjustFastThreshold();
            else { PrintColored("\n  [!] Only in BOOST mode\n", 12); Sleep(1000); }
            return true;
        case 11:
            if (config_.IsFastReturnMode()) AdjustSlowBoost();
            else { PrintColored("\n  [!] Only in BOOST mode\n", 12); Sleep(1000); }
            return true;
        case 12:
            if (config_.IsFastReturnMode()) AdjustFastBoost();
            else { PrintColored("\n  [!] Only in BOOST mode\n", 12); Sleep(1000); }
            return true;
        case 13:
            Logger::Info("User selected exit");
            return false;
        default:
            PrintColored("\n  [!] Invalid option.\n", 12);
            Logger::Warning("Invalid menu option: " + std::to_string(sel));
            Sleep(1000); return true;
        }
    }

    void CycleMouseMode() {
        if (config_.IsMouseRelativeMode()) {
            config_.ToggleMouseMode();
            if (config_.IsFastReturnMode()) config_.ToggleFastReturnMode();
            PrintColored("\n  [OK] Mouse mode: ABSOLUTE\n", 11);
        }
        else if (config_.IsFastReturnMode()) {
            config_.ToggleFastReturnMode();
            config_.ToggleMouseMode();
            PrintColored("\n  [OK] Mouse mode: SPRING/RELATIVE\n", 13);
        }
        else {
            config_.ToggleFastReturnMode();
            PrintColored("\n  [OK] Mouse mode: ABSOLUTE BOOST\n", 10);
        }
        Sleep(500);
    }

    void CycleBoostDirection() {
        config_.CycleBoostDirection();
        PrintColored("\n  [OK] Boost direction: " + config_.GetBoostDirectionString() + "\n", 10);
        Sleep(500);
    }

    void ToggleEngine() {
        if (state_.IsActive()) {
            state_.Stop();
            PrintColored("\n  [OFF] Engine stopped\n", 12);
        }
        else {
            state_.Start();
            PrintColored("\n  [ON] Engine started\n", 10);
        }
        Sleep(500);
    }

    void ShowCurrentMappings() {
        Logger::Info("Showing current mappings");
        ClearScreen();

        PrintSeparator('=', 50);
        PrintCentered("CURRENT CONFIGURATION", 50);
        PrintSeparator('=', 50);
        std::cout << "\n";

        try {
            for (const auto& btn : ThreadSafeConfig::VALID_BUTTONS) {
                std::string keyName = config_.GetKeyNameForAction(btn);
                printf("    %-12s -> ", btn.c_str());
                PrintColored(keyName + "\n", keyName == "NONE" ? 8 : 10);
            }
        }
        catch (const std::exception& e) {
            PrintColored("  [ERROR] " + std::string(e.what()) + "\n", 12);
            Logger::Error(std::string("Error showing mappings: ") + e.what());
        }

        std::cout << "\n";
        Pause();
    }

    void MapControls() {
        Logger::Info("Entering mapping mode");
        std::string button, newKey;

        while (!state_.ShouldExit()) {
            ClearScreen();
            PrintSeparator('=', 50);
            PrintCentered("CONTROL MAPPING", 50);
            PrintSeparator('=', 50);
            PrintColored("  Type 'EXIT' to return\n\n", 8);

            std::cout << "  >> Xbox button: ";
            if (!(std::cin >> button)) { HandleInputError(); continue; }

            button = ToUpper(button);
            if (button == "EXIT") { Logger::Info("Exiting mapping mode"); return; }

            if (!ThreadSafeConfig::IsValidButton(button)) {
                PrintColored("\n  [!] Invalid button.\n", 12);
                Logger::Warning("Attempt to map invalid button: " + button);
                Pause(); continue;
            }

            std::cout << "  >> Assign to (ENTER for none): ";
            std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
            std::getline(std::cin, newKey);
            newKey = ToUpper(newKey);

            if (newKey.empty() || newKey == "EXIT") {
                if (newKey == "EXIT") return;
                newKey = "NONE";
            }

            int vk = Key::StringToVK(newKey);
            if (vk != 0 || newKey == "NONE") {
                config_.UpdateMapping(button, newKey);
                PrintColored("\n  [OK] " + button + " -> " + newKey + "\n", 10);
                std::cout << "\n  >> Press any key...";
                _getch();
            }
            else {
                PrintColored("\n  [!] Key not recognized.\n", 12);
                Logger::Warning("Unrecognized key: " + newKey);
                Pause();
            }
        }
    }

    void AdjustSmoothing() {
        Logger::Info("Adjusting smoothing");
        std::string input;
        float current = config_.GetSmoothingFactor();

        while (true) {
            ClearScreen();
            PrintSeparator('=', 50);
            PrintCentered("SMOOTHING ADJUSTMENT", 50);
            PrintSeparator('=', 50);
            printf("\n    Current: %.2f\n", current);
            std::cout << "    Range: 0.01 - 1.0\n\n";
            std::cout << "  >> New value (or EXIT): ";

            if (!(std::cin >> input)) { HandleInputError(); continue; }

            if (ToUpper(input) == "EXIT") {
                Logger::Info("Smoothing cancelled");
                return;
            }

            try {
                float value = std::stof(input);
                config_.UpdateSmoothing(value);
                PrintColored("\n  [OK] Smoothing updated\n", 10);
                Sleep(500); return;
            }
            catch (...) {
                PrintColored("\n  [!] Invalid value\n", 12);
                HandleInputError(); Pause();
            }
        }
    }

    void AdjustSpringSpeed() {
        Logger::Info("Adjusting spring speed");
        std::string input;
        float current = config_.GetSpringReturnSpeed();

        while (true) {
            ClearScreen();
            PrintSeparator('=', 50);
            PrintCentered("SPRING RETURN SPEED", 50);
            PrintSeparator('=', 50);
            printf("\n    Current: %.2f\n", current);
            std::cout << "    Range: 0.01 - 1.0\n\n";
            std::cout << "  >> New value (or EXIT): ";

            if (!(std::cin >> input)) { HandleInputError(); continue; }

            if (ToUpper(input) == "EXIT") {
                Logger::Info("Spring speed cancelled");
                return;
            }

            try {
                float value = std::stof(input);
                config_.UpdateSpringSpeed(value);
                PrintColored("\n  [OK] Spring speed updated\n", 10);
                Sleep(500); return;
            }
            catch (...) {
                PrintColored("\n  [!] Invalid value\n", 12);
                HandleInputError(); Pause();
            }
        }
    }

    void AdjustFastReturnBoost() {
        Logger::Info("Adjusting base boost");
        std::string input;
        float current = config_.GetFastReturnBoost();

        while (true) {
            ClearScreen();
            PrintSeparator('=', 50);
            PrintCentered("BASE BOOST", 50);
            PrintSeparator('=', 50);
            printf("\n    Current: %.1fx\n", current);
            std::cout << "    Range: 1.1x - 5.0x\n\n";
            std::cout << "  >> New value (or EXIT): ";

            if (!(std::cin >> input)) { HandleInputError(); continue; }

            if (ToUpper(input) == "EXIT") {
                Logger::Info("Base boost cancelled");
                return;
            }

            try {
                float value = std::stof(input);
                config_.UpdateFastReturnBoost(value);
                PrintColored("\n  [OK] Base boost updated\n", 10);
                Sleep(500); return;
            }
            catch (...) {
                PrintColored("\n  [!] Invalid value\n", 12);
                HandleInputError(); Pause();
            }
        }
    }

    void AdjustSlowThreshold() {
        Logger::Info("Adjusting slow threshold");
        std::string input;
        float current = config_.GetSlowSpeedThreshold();

        while (true) {
            ClearScreen();
            PrintSeparator('=', 50);
            PrintCentered("SLOW THRESHOLD", 50);
            PrintSeparator('=', 50);
            printf("\n    Current: %.1f\n", current);
            std::cout << "    Range: 1.0 - 100.0\n\n";
            std::cout << "  >> New value (or EXIT): ";

            if (!(std::cin >> input)) { HandleInputError(); continue; }

            if (ToUpper(input) == "EXIT") {
                Logger::Info("Slow threshold cancelled");
                return;
            }

            try {
                float value = std::stof(input);
                config_.UpdateSlowSpeedThreshold(value);
                PrintColored("\n  [OK] Slow threshold updated\n", 10);
                Sleep(500); return;
            }
            catch (...) {
                PrintColored("\n  [!] Invalid value\n", 12);
                HandleInputError(); Pause();
            }
        }
    }

    void AdjustFastThreshold() {
        Logger::Info("Adjusting fast threshold");
        std::string input;
        float current = config_.GetFastSpeedThreshold();

        while (true) {
            ClearScreen();
            PrintSeparator('=', 50);
            PrintCentered("FAST THRESHOLD", 50);
            PrintSeparator('=', 50);
            printf("\n    Current: %.1f\n", current);
            std::cout << "    Range: 1.0 - 100.0\n\n";
            std::cout << "  >> New value (or EXIT): ";

            if (!(std::cin >> input)) { HandleInputError(); continue; }

            if (ToUpper(input) == "EXIT") {
                Logger::Info("Fast threshold cancelled");
                return;
            }

            try {
                float value = std::stof(input);
                config_.UpdateFastSpeedThreshold(value);
                PrintColored("\n  [OK] Fast threshold updated\n", 10);
                Sleep(500); return;
            }
            catch (...) {
                PrintColored("\n  [!] Invalid value\n", 12);
                HandleInputError(); Pause();
            }
        }
    }

    void AdjustSlowBoost() {
        Logger::Info("Adjusting slow boost");
        std::string input;
        float current = config_.GetSlowBoostFactor();

        while (true) {
            ClearScreen();
            PrintSeparator('=', 50);
            PrintCentered("SLOW BOOST FACTOR", 50);
            PrintSeparator('=', 50);
            printf("\n    Current: %.2fx\n", current);
            std::cout << "    Range: 0.3x - 2.0x\n\n";
            std::cout << "  >> New value (or EXIT): ";

            if (!(std::cin >> input)) { HandleInputError(); continue; }

            if (ToUpper(input) == "EXIT") {
                Logger::Info("Slow boost cancelled");
                return;
            }

            try {
                float value = std::stof(input);
                config_.UpdateSlowBoostFactor(value);
                PrintColored("\n  [OK] Slow boost updated\n", 10);
                Sleep(500); return;
            }
            catch (...) {
                PrintColored("\n  [!] Invalid value\n", 12);
                HandleInputError(); Pause();
            }
        }
    }

    void AdjustFastBoost() {
        Logger::Info("Adjusting fast boost");
        std::string input;
        float current = config_.GetFastBoostFactor();

        while (true) {
            ClearScreen();
            PrintSeparator('=', 50);
            PrintCentered("FAST BOOST FACTOR", 50);
            PrintSeparator('=', 50);
            printf("\n    Current: %.2fx\n", current);
            std::cout << "    Range: 0.3x - 2.0x\n\n";
            std::cout << "  >> New value (or EXIT): ";

            if (!(std::cin >> input)) { HandleInputError(); continue; }

            if (ToUpper(input) == "EXIT") {
                Logger::Info("Fast boost cancelled");
                return;
            }

            try {
                float value = std::stof(input);
                config_.UpdateFastBoostFactor(value);
                PrintColored("\n  [OK] Fast boost updated\n", 10);
                Sleep(500); return;
            }
            catch (...) {
                PrintColored("\n  [!] Invalid value\n", 12);
                HandleInputError(); Pause();
            }
        }
    }
    /// @}
};

// ============================================================================
/// @class Application
/// @brief Main application coordinator
// ============================================================================
class Application {
private:
    std::unique_ptr<ThreadSafeConfig> config_;
    std::unique_ptr<EmulationState>   state_;
    std::unique_ptr<EmulationEngine>  engine_;
    std::unique_ptr<ConsoleUI>        ui_;

public:
    /// @brief Run the application
    int Run() {
        try {
            Initialize();
            MainLoop();
            Shutdown();
            return 0;
        }
        catch (const std::exception& e) {
            std::cerr << "[FATAL] " << e.what() << "\n";
            Logger::Error(std::string("Fatal error: ") + e.what());
            return 1;
        }
    }

private:
    void Initialize() {
        if (!Logger::Initialize("xvcpp_emulator.log", LogLevel::INFO_LEVEL)) {
            std::cout << "[INFO] Logger not available\n";
        }
        else {
            Logger::Info("XVC++ Emulator - Starting");
        }

        config_ = std::make_unique<ThreadSafeConfig>();
        state_ = std::make_unique<EmulationState>();
        engine_ = std::make_unique<EmulationEngine>(*config_, *state_);
        ui_ = std::make_unique<ConsoleUI>(*config_, *state_);

        engine_->Start();
        Logger::Info("System initialized");
    }

    void MainLoop() {
        if (ui_) ui_->ShowMainMenu();
    }

    void Shutdown() {
        Logger::Info("Shutting down...");
        if (engine_) engine_->Stop();
        if (ui_) ui_->ShowExitMessage();
        Logger::Shutdown();
    }
};

// ============================================================================
/// @name Entry Point
/// @{
// ============================================================================
int main() {
    MMRESULT timerRes = timeBeginPeriod(1);
    if (timerRes != TIMERR_NOERROR) {
        Logger::Warning("Could not set 1ms timer resolution");
    }

    int exitCode = 0;
    try {
        Application app;
        exitCode = app.Run();
    }
    catch (const std::exception& e) {
        Logger::Fatal(std::string("Unhandled exception: ") + e.what());
        exitCode = 1;
    }

    if (timerRes == TIMERR_NOERROR) timeEndPeriod(1);
    return exitCode;
}
/// @}