#pragma once

// ============================================================================
/// @file Emulator.hpp
/// @brief ViGEm-based Xbox 360 controller emulation with RAII management
/// @provides Connection handling, report queuing, and automatic reconnection
// ============================================================================

// ============================================================================
// DEPENDENCIES
// ============================================================================
#include <windows.h>
#undef NOMINMAX

#include <ViGEM/Client.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>

#include "Logger.hpp"

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================
class Emulator;
class ViGEmClientHandle;
class ViGEmTargetHandle;
struct EmulationReport;

// ============================================================================
/// @namespace EmulatorConfig
/// @brief Configuration constants for emulator behavior
// ============================================================================
namespace EmulatorConfig {
    /// @name Retry Configuration
    ///@{
    constexpr int MAX_RETRY_ATTEMPTS = 3;      ///< Maximum reconnection attempts
    constexpr int INITIAL_RETRY_DELAY_MS = 1000;   ///< First retry delay (ms)
    constexpr int MAX_RETRY_DELAY_MS = 5000;   ///< Maximum retry delay (ms)
    ///@}

    /// @name Queue Configuration
    ///@{
    constexpr int REPORT_QUEUE_SIZE = 16;          ///< Maximum queued reports
    ///@}
}

// ============================================================================
/// @name Exception Classes
/// @{
// ============================================================================
/// @brief Base exception for emulator errors
class EmulatorException : public std::runtime_error {
public:
    explicit EmulatorException(const std::string& msg) : std::runtime_error(msg) {}
};

/// @brief Exception for ViGEm connection failures
class ViGEmConnectionException : public EmulatorException {
public:
    explicit ViGEmConnectionException(const std::string& msg)
        : EmulatorException("ViGEm Connection Failed: " + msg) {
    }
};

/// @brief Exception for ViGEm target creation failures
class ViGEmTargetException : public EmulatorException {
public:
    explicit ViGEmTargetException(const std::string& msg)
        : EmulatorException("ViGEm Target Error: " + msg) {
    }
};
/// @}

// ============================================================================
/// @struct EmulationReport
/// @brief Timestamped controller report for queue management
// ============================================================================
struct EmulationReport {
    XUSB_REPORT xusbReport;                               ///< Raw XUSB report
    std::chrono::steady_clock::time_point timestamp;     ///< Creation time
    bool valid = false;                                   ///< Report validity flag

    /// @brief Default constructor (invalid report)
    EmulationReport() {
        XUSB_REPORT_INIT(&xusbReport);
        valid = false;
    }

    /// @brief Construct from XUSB report (automatically timestamped)
    explicit EmulationReport(const XUSB_REPORT& report)
        : xusbReport(report)
        , timestamp(std::chrono::steady_clock::now())
        , valid(true) {
    }
};

// ============================================================================
/// @class ViGEmClientHandle
/// @brief RAII wrapper for PVIGEM_CLIENT
/// @details Automatically disconnects and frees the client on destruction
// ============================================================================
class ViGEmClientHandle {
private:
    PVIGEM_CLIENT client_ = nullptr;  ///< Raw ViGEm client handle

public:
    /// @name Constructors & Destructor
    ///@{
    ViGEmClientHandle() = default;
    explicit ViGEmClientHandle(PVIGEM_CLIENT c) : client_(c) {}
    ~ViGEmClientHandle() { Reset(); }
    ///@}

    /// @name Copy Operations (Deleted - RAII handles cannot be copied)
    ///@{
    ViGEmClientHandle(const ViGEmClientHandle&) = delete;
    ViGEmClientHandle& operator=(const ViGEmClientHandle&) = delete;
    ///@}

    /// @name Move Operations
    ///@{
    ViGEmClientHandle(ViGEmClientHandle&& other) noexcept
        : client_(other.client_) {
        other.client_ = nullptr;
    }

    ViGEmClientHandle& operator=(ViGEmClientHandle&& other) noexcept {
        if (this != &other) {
            Reset();
            client_ = other.client_;
            other.client_ = nullptr;
        }
        return *this;
    }
    ///@}

    /// @name Accessors
    ///@{
    /// @brief Get raw client handle
    [[nodiscard]] PVIGEM_CLIENT Get() const { return client_; }

    /// @brief Check if handle is valid
    [[nodiscard]] bool IsValid() const { return client_ != nullptr; }

    /// @brief Bool conversion (same as IsValid)
    explicit operator bool() const { return IsValid(); }
    ///@}

