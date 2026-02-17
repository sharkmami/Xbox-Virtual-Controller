#pragma once

// ============================================================================
/// @file Processor.hpp
/// @brief Core mathematical processing for Xbox 360 controller emulation
/// @provides Deadzone, response curves, normalization and format conversion
// ============================================================================

// ============================================================================
// DEPENDENCIES
// ============================================================================
#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "Logger.hpp"

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================
class InputProcessor;
class MathUtils;
class Processor;
struct Vector2D;

// ============================================================================
/// @namespace ProcessorConfig
/// @brief Constants for Xbox 360 controller ranges and defaults
// ============================================================================
namespace ProcessorConfig {
    /// @name Xbox 360 Hardware Ranges
    ///@{
    constexpr float THUMB_MIN = -32768.0f;  ///< Minimum thumbstick value
    constexpr float THUMB_MAX = 32767.0f;  ///< Maximum thumbstick value
    constexpr float TRIGGER_MIN = 0.0f;  ///< Minimum trigger value
    constexpr float TRIGGER_MAX = 255.0f;  ///< Maximum trigger value
    ///@}

    /// @name Normalized Ranges (-1.0 to 1.0)
    ///@{
    constexpr float NORM_MIN = -1.0f;
    constexpr float NORM_MAX = 1.0f;
    ///@}

    /// @name Default Processing Values
    ///@{
    constexpr float DEFAULT_CURVE = 1.0f;  ///< Linear response curve
    constexpr float DEFAULT_DEADZONE = 0.0f;  ///< No deadzone by default
    ///@}

    /// @name Numerical Precision
    ///@{
    constexpr float EPSILON = 1e-6f;  ///< Small value for floating point comparisons
    ///@}
}

// ============================================================================
/// @class ProcessorException
/// @brief Exception thrown by processor operations
// ============================================================================
class ProcessorException : public std::runtime_error {
public:
    explicit ProcessorException(const std::string& msg) noexcept
        : std::runtime_error("Processor Error: " + msg) {}
};

// ============================================================================
/// @struct Vector2D
/// @brief 2D vector with common operations for thumbstick processing
// ============================================================================
struct Vector2D {
    float x = 0.0f;  ///< X component
    float y = 0.0f;  ///< Y component

    /// @name Constructors
    ///@{
    constexpr Vector2D() = default;
    constexpr Vector2D(float xv, float yv) noexcept : x(xv), y(yv) {}
    ///@}

    /// @name Magnitude Operations
    ///@{
    /// @brief Calculate vector magnitude (length)
    /// @return Length of the vector
    [[nodiscard]] float Magnitude() const noexcept {
        return std::sqrt(x * x + y * y);
    }

    /// @brief Calculate squared magnitude (faster, no sqrt)
    /// @return Squared length of the vector
    [[nodiscard]] constexpr float MagnitudeSquared() const noexcept {
        return x * x + y * y;
    }

    /// @brief Normalize the vector in-place (unit length)
    /// @post If magnitude > EPSILON, vector becomes unit length
    void Normalize() noexcept {
        float mag = Magnitude();
        if (mag > ProcessorConfig::EPSILON) {
            x /= mag;
            y /= mag;
        }
    }

    /// @brief Get normalized copy of the vector
    /// @return Unit length vector (or zero if magnitude < EPSILON)
    [[nodiscard]] Vector2D Normalized() const noexcept {
        Vector2D res = *this;
        res.Normalize();
        return res;
    }
    ///@}

    /// @name Operators
    ///@{
    [[nodiscard]] constexpr Vector2D operator+(const Vector2D& o) const noexcept { return { x + o.x, y + o.y }; }
    [[nodiscard]] constexpr Vector2D operator-(const Vector2D& o) const noexcept { return { x - o.x, y - o.y }; }
    [[nodiscard]] constexpr Vector2D operator*(float s) const noexcept { return { x * s,   y * s }; }
    constexpr Vector2D& operator+=(const Vector2D& o) noexcept { x += o.x; y += o.y; return *this; }
    ///@}
};

