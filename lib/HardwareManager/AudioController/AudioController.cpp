// ============================================================================
//  AudioController.cpp - Audio input and beat/silence detection implementation.
//
//  Samples from MAX9814 via ADC, calculates RMS energy in sliding windows,
//  and detects beats and silence events. Outputs simple "beat" and "silence" commands
//  with timestamps for integration with scenes/scripts.
// ============================================================================

#include "AudioController.h"


AudioController::AudioController(uint8_t adcPin)
    : _adcPin(adcPin),                    // ADC pin for audio input
      _adcMax(4095),                      // 12-bit ADC max value
      _sampleBuffer(nullptr),             // Circular buffer for audio samples
      _bufferIndex(0),                   // Current position in circular buffer
      _sampleCount(0),                   // Number of samples collected
      _currentEnergy(0.0f),              // Current RMS energy level
      _averageEnergy(0.0f),              // Running average of energy
      _energySmoothAlpha(0.1f),         // Smoothing factor for energy averaging
      _beatCallback(nullptr),            // Callback for beat detection
      _beatMultiplier(AUDIO_BEAT_MULTIPLIER),  // Beat sensitivity multiplier
      _lastBeatTime(0),                 // Last beat detection time
      _beatDebounceMs(AUDIO_BEAT_DEBOUNCE_MS), // Minimum time between beat detections
      _beatTimestamps(),                // History of beat timestamps for BPM calculation
      _currentBPM(0.0f),                // Current BPM estimate
      _lastBPMUpdateTime(0),            // Last BPM calculation time
      _silenceCallback(nullptr),         // Callback for silence detection
      _silenceThresholdMv(AUDIO_SILENCE_THRESHOLD_MV), // Silence detection threshold in mV
      _audioPresent(true),              // Current audio presence state
      _silenceStartTime(0),             // When silence period started
      _silenceTimeoutMs(AUDIO_SILENCE_TIMEOUT_MS), // How long to confirm silence
      _audioInitialized(false),         // Flag to track initialization completion
      _lastLedUpdate(0),                // Last LED update time
      _ledSequenceStep(0),              // Current step in LED sequence
      _ledState(LOW),                   // Current LED state
      _ledPulseActive(false),           // Non-blocking LED pulse in progress
      _ledPulseStart(0),                // When the LED pulse started
      _silenceEventFired(false),        // Latch: silence event fired until audio resumes
      _expectedBeatTime(0),            // When next beat is expected
      _missedBeatCount(0)              // Count of consecutive missed beats
{
}

void AudioController::init() {
    // Initialize LED pin for visual feedback
    pinMode(AUDIO_LED_PIN, OUTPUT);

    // Initialize LED state
    _ledState = LOW;
    digitalWrite(AUDIO_LED_PIN, _ledState);

    // Set up non-blocking LED sequence for initialization indication
    _lastLedUpdate = millis();
    _ledSequenceStep = 0;
    _audioInitialized = false;
    
    // Initialize silence detection state
    _audioPresent = true;  // Assume audio is present initially
    _silenceEventFired = false;
    _silenceStartTime = 0;
    
    // Initialize beat-based silence detection state
    _expectedBeatTime = 0;
    _missedBeatCount = 0;


    // Allocate circular sample buffer for audio data
    _sampleBuffer = new uint16_t[AUDIO_WINDOW_SIZE];
    if (!_sampleBuffer) {
#if DEBUG_LEVEL >= 1
        Debug::println(1, "[AUDIO][ERROR] Failed to allocate sample buffer");
#endif
        return;
    }

    // Initialize buffer with zeros
    for (uint16_t i = 0; i < AUDIO_WINDOW_SIZE; i++) {
        _sampleBuffer[i] = 0;
    }
    _bufferIndex = 0;
    _sampleCount = 0;

    // Configure ADC for audio input
    pinMode(_adcPin, INPUT);
    analogSetAttenuation(ADC_11db);  // Full range for 3.3V input
    // Note: ADC is set to maximum resolution in platformio.ini or default Arduino config

#if DEBUG_LEVEL >= 2
    Debug::println(2, "[AUDIO][INIT] AudioController initialized on pin " + String(_adcPin));
    Debug::println(2, "[AUDIO][INFO] Buffer size: " + String(AUDIO_WINDOW_SIZE) + " samples, sample rate: " + String(AUDIO_SAMPLE_RATE) + " Hz");
#endif
}

