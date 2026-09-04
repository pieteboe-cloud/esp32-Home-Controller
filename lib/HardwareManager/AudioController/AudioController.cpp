// ============================================================================
//  AudioController.cpp - Audio input and beat/silence detection implementation.
//
//  Samples from MAX9814 via ADC, calculates RMS energy in sliding windows,
//  and detects beats and silence events for EventBus publication.
// ============================================================================

#include "AudioController.h"
#include "../Config.h"
#include "../../Debug/Debug.h"
#include <Arduino.h>
#include <cmath>

AudioController::AudioController(uint8_t adcPin)
    : _adcPin(adcPin),
      _adcMax(4095),  // 12-bit ADC
      _sampleBuffer(nullptr),
      _bufferIndex(0),
      _sampleCount(0),
      _currentEnergy(0.0f),
      _averageEnergy(0.0f),
      _energySmoothAlpha(0.1f),
      _beatCallback(nullptr),
      _beatMultiplier(AUDIO_BEAT_MULTIPLIER),
      _lastBeatTime(0),
      _beatDebounceMs(AUDIO_BEAT_DEBOUNCE_MS),
      _silenceCallback(nullptr),
      _silenceThresholdMv(AUDIO_SILENCE_THRESHOLD_MV),
      _audioPresent(true),
      _silenceStartTime(0),
      _silenceTimeoutMs(AUDIO_SILENCE_TIMEOUT_MS),
      _audioInitialized(false),
      _lastLedUpdate(0),
      _ledSequenceStep(0),
      _ledState(LOW),
      _ledPulseActive(false),
      _ledPulseStart(0),
      _silenceEventFired(false)
{
}

void AudioController::init() {
    pinMode(AUDIO_LED_PIN, OUTPUT);

    // Initialize LED state
    _ledState = LOW;
    digitalWrite(AUDIO_LED_PIN, _ledState);

    // Set up non-blocking LED sequence
    _lastLedUpdate = millis();
    _ledSequenceStep = 0;
    _audioInitialized = false;

    // Allocate circular sample buffer
    _sampleBuffer = new uint16_t[AUDIO_WINDOW_SIZE];
    if (!_sampleBuffer) {
#if DEBUG_LEVEL >= 1
        Debug::println(1, "[AUDIO][ERROR] Failed to allocate sample buffer");
#endif
        return;
    }

    // Initialize buffer
    for (uint16_t i = 0; i < AUDIO_WINDOW_SIZE; i++) {
        _sampleBuffer[i] = 0;
    }
    _bufferIndex = 0;
    _sampleCount = 0;

    // Configure ADC
    pinMode(_adcPin, INPUT);
    analogSetAttenuation(ADC_11db);  // Full range for 3.3V input
    // Note: ADC is set to maximum resolution in platformio.ini or default Arduino config

#if DEBUG_LEVEL >= 2
    Debug::println(2, "[AUDIO][INIT] AudioController initialized on pin " + String(_adcPin));
    Debug::println(2, "[AUDIO][INFO] Buffer size: " + String(AUDIO_WINDOW_SIZE) + " samples, sample rate: " + String(AUDIO_SAMPLE_RATE) + " Hz");
#endif
}