    /// @brief Reset the handle (disconnect and free)
    void Reset() {
        if (client_) {
            vigem_disconnect(client_);
            vigem_free(client_);
            client_ = nullptr;
        }
    }
};

// ============================================================================
/// @class ViGEmTargetHandle
/// @brief RAII wrapper for PVIGEM_TARGET
/// @details Automatically removes and frees the target on destruction
// ============================================================================
class ViGEmTargetHandle {
private:
    PVIGEM_CLIENT client_ = nullptr;  ///< Associated client (needed for removal)
    PVIGEM_TARGET target_ = nullptr;  ///< Raw ViGEm target handle

public:
    /// @name Constructors & Destructor
    ///@{
    ViGEmTargetHandle() = default;
    ViGEmTargetHandle(PVIGEM_CLIENT c, PVIGEM_TARGET t) : client_(c), target_(t) {}
    ~ViGEmTargetHandle() { Reset(); }
    ///@}

    /// @name Copy Operations (Deleted - RAII handles cannot be copied)
    ///@{
    ViGEmTargetHandle(const ViGEmTargetHandle&) = delete;
    ViGEmTargetHandle& operator=(const ViGEmTargetHandle&) = delete;
    ///@}

    /// @name Move Operations
    ///@{
    ViGEmTargetHandle(ViGEmTargetHandle&& other) noexcept
        : client_(other.client_), target_(other.target_) {
        other.client_ = nullptr;
        other.target_ = nullptr;
    }

    ViGEmTargetHandle& operator=(ViGEmTargetHandle&& other) noexcept {
        if (this != &other) {
            Reset();
            client_ = other.client_;
            target_ = other.target_;
            other.client_ = nullptr;
            other.target_ = nullptr;
        }
        return *this;
    }
    ///@}

    /// @name Accessors
    ///@{
    /// @brief Get raw target handle
    [[nodiscard]] PVIGEM_TARGET Get() const { return target_; }

    /// @brief Check if handle is valid
    [[nodiscard]] bool IsValid() const { return target_ != nullptr; }

    /// @brief Bool conversion (same as IsValid)
    explicit operator bool() const { return IsValid(); }
    ///@}

    /// @brief Reset the target (remove from bus and free)
    void Reset() {
        if (target_ && client_) {
            vigem_target_remove(client_, target_);
            vigem_target_free(target_);
        }
        target_ = nullptr;
        client_ = nullptr;
    }
};

// ============================================================================
/// @class Emulator
/// @brief Main ViGEm controller emulation class
/// @details Manages connection, report queuing, and automatic reconnection
// ============================================================================
class Emulator {
private:
    // ------------------------------------------------------------------------
    /// @name Resources
    /// @{
    ViGEmClientHandle client_;  ///< ViGEm client handle
    ViGEmTargetHandle target_;  ///< Xbox 360 target handle
    /// @}

    // ------------------------------------------------------------------------
    /// @name State
    /// @{
    std::atomic<bool> initialized_{ false };   ///< Initialization flag
    std::atomic<bool> connected_{ false };     ///< Connection flag
    std::atomic<int>  retryCount_{ 0 };        ///< Current retry attempt count

    mutable std::mutex stateMutex_;            ///< Mutex for error state
    std::string lastError_;                     ///< Last error message
    /// @}

    // ------------------------------------------------------------------------
    /// @name Report Queue
    /// @{
    mutable std::mutex queueMutex_;                         ///< Queue mutex
    std::queue<EmulationReport> reportQueue_;               ///< Report queue
    size_t maxQueueSize_ = EmulatorConfig::REPORT_QUEUE_SIZE; ///< Max queue size
    /// @}

    // ------------------------------------------------------------------------
    /// @name Current Report
    /// @{
    mutable std::mutex reportMutex_;            ///< Report mutex
    XUSB_REPORT currentReport_;                 ///< Current controller state
    bool reportDirty_ = false;                   ///< Pending update flag
    /// @}

public:
    // ------------------------------------------------------------------------
    /// @name Public Access (Compatibility)
    /// @{
    XUSB_REPORT& report = currentReport_;  ///< Direct report access (legacy)
    /// @}

    // ------------------------------------------------------------------------
    /// @name Lifetime Management
    /// @{
    /// @brief Constructor - initializes ViGEm connection
    /// @throws ViGEmConnectionException on initialization failure
    Emulator() {
        XUSB_REPORT_INIT(&currentReport_);
        ResetReport();
        if (!Initialize()) throw ViGEmConnectionException(lastError_);
    }