void AudioController::update() {
    // Handle non-blocking LED pulse for beat/silence indication
    // LED pulses for 50ms when beat or silence is detected
    if (_ledPulseActive && millis() - _ledPulseStart >= 50) {
        _ledPulseActive = false;
        digitalWrite(AUDIO_LED_PIN, LOW);
    }

    // Handle non-blocking LED initialization sequence
    // LED blinks 5 times during initialization to indicate the system is starting up
    if (!_audioInitialized && _ledSequenceStep < 5) {
        if (millis() - _lastLedUpdate >= 300) {  // 300ms between LED changes
            _lastLedUpdate = millis();
            _ledState = !_ledState;
            digitalWrite(AUDIO_LED_PIN, _ledState);
            _ledSequenceStep++;

            if (_ledSequenceStep >= 5) {
                _audioInitialized = true;
                _ledState = LOW;
                digitalWrite(AUDIO_LED_PIN, _ledState);
            }
        }
        // Continue with audio processing even during initialization
    }

    // Read one ADC sample from the microphone input
    uint16_t rawSample = analogRead(_adcPin);

    // Add to circular buffer for audio analysis
    _sampleBuffer[_bufferIndex] = rawSample;
    _bufferIndex = (_bufferIndex + 1) % AUDIO_WINDOW_SIZE;

    // Track how many samples we have (fills up over first window_size calls)
    if (_sampleCount < AUDIO_WINDOW_SIZE) {
        _sampleCount++;
    }

    // When buffer is full, analyze audio for beats and silence
    if (_sampleCount == AUDIO_WINDOW_SIZE) {
        // Calculate RMS energy from current window
        _currentEnergy = calculateRMS();

        // Exponential smoothing of energy average for more stable detection
        _averageEnergy = (_energySmoothAlpha * _currentEnergy) + ((1.0f - _energySmoothAlpha) * _averageEnergy);

        // Beat detection: trigger if current energy exceeds threshold
        // The threshold is based on the average energy multiplied by a sensitivity multiplier
        // Add a minimum energy requirement to avoid detecting noise as beats
        float beatThreshold = (_averageEnergy * _beatMultiplier);
        float minEnergyThreshold = _adcMax * 0.03f;  // Minimum energy threshold (3% of max ADC)
        
#if DEBUG_LEVEL >= 3
        Debug::println(3, "[AUDIO][BEAT_DEBUG] Energy: " + String(_currentEnergy, 1) + 
                      ", Threshold: " + String(beatThreshold, 1) + 
                      ", Min: " + String(minEnergyThreshold, 1) + 
                      ", Avg: " + String(_averageEnergy, 1));
#endif
        
        if (_currentEnergy > beatThreshold && _currentEnergy > minEnergyThreshold) {
            uint32_t now = millis();
            if (now - _lastBeatTime >= _beatDebounceMs) {  // Debounce to avoid multiple detections for the same beat
                _lastBeatTime = now;
                // Output beat command with timestamp
                outputBeat();
            }
        } else {
            // No beat detected - check if we should trigger silence based on missed beats
            uint32_t now = millis();
            if (_currentBPM > 0 && _expectedBeatTime > 0 && now > _expectedBeatTime) {
                _missedBeatCount++;
                
                // If we've missed enough beats, trigger silence
                if (_missedBeatCount >= _missedBeatThreshold && !_silenceEventFired) {
                    _silenceEventFired = true;
                    outputSilence();
#if DEBUG_LEVEL >= 2
                    Debug::println(2, "[AUDIO][BEAT_SILENCE] Silence triggered after " + String(_missedBeatCount) + " missed beats");
#endif
                }
            }
        }

        // Silence detection: check if RMS is below threshold
        // Convert ADC value to millivolts for comparison with threshold
        float currentMv = (_currentEnergy / _adcMax) * 3300.0f;  // Convert to mV
        bool wouldBeSilent = (currentMv < (_silenceThresholdMv * 0.9f));  // Slightly lower threshold for sensitivity

#if DEBUG_LEVEL >= 3
        Debug::println(3, "[AUDIO][SILENCE_DEBUG] Energy: " + String(_currentEnergy, 1) + 
                      ", mV: " + String(currentMv, 1) + 
                      ", Threshold: " + String(_silenceThresholdMv) + 
                      ", State: " + (_audioPresent ? "Audio" : "Silence") + 
                      ", EventFired: " + String(_silenceEventFired));
#endif

        if (wouldBeSilent) {
            // Audio is quiet
            if (_audioPresent) {
                // Just went quiet, start timer
                _silenceStartTime = millis();
                _audioPresent = false;
#if DEBUG_LEVEL >= 2
                Debug::println(2, "[AUDIO][SILENCE_START] Audio level dropped below threshold");
#endif
            } else if (!_silenceEventFired && millis() - _silenceStartTime >= (_silenceTimeoutMs * 0.7f)) {
                // Slightly reduced timeout period - fire once, latch until audio resumes
                _silenceEventFired = true;
                // Output silence command with timestamp
                outputSilence();
#if DEBUG_LEVEL >= 2
                Debug::println(2, "[AUDIO][SILENCE_EVENT] Silence event triggered");
#endif
            }
        } else {
            // Audio is loud
            if (!_audioPresent) {
                // Just came back from silence
                _audioPresent = true;
                _silenceEventFired = false;
                // Output audio resumed command with timestamp
                outputAudioResumed();
#if DEBUG_LEVEL >= 2
                Debug::println(2, "[AUDIO][SILENCE_END] Audio level returned above threshold");
#endif
            }
        }
    }
}