// ============================================================================
/// @class MathUtils
/// @brief Static mathematical utilities for interpolation and range mapping
// ============================================================================
class MathUtils {
public:
    /// @brief Clamp a value between min and max
    /// @tparam T Arithmetic type (integral or floating point)
    /// @param v Value to clamp
    /// @param min Minimum allowed value
    /// @param max Maximum allowed value
    /// @return Clamped value
    template<typename T>
    [[nodiscard]] static constexpr T Clamp(T v, T min, T max) noexcept {
        static_assert(std::is_arithmetic_v<T>);
        if constexpr (std::is_floating_point_v<T>) {
            if (std::isnan(v)) return min;
            if (std::isinf(v)) return (v > 0) ? max : min;
        }
        return (v < min) ? min : (v > max ? max : v);
    }

    /// @name Interpolation Functions
    ///@{
    /// @brief Linear interpolation
    /// @param a Start value
    /// @param b End value
    /// @param t Interpolation factor [0,1]
    /// @return Interpolated value
    [[nodiscard]] static constexpr float Lerp(float a, float b, float t) noexcept {
        t = Clamp(t, 0.0f, 1.0f);
        return a + t * (b - a);
    }

    /// @brief Smooth step interpolation (ease-in/out)
    /// @param a Start value
    /// @param b End value
    /// @param t Interpolation factor [0,1]
    /// @return Smoothly interpolated value
    [[nodiscard]] static float SmoothStep(float a, float b, float t) noexcept {
        t = Clamp(t, 0.0f, 1.0f);
        t = t * t * (3.0f - 2.0f * t);
        return Lerp(a, b, t);
    }

    /// @brief Exponential interpolation
    /// @param a Start value
    /// @param b End value
    /// @param t Interpolation factor [0,1]
    /// @param sharpness Exponential sharpness (higher = more aggressive)
    /// @return Exponentially interpolated value
    [[nodiscard]] static float ExpLerp(float a, float b, float t, float sharpness = 2.0f) noexcept {
        t = Clamp(t, 0.0f, 1.0f);
        float expT = (std::exp(t * sharpness) - 1.0f) / (std::exp(sharpness) - 1.0f);
        return Lerp(a, b, expT);
    }
    ///@}

    /// @brief Map a value from one range to another
    /// @param v Value to map
    /// @param inMin Input range minimum
    /// @param inMax Input range maximum
    /// @param outMin Output range minimum
    /// @param outMax Output range maximum
    /// @return Mapped value
    [[nodiscard]] static float MapRange(float v, float inMin, float inMax,
        float outMin, float outMax) noexcept {
        if (std::abs(inMax - inMin) < ProcessorConfig::EPSILON) {
            Logger::Warning("MapRange: zero input range");
            return outMin;
        }
        float norm = (v - inMin) / (inMax - inMin);
        return Lerp(outMin, outMax, norm);
    }
};

// ============================================================================
/// @class InputProcessor
/// @brief Main processing pipeline for converting inputs to Xbox 360 format
// ============================================================================
class InputProcessor {
public:
    /// @name Normalization
    ///@{
    /// @brief Normalize a value to [-1.0, 1.0] range
    /// @tparam T Arithmetic type
    /// @param value Raw input value
    /// @param min Minimum possible raw value
    /// @param max Maximum possible raw value
    /// @return Normalized value in [-1.0, 1.0]
    /// @throws ProcessorException if min == max
    template<typename T>
    [[nodiscard]] static float Normalize(T value, T min, T max) {
        static_assert(std::is_arithmetic_v<T>);
        if (max == min) throw ProcessorException("Normalize: zero range");
        if (max < min) { std::swap(min, max); Logger::Warning("Normalize: swapped min/max"); }

        float fv = static_cast<float>(value);
        float fmin = static_cast<float>(min);
        float fmax = static_cast<float>(max);

        float norm01 = (fv - fmin) / (fmax - fmin);
        float result = norm01 * 2.0f - 1.0f;
        return MathUtils::Clamp(result, ProcessorConfig::NORM_MIN, ProcessorConfig::NORM_MAX);
    }