    /// @brief Destructor - clean shutdown
    ~Emulator() { Shutdown(); }

    /// @brief Copy operations deleted
    Emulator(const Emulator&) = delete;
    Emulator& operator=(const Emulator&) = delete;

    /// @brief Move constructor
    Emulator(Emulator&& other) noexcept
        : client_(std::move(other.client_))
        , target_(std::move(other.target_))
        , initialized_(other.initialized_.exchange(false))
        , connected_(other.connected_.exchange(false))
        , retryCount_(other.retryCount_.load())
        , lastError_(std::move(other.lastError_))
        , currentReport_(other.currentReport_)
        , reportDirty_(other.reportDirty_) {
        std::lock_guard lock(other.reportMutex_);
        reportQueue_ = std::move(other.reportQueue_);
    }
    /// @}

    // ------------------------------------------------------------------------
    /// @name Report Access
    /// @{
    /// @brief Get mutable reference to current report
    XUSB_REPORT& GetReport() {
        std::lock_guard lock(reportMutex_);
        reportDirty_ = true;
        return currentReport_;
    }

    /// @brief Get const reference to current report
    [[nodiscard]] const XUSB_REPORT& GetReport() const {
        std::lock_guard lock(reportMutex_);
        return currentReport_;
    }

    /// @brief Set current report and queue it
    void SetReport(const XUSB_REPORT& r) {
        {
            std::lock_guard lock(reportMutex_);
            currentReport_ = r;
            reportDirty_ = true;
        }
        QueueReport(r);
    }
    /// @}

    // ------------------------------------------------------------------------
    /// @name Control Operations
    /// @{
    /// @brief Send current report to the virtual controller
    /// @return true if successful, false on error (auto-reconnect attempted)
    bool SendUpdate() {
        if (!IsReady()) {
            SetLastError("Emulator not initialized");
            return false;
        }

        XUSB_REPORT toSend;
        {
            std::lock_guard lock(reportMutex_);
            toSend = currentReport_;
            reportDirty_ = false;
        }

        VIGEM_ERROR status = vigem_target_x360_update(client_.Get(), target_.Get(), toSend);
        if (!VIGEM_SUCCESS(status)) {
            std::string err = ErrorCodeToString(status);
            SetLastError("Send error: " + err);
            if (status == VIGEM_ERROR_BUS_NOT_FOUND || status == VIGEM_ERROR_BUS_ACCESS_FAILED)
                return AttemptReconnect();
            return false;
        }

        retryCount_ = 0;
        return true;
    }

    /// @brief Send a specific report immediately
    /// @param r Report to send
    /// @return true if successful
    bool SendReport(const XUSB_REPORT& r) {
        if (!IsReady()) return false;
        return VIGEM_SUCCESS(vigem_target_x360_update(client_.Get(), target_.Get(), r));
    }

    /// @brief Reset controller to neutral state
    void ResetState() {
        XUSB_REPORT r;
        XUSB_REPORT_INIT(&r);
        r.sThumbLX = r.sThumbLY = r.sThumbRX = r.sThumbRY = 0;
        r.bLeftTrigger = r.bRightTrigger = 0;
        r.wButtons = 0;
        SetReport(r);
        if (IsReady()) SendUpdate();
        Logger::Info("Controller state reset");
    }
    /// @}

    // ------------------------------------------------------------------------
    /// @name Status Query
    /// @{
    /// @brief Check if emulator is ready for use
    [[nodiscard]] bool IsReady() const {
        return initialized_.load(std::memory_order_acquire) &&
            connected_.load(std::memory_order_acquire) &&
            client_ && target_;
    }

    /// @brief Check if initialized
    [[nodiscard]] bool IsInitialized() const {
        return initialized_.load(std::memory_order_acquire);
    }

    /// @brief Check if connected to ViGEm bus
    [[nodiscard]] bool IsConnected() const {
        return connected_.load(std::memory_order_acquire);
    }

    /// @brief Get last error message
    [[nodiscard]] std::string GetLastError() const {
        std::lock_guard lock(stateMutex_);
        return lastError_;
    }

    /// @brief Get current retry count
    [[nodiscard]] int GetRetryCount() const {
        return retryCount_.load();
    }
    /// @}