void AudioController::onBeat(BeatCallback callback) {
    // Register a callback function to be called when a beat is detected
    // The callback should not block execution and should be quick to execute
    _beatCallback = callback;
}

void AudioController::onSilence(SilenceCallback callback) {
    // Register a callback function to be called when silence state changes
    // The callback receives a boolean parameter: true for silence, false for audio resumed
    _silenceCallback = callback;
}

void AudioController::setSensitivity(float multiplier) {
    // Sets the beat detection sensitivity
    // Higher multiplier = harder to trigger a beat (more energy required)
    // Lower multiplier = easier to trigger a beat (less energy required)
    _beatMultiplier = multiplier;
}

void AudioController::setSilenceThreshold(uint16_t thresholdMv) {
    // Sets the silence detection threshold in millivolts
    // Lower threshold = more sensitive to silence (easier to detect silence)
    // Higher threshold = less sensitive to silence (harder to detect silence)
    _silenceThresholdMv = thresholdMv;
#if DEBUG_LEVEL >= 2
    Debug::println(2, "[AUDIO][SILENCE_THRESHOLD] Set to " + String(thresholdMv) + " mV");
#endif
}

float AudioController::getCurrentEnergy() const {
    // Returns the current RMS energy level of the audio signal
    // This is a measure of the current audio amplitude/volume
    // Higher values indicate louder audio, lower values indicate quieter audio
    return _currentEnergy;
}

bool AudioController::isSilent() const {
    // Returns true if audio is currently detected as silent
    // Based on whether the audio level has been below the threshold
    // for the configured timeout period
    return !_audioPresent;
}

float AudioController::getCurrentBPM() const {
    // Returns the current beats per minute (BPM) estimate
    // Calculated from the average interval between detected beats
    // Returns 0.0 if insufficient beat data is available
    return _currentBPM;
}