    /// @brief Safe normalization with default fallback
    /// @tparam T Arithmetic type
    /// @param value Raw input value
    /// @param min Minimum possible raw value
    /// @param max Maximum possible raw value
    /// @param def Default value if normalization fails
    /// @return Normalized value or default
    template<typename T>
    [[nodiscard]] static float NormalizeSafe(T value, T min, T max, float def = 0.0f) noexcept {
        try { return Normalize(value, min, max); }
        catch (const ProcessorException& e) {
            Logger::Warning(std::string("NormalizeSafe: ") + e.what());
            return def;
        }
    }
    ///@}

    /// @name Xbox 360 Format Conversion
    ///@{
    /// @brief Convert normalized float to thumbstick short
    /// @param value Value in [-1.0, 1.0]
    /// @return Thumbstick value in [-32768, 32767]
    [[nodiscard]] static short ToThumb(float value) noexcept {
        if (std::abs(value) < ProcessorConfig::EPSILON) value = 0.0f;
        float scaled = MathUtils::MapRange(value,
            ProcessorConfig::NORM_MIN, ProcessorConfig::NORM_MAX,
            ProcessorConfig::THUMB_MIN, ProcessorConfig::THUMB_MAX);
        return static_cast<short>(std::round(scaled));
    }

    /// @brief Convert normalized float to trigger byte
    /// @param value Value in [0.0, 1.0]
    /// @return Trigger value in [0, 255]
    [[nodiscard]] static unsigned char ToTrigger(float value) noexcept {
        value = MathUtils::Clamp(value, 0.0f, 1.0f);
        if (value < ProcessorConfig::EPSILON) return 0;
        return static_cast<unsigned char>(std::round(value * ProcessorConfig::TRIGGER_MAX));
    }
    ///@}

    /// @name Response Curves
    ///@{
    /// @brief Apply exponential response curve
    /// @param value Input value [-1.0, 1.0]
    /// @param curve Curve exponent (1.0 = linear)
    /// @return Value with curve applied
    [[nodiscard]] static float ApplyResponseCurve(float value, float curve = ProcessorConfig::DEFAULT_CURVE) noexcept {
        if (std::abs(curve - 1.0f) < ProcessorConfig::EPSILON) return value;
        float sign = (value < 0.0f) ? -1.0f : 1.0f;
        return sign * std::pow(std::abs(value), curve);
    }

    /// @brief Apply dual-zone advanced curve
    /// @param value Input value [-1.0, 1.0]
    /// @param lowCurve Exponent for low range
    /// @param highCurve Exponent for high range
    /// @param thresh Threshold between low/high zones
    /// @return Value with advanced curve applied
    [[nodiscard]] static float ApplyAdvancedCurve(float value, float lowCurve = 2.0f,
        float highCurve = 0.5f, float thresh = 0.6f) noexcept {
        float av = std::abs(value);
        float sign = (value < 0.0f) ? -1.0f : 1.0f;
        float res;
        if (av < thresh) {
            float t = av / thresh;
            res = std::pow(t, lowCurve) * thresh;
        }
        else {
            float t = (av - thresh) / (1.0f - thresh);
            res = thresh + std::pow(t, highCurve) * (1.0f - thresh);
        }
        return sign * MathUtils::Clamp(res, 0.0f, 1.0f);
    }
    ///@}

    /// @name Deadzone Processing
    ///@{
    /// @brief Apply axial deadzone to single axis
    /// @param value Input value [-1.0, 1.0]
    /// @param deadzone Deadzone radius [0.0, 1.0]
    /// @return Value with deadzone applied
    [[nodiscard]] static float ApplyDeadzone(float value, float deadzone) noexcept {
        deadzone = MathUtils::Clamp(deadzone, 0.0f, 1.0f);
        if (deadzone < ProcessorConfig::EPSILON) return value;

        float av = std::abs(value);
        if (av < deadzone) return 0.0f;

        float sign = (value < 0.0f) ? -1.0f : 1.0f;
        float scaled = (av - deadzone) / (1.0f - deadzone);
        return sign * MathUtils::Clamp(scaled, 0.0f, 1.0f);
    }

