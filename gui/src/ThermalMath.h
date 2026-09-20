#ifndef THERMAL_MATH_H
#define THERMAL_MATH_H

#include <cstdint>
#include <algorithm>
#include <cmath>

/** Sensor geometry used by Qianli / Topdon / InfiRay 0bda:5830 modules. */
inline constexpr int kThermalWidth = 256;
inline constexpr int kThermalHeight = 192;
inline constexpr int kFrameHeight = 384;

/**
 * Convert a packed 16-bit radiometric sample to Celsius.
 * Community formula from EEVblog / PyThermalCamera / Thermal-Camera-Redux.
 */
inline float rawToCelsius(uint16_t raw) {
  return (static_cast<float>(raw) / 64.0f) - 273.15f;
}

inline float celsiusToFahrenheit(float c) {
  return c * 9.0f / 5.0f + 32.0f;
}

/**
 * Decode one YUYV 256x384 frame into a 256x192 Celsius grid.
 * The bottom half stores little-endian uint16 samples in the YUYV bytes.
 */
inline void decodeRadiometricYuyv(
    const uint8_t *yuyv,
    int strideBytes,
    float *outCelsius,
    uint16_t *outKelvin = nullptr) {
  const uint8_t *bot = yuyv + static_cast<size_t>(strideBytes) * kThermalHeight;
  for (int row = 0; row < kThermalHeight; ++row) {
    const uint8_t *line = bot + static_cast<size_t>(row) * strideBytes;
    float *dst = outCelsius + row * kThermalWidth;
    uint16_t *kdst = outKelvin ? outKelvin + row * kThermalWidth : nullptr;
    for (int col = 0; col < kThermalWidth; ++col) {
      const uint16_t raw = static_cast<uint16_t>(line[col * 2])
          | (static_cast<uint16_t>(line[col * 2 + 1]) << 8);
      dst[col] = rawToCelsius(raw);
      if (kdst) {
        kdst[col] = raw;
      }
    }
  }
}

/** Extract the visual Y plane from the top 192 rows of a YUYV frame. */
inline void decodeVisualY(
    const uint8_t *yuyv,
    int strideBytes,
    uint8_t *outGray) {
  for (int row = 0; row < kThermalHeight; ++row) {
    const uint8_t *line = yuyv + static_cast<size_t>(row) * strideBytes;
    uint8_t *dst = outGray + row * kThermalWidth;
    for (int col = 0; col < kThermalWidth; ++col) {
      dst[col] = line[col * 2];
    }
  }
}

inline float clampf(float v, float lo, float hi) {
  return std::max(lo, std::min(hi, v));
}

#endif