float AudioController::calculateRMS() {
    // Calculate RMS (root mean square) energy from sample buffer
    // RMS is a measure of the power in the audio signal
    // Higher values indicate louder audio, lower values indicate quieter audio
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

void AudioController::updateBPM() {
    // Update BPM calculation at regular intervals
    uint32_t now = millis();
    if (now - _lastBPMUpdateTime < _bpmUpdateInterval) {
        return;
    }
    
    _lastBPMUpdateTime = now;
    
    // Need at least 2 beat timestamps to calculate BPM
    if (_beatTimestamps.size() < 2) {
        _currentBPM = 0.0f;
        return;
    }
    
    // Calculate average interval between beats
    uint32_t totalInterval = 0;
    for (size_t i = 1; i < _beatTimestamps.size(); i++) {
        totalInterval += (_beatTimestamps[i] - _beatTimestamps[i-1]);
    }
    
    float avgInterval = (float)totalInterval / (_beatTimestamps.size() - 1);
    
    // Convert to BPM (beats per minute)
    if (avgInterval > 0) {
        _currentBPM = 60000.0f / avgInterval;
    } else {
        _currentBPM = 0.0f;
    }
    
#if DEBUG_LEVEL >= 3
    Debug::println(3, "[AUDIO][BPM] Current BPM: " + String(_currentBPM, 1) + 
                  " (based on " + String(_beatTimestamps.size()) + " beats)");
#endif
}

void AudioController::outputBeat() {
    // Output beat command with timestamp in the format "beat[timestamp]"
    uint32_t now = millis();
    String output = "beat[" + String(now) + "]";
    
#if DEBUG_LEVEL >= 3
    Debug::println(3, "[AUDIO][BEAT] " + output);
#endif
    
    // Flash LED on pin 12 (non-blocking 50ms pulse) for visual feedback
    digitalWrite(AUDIO_LED_PIN, HIGH);
    _ledPulseActive = true;
    _ledPulseStart = millis();
    
    // Add to beat history for BPM calculation
    _beatTimestamps.push_back(now);
    
    // Keep only the most recent beats to limit memory usage
    if (_beatTimestamps.size() > _maxBeatHistory) {
        _beatTimestamps.erase(_beatTimestamps.begin());
    }
    
    // Update BPM calculation
    updateBPM();
    
    // Call registered callback if any
    if (_beatCallback) {
        _beatCallback();
    }
}

void AudioController::outputSilence() {
    // Output silence command with timestamp in the format "silence[timestamp]"
    // This is called only once when silence is detected and latches until audio resumes
    uint32_t now = millis();
    String output = "silence[" + String(now) + "]";
    
#if DEBUG_LEVEL >= 3
    Debug::println(3, "[AUDIO][SILENCE] " + output);
#endif
    
    // Flash LED on pin 12 (non-blocking 50ms pulse) for visual feedback
    digitalWrite(AUDIO_LED_PIN, HIGH);
    _ledPulseActive = true;
    _ledPulseStart = millis();
    
    // Call registered callback if any
    if (_silenceCallback) {
        _silenceCallback(true);  // true indicates silence state
    }
}

void AudioController::outputAudioResumed() {
    // Output audio resumed command with timestamp in the format "audio_resumed[timestamp]"
    // This is called when audio resumes after a period of silence
    uint32_t now = millis();
    String output = "audio_resumed[" + String(now) + "]";
    
#if DEBUG_LEVEL >= 3
    Debug::println(3, "[AUDIO][RESUMED] " + output);
#endif
    
    // Flash LED on pin 12 (non-blocking 50ms pulse) for visual feedback
    digitalWrite(AUDIO_LED_PIN, HIGH);
    _ledPulseActive = true;
    _ledPulseStart = millis();
    
    // Reset beat-based silence detection when audio resumes
    _missedBeatCount = 0;
    _expectedBeatTime = 0;
    _silenceEventFired = false;
    
    // Call registered callback if any
    if (_silenceCallback) {
        _silenceCallback(false);  // false indicates audio resumed
    }
}
