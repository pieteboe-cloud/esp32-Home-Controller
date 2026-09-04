#pragma once

#include <cstdint>
#include <functional>

// ============================================================================
//  AudioController.h - Audio input and beat/silence detection.
//
//  Uses MAX9814 electret microphone amplifier on GPIO32 (ADC1_CH4).
//  Performs RMS-based beat detection and silence detection.
//  Events published to EventBus for integration with scenes/scripts.
// ============================================================================

// Callback types
using BeatCallback = std::function<void()>;
using SilenceCallback = std::function<void(bool isSilent)>;

class AudioController {
public:
    // Constructor: takes ADC pin
    AudioController(uint8_t adcPin);

    // Initialize ADC and sampling
    void init();

    // Poll ADC and update beat/silence detection (call every loop)
    void update();

    // Register beat detection callback
    void onBeat(BeatCallback callback);

    // Register silence detection callback
    void onSilence(SilenceCallback callback);

    // Tune beat sensitivity: higher multiplier = harder to trigger beat
    void setSensitivity(float multiplier);

    // Tune silence threshold: lower voltage = more sensitive to silence
    void setSilenceThreshold(uint16_t thresholdMv);

    // Get current audio energy level (for debugging/UI)
    float getCurrentEnergy() const;

    // Get current silence state
    bool isSilent() const;

private:
    // ADC configuration
    uint8_t _adcPin;
    uint16_t _adcMax;  // Max ADC value for 12-bit (4095)

    // Circular buffer for audio samples
    uint16_t* _sampleBuffer;
    uint16_t _bufferIndex;
    uint16_t _sampleCount;  // Track how many samples collected

    // Energy tracking
    float _currentEnergy;      // RMS of current window
    float _averageEnergy;      // Running average (exponential smoothing)
    float _energySmoothAlpha;  // Smoothing factor (~0.1)

    // Beat detection
    BeatCallback _beatCallback;
    float _beatMultiplier;          // threshold = average * multiplier
    uint32_t _lastBeatTime;         // Debounce timer
    uint32_t _beatDebounceMs;

    // Silence detection
    SilenceCallback _silenceCallback;
    uint16_t _silenceThresholdMv;
    bool _audioPresent;             // Current silence state
    uint32_t _silenceStartTime;     // When quiet period started
    uint32_t _silenceTimeoutMs;     // How long to confirm silence

    // Non-blocking LED initialization
    bool _audioInitialized;         // Flag to track initialization completion
    uint32_t _lastLedUpdate;        // Last LED update time
    uint8_t _ledSequenceStep;       // Current step in LED sequence
    bool _ledState;                 // Current LED state
    bool _ledPulseActive;           // Non-blocking LED pulse in progress
    uint32_t _ledPulseStart;        // When the LED pulse started
    bool _silenceEventFired;        // Latch: silence event fired until audio resumes

    // Helper methods
    float calculateRMS();  // Calculate RMS energy from buffer
};