    // ------------------------------------------------------------------------
    /// @name Diagnostics
    /// @{
    /// @brief Print diagnostic information to console
    void PrintDiagnostics() const {
        std::cout << "\n=== EMULATOR DIAGNOSTICS ===\n"
            << "State: " << (IsReady() ? "READY" : "ERROR") << "\n"
            << "Init: " << (IsInitialized() ? "YES" : "NO") << "\n"
            << "Connected: " << (IsConnected() ? "YES" : "NO") << "\n"
            << "Retries: " << GetRetryCount() << "\n";

        {
            std::lock_guard lock(stateMutex_);
            if (!lastError_.empty()) std::cout << "Last error: " << lastError_ << "\n";
        }

        std::cout << "Client: " << (client_ ? "OK" : "FAIL") << "\n"
            << "Target: " << (target_ ? "OK" : "FAIL") << "\n";

        {
            std::lock_guard lock(queueMutex_);
            std::cout << "Queued: " << reportQueue_.size() << "\n";
        }
        std::cout << "============================\n";
    }

    /// @brief Print current controller state to console
    void PrintCurrentState() const {
        auto r = GetReport();
        std::cout << "\n=== CURRENT STATE ===\n"
            << "LS: X=" << r.sThumbLX << " Y=" << r.sThumbLY << "\n"
            << "RS: X=" << r.sThumbRX << " Y=" << r.sThumbRY << "\n"
            << "LT=" << (int)r.bLeftTrigger << " RT=" << (int)r.bRightTrigger << "\n"
            << "Buttons: " << (r.wButtons ? ButtonsToString(r.wButtons) : "NONE") << "\n"
            << "=====================\n";
    }
    /// @}

    // ------------------------------------------------------------------------
    /// @name Queue Management
    /// @{
    /// @brief Queue a report for later sending
    void QueueReport(const XUSB_REPORT& r) {
        std::lock_guard lock(queueMutex_);
        if (reportQueue_.size() >= maxQueueSize_) {
            reportQueue_.pop();
            Logger::Warning("Report queue full, dropping oldest");
        }
        reportQueue_.emplace(r);
    }

    /// @brief Send the oldest queued report
    /// @return true if a report was sent
    bool SendQueuedReport() {
        std::optional<EmulationReport> rep;
        {
            std::lock_guard lock(queueMutex_);
            if (reportQueue_.empty()) return false;
            rep = reportQueue_.front();
            reportQueue_.pop();
        }
        return rep && rep->valid ? SendReport(rep->xusbReport) : false;
    }

    /// @brief Clear all queued reports
    void ClearQueue() {
        std::lock_guard lock(queueMutex_);
        std::queue<EmulationReport> empty;
        std::swap(reportQueue_, empty);
    }
    /// @}

private:
    // ------------------------------------------------------------------------
    /// @name Initialization
    /// @{
    /// @brief Initialize ViGEm connection and target
    /// @return true if successful
    bool Initialize() {
        try {
            PVIGEM_CLIENT rawClient = vigem_alloc();
            if (!rawClient) throw std::runtime_error("Failed to allocate ViGEm client");
            client_ = ViGEmClientHandle(rawClient);

            VIGEM_ERROR conn = vigem_connect(client_.Get());
            if (!VIGEM_SUCCESS(conn)) throw ViGEmConnectionException(ErrorCodeToString(conn));
            connected_ = true;

            PVIGEM_TARGET rawTarget = vigem_target_x360_alloc();
            if (!rawTarget) throw ViGEmTargetException("Failed to create target");
            target_ = ViGEmTargetHandle(client_.Get(), rawTarget);

            VIGEM_ERROR add = vigem_target_add(client_.Get(), target_.Get());
            if (!VIGEM_SUCCESS(add)) throw ViGEmTargetException(ErrorCodeToString(add));

            ResetReport();
            initialized_ = true;
            retryCount_ = 0;
            Logger::Info("Emulator initialized");
            return true;
        }
        catch (const std::exception& e) {
            SetLastError(e.what());
            Cleanup();
            return false;
        }
    }

    /// @brief Clean up resources
    void Cleanup() {
        target_.Reset();
        client_.Reset();
        initialized_ = false;
        connected_ = false;
        Logger::Info("Emulator resources released");
    }

    /// @brief Shutdown gracefully
    void Shutdown() {
        if (IsReady()) ResetState();
        Cleanup();
        ClearQueue();
    }
    /// @}