void AudioController::update() {
    // Non-blocking LED pulse handling (used for beat/silence indication)
    if (_ledPulseActive && millis() - _ledPulseStart >= 50) {
        _ledPulseActive = false;
        digitalWrite(AUDIO_LED_PIN, LOW);
    }

    // Handle non-blocking LED initialization sequence
    if (!_audioInitialized && _ledSequenceStep < 10) {
        if (millis() - _lastLedUpdate >= 500) {  // 500ms between LED changes
            _lastLedUpdate = millis();
            _ledState = !_ledState;
            digitalWrite(AUDIO_LED_PIN, _ledState);
            _ledSequenceStep++;

            if (_ledSequenceStep >= 10) {
                _audioInitialized = true;
                _ledState = LOW;
                digitalWrite(AUDIO_LED_PIN, _ledState);
            }
        }
        return;  // Skip audio processing until initialization is complete
    }

    // Read one ADC sample
    uint16_t rawSample = analogRead(_adcPin);

    // Add to circular buffer
    _sampleBuffer[_bufferIndex] = rawSample;
    _bufferIndex = (_bufferIndex + 1) % AUDIO_WINDOW_SIZE;

    // Track how many samples we have (fills up over first window_size calls)
    if (_sampleCount < AUDIO_WINDOW_SIZE) {
        _sampleCount++;
    }

    // When buffer is full, analyze
    if (_sampleCount == AUDIO_WINDOW_SIZE) {
        // Calculate RMS energy from current window
        _currentEnergy = calculateRMS();

        // Exponential smoothing of energy average
        _averageEnergy = (_energySmoothAlpha * _currentEnergy) + ((1.0f - _energySmoothAlpha) * _averageEnergy);

        // Beat detection: trigger if current energy exceeds threshold
        if (_currentEnergy > (_averageEnergy * _beatMultiplier)) {
            uint32_t now = millis();
            if (now - _lastBeatTime >= _beatDebounceMs) {
                _lastBeatTime = now;
                // Flash LED on pin 16 (non-blocking 50ms pulse)
                digitalWrite(AUDIO_LED_PIN, HIGH);
                _ledPulseActive = true;
                _ledPulseStart = millis();
                if (_beatCallback) {
                    _beatCallback();
                }
            }
        }

        // Silence detection: check if RMS is below threshold
        float currentMv = (_currentEnergy / _adcMax) * 3300.0f;  // Convert to mV
        bool wouldBeSilent = (currentMv < _silenceThresholdMv);

        if (wouldBeSilent) {
            // Audio is quiet
            if (_audioPresent) {
                // Just went quiet, start timer
                _silenceStartTime = millis();
                _audioPresent = false;
            } else if (!_silenceEventFired && millis() - _silenceStartTime >= _silenceTimeoutMs) {
                // Been quiet for timeout period - fire once, latch until audio resumes
                _silenceEventFired = true;
                // Flash LED on pin 16 (non-blocking 50ms pulse)
                digitalWrite(AUDIO_LED_PIN, HIGH);
                _ledPulseActive = true;
                _ledPulseStart = millis();
                if (_silenceCallback) {
                    _silenceCallback(true);  // true = silent
                }
            }
        } else {
            // Audio is loud
            if (!_audioPresent) {
                // Just came back
                _audioPresent = true;
                _silenceEventFired = false;
                if (_silenceCallback) {
                    _silenceCallback(false);  // false = audio resumed
                }
            }
        }
    }
}

void AudioController::onBeat(BeatCallback callback) {
    _beatCallback = callback;
}

void AudioController::onSilence(SilenceCallback callback) {
    _silenceCallback = callback;
}

void AudioController::setSensitivity(float multiplier) {
    _beatMultiplier = multiplier;
}

void AudioController::setSilenceThreshold(uint16_t thresholdMv) {
    _silenceThresholdMv = thresholdMv;
#if DEBUG_LEVEL >= 2
    Debug::println(2, "[AUDIO][SILENCE_THRESHOLD] Set to " + String(thresholdMv) + " mV");
#endif
}

float AudioController::getCurrentEnergy() const {
    return _currentEnergy;
}

bool AudioController::isSilent() const {
    return !_audioPresent;
}

float AudioController::calculateRMS() {
    // Calculate RMS (root mean square) energy from sample buffer
    if (_sampleCount == 0) {
        return 0.0f;
    }

    // Calculate mean of samples
    uint32_t sum = 0;
    for (uint16_t i = 0; i < AUDIO_WINDOW_SIZE; i++) {
        sum += _sampleBuffer[i];
    }
    float mean = (float)sum / AUDIO_WINDOW_SIZE;

    // Calculate sum of squared differences from mean
    float sumSquaredDiff = 0.0f;
    for (uint16_t i = 0; i < AUDIO_WINDOW_SIZE; i++) {
        float diff = (float)_sampleBuffer[i] - mean;
        sumSquaredDiff += diff * diff;
    }

    // RMS = sqrt(mean of squared differences)
    return sqrt(sumSquaredDiff / AUDIO_WINDOW_SIZE);
}