    /// @brief Apply radial deadzone to thumbstick (preserves direction)
    /// @param stick Input vector
    /// @param deadzone Deadzone radius [0.0, 1.0]
    /// @return Vector with radial deadzone applied
    [[nodiscard]] static Vector2D ApplyRadialDeadzone(Vector2D stick, float deadzone) noexcept {
        deadzone = MathUtils::Clamp(deadzone, 0.0f, 1.0f);
        float mag = stick.Magnitude();
        if (mag < deadzone) return { 0.0f, 0.0f };
        float scale = (mag - deadzone) / (1.0f - deadzone) / mag;
        return stick * scale;
    }
    ///@}

    /// @name Complete Processing Pipelines
    ///@{
    /// @brief Process single thumbstick axis
    /// @param raw Raw input value [-1.0, 1.0]
    /// @param deadzone Deadzone to apply
    /// @param curve Response curve exponent
    /// @return Processed thumbstick value
    [[nodiscard]] static short ProcessThumb(float raw, float deadzone = 0.0f,
        float curve = ProcessorConfig::DEFAULT_CURVE) noexcept {
        float dz = ApplyDeadzone(raw, deadzone);
        float cv = ApplyResponseCurve(dz, curve);
        return ToThumb(cv);
    }

    /// @brief Process 2D thumbstick (preserves angle)
    /// @param rx Raw X input [-1.0, 1.0]
    /// @param ry Raw Y input [-1.0, 1.0]
    /// @param[out] ox Processed X output
    /// @param[out] oy Processed Y output
    /// @param deadzone Deadzone to apply
    /// @param curve Response curve exponent
    static void ProcessThumb2D(float rx, float ry, short& ox, short& oy,
        float deadzone = 0.0f,
        float curve = ProcessorConfig::DEFAULT_CURVE) noexcept {
        Vector2D stick = ApplyRadialDeadzone({ rx, ry }, deadzone);
        stick.x = ApplyResponseCurve(stick.x, curve);
        stick.y = ApplyResponseCurve(stick.y, curve);
        ox = ToThumb(stick.x);
        oy = ToThumb(stick.y);
    }
    ///@}

    /// @name Smoothing and Interpolation
    ///@{
    /// @brief Frame-rate independent interpolation
    /// @param cur Current value
    /// @param tgt Target value
    /// @param speed Interpolation speed
    /// @param dt Delta time in seconds
    /// @return Interpolated value
    [[nodiscard]] static float LerpDeltaTime(float cur, float tgt, float speed, float dt) noexcept {
        float factor = 1.0f - std::exp(-speed * dt * 60.0f);
        return MathUtils::Lerp(cur, tgt, factor);
    }

    /// @brief Simple linear interpolation (frame-dependent)
    /// @param cur Current value
    /// @param tgt Target value
    /// @param speed Interpolation factor [0,1]
    /// @return Interpolated value
    [[nodiscard]] static float Lerp(float cur, float tgt, float speed) noexcept {
        return MathUtils::Lerp(cur, tgt, speed);
    }
    ///@}

    /// @name Utilities
    ///@{
    /// @brief Calculate delta time since last call
    /// @param[in,out] last Time point to update
    /// @return Delta time in seconds
    [[nodiscard]] static float CalculateDeltaTime(std::chrono::steady_clock::time_point& last) noexcept {
        auto now = std::chrono::steady_clock::now();
        auto dur = now - last;
        last = now;
        return std::chrono::duration<float>(dur).count();
    }

    /// @brief Calculate acceleration multiplier based on movement speed
    /// @param dx Movement delta X
    /// @param dy Movement delta Y
    /// @param thresh Speed threshold for acceleration
    /// @param gain Acceleration gain factor
    /// @return Acceleration multiplier [1.0, 3.0]
    [[nodiscard]] static float CalculateAcceleration(float dx, float dy, float thresh, float gain) noexcept {
        float mag = std::sqrt(dx * dx + dy * dy);
        if (mag < thresh) return 1.0f;
        float mult = 1.0f + (mag - thresh) * (gain / 10.0f);
        return std::clamp(mult, 1.0f, 3.0f);
    }
    ///@}
};

// ============================================================================
/// @class Processor
/// @brief Backwards compatibility alias for InputProcessor
// ============================================================================
class Processor : public InputProcessor {};