    // ------------------------------------------------------------------------
    /// @name Reconnection
    /// @{
    /// @brief Attempt to reconnect after connection loss
    /// @return true if reconnection successful
    bool AttemptReconnect() {
        int cur = retryCount_.fetch_add(1) + 1;
        if (cur > EmulatorConfig::MAX_RETRY_ATTEMPTS) {
            SetLastError("Max retry attempts reached");
            return false;
        }

        int delay = EmulatorConfig::INITIAL_RETRY_DELAY_MS * (1 << (cur - 1));
        if (delay > EmulatorConfig::MAX_RETRY_DELAY_MS)
            delay = EmulatorConfig::MAX_RETRY_DELAY_MS;

        Logger::Warning("Reconnect " + std::to_string(cur) + "/" +
            std::to_string(EmulatorConfig::MAX_RETRY_ATTEMPTS) +
            " in " + std::to_string(delay) + "ms");

        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        Cleanup();

        if (Initialize()) {
            Logger::Info("Reconnect successful");
            retryCount_ = 0;
            return true;
        }
        return false;
    }
    /// @}

    // ------------------------------------------------------------------------
    /// @name Utilities
    /// @{
    /// @brief Reset report to neutral values
    void ResetReport() {
        std::lock_guard lock(reportMutex_);
        XUSB_REPORT_INIT(&currentReport_);
        currentReport_.sThumbLX = currentReport_.sThumbLY = 0;
        currentReport_.sThumbRX = currentReport_.sThumbRY = 0;
        currentReport_.bLeftTrigger = currentReport_.bRightTrigger = 0;
        currentReport_.wButtons = 0;
        reportDirty_ = false;
    }

    /// @brief Set last error and log it
    void SetLastError(const std::string& err) {
        std::lock_guard lock(stateMutex_);
        lastError_ = err;
        Logger::Error("Emulator: " + err);
    }

    /// @brief Convert VIGEM_ERROR to readable string
    [[nodiscard]] static std::string ErrorCodeToString(VIGEM_ERROR e) {
        switch (e) {
        case VIGEM_ERROR_NONE: return "SUCCESS";
        case VIGEM_ERROR_BUS_NOT_FOUND: return "Driver not found";
        case VIGEM_ERROR_NO_FREE_SLOT: return "No free slots";
        case VIGEM_ERROR_INVALID_TARGET: return "Invalid target";
        case VIGEM_ERROR_REMOVAL_FAILED: return "Remove failed";
        case VIGEM_ERROR_ALREADY_CONNECTED: return "Already connected";
        case VIGEM_ERROR_TARGET_UNINITIALIZED: return "Target uninit";
        case VIGEM_ERROR_TARGET_NOT_PLUGGED_IN: return "Target not plugged";
        case VIGEM_ERROR_BUS_VERSION_MISMATCH: return "Version mismatch";
        case VIGEM_ERROR_BUS_ACCESS_FAILED: return "Access denied";
        case VIGEM_ERROR_CALLBACK_ALREADY_REGISTERED: return "Callback exists";
        case VIGEM_ERROR_CALLBACK_NOT_FOUND: return "Callback not found";
        case VIGEM_ERROR_BUS_ALREADY_CONNECTED: return "Bus connected";
        case VIGEM_ERROR_XUSB_USERINDEX_OUT_OF_RANGE: return "User index out of range";
        default: return "Unknown (" + std::to_string(e) + ")";
        }
    }

    /// @brief Convert button flags to readable string
    [[nodiscard]] static std::string ButtonsToString(unsigned short b) {
        std::string r;
        if (b & XUSB_GAMEPAD_A) r += "A ";
        if (b & XUSB_GAMEPAD_B) r += "B ";
        if (b & XUSB_GAMEPAD_X) r += "X ";
        if (b & XUSB_GAMEPAD_Y) r += "Y ";
        if (b & XUSB_GAMEPAD_LEFT_SHOULDER) r += "LB ";
        if (b & XUSB_GAMEPAD_RIGHT_SHOULDER) r += "RB ";
        if (b & XUSB_GAMEPAD_START) r += "START ";
        if (b & XUSB_GAMEPAD_BACK) r += "BACK ";
        if (b & XUSB_GAMEPAD_LEFT_THUMB) r += "LS ";
        if (b & XUSB_GAMEPAD_RIGHT_THUMB) r += "RS ";
        if (b & XUSB_GAMEPAD_DPAD_UP) r += "UP ";
        if (b & XUSB_GAMEPAD_DPAD_DOWN) r += "DOWN ";
        if (b & XUSB_GAMEPAD_DPAD_LEFT) r += "LEFT ";
        if (b & XUSB_GAMEPAD_DPAD_RIGHT) r += "RIGHT ";
        return r.empty() ? "NONE" : r;
    }
    /// @}
};