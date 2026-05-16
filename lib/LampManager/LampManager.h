#ifndef LAMP_MANAGER_H
#define LAMP_MANAGER_H

#include <Arduino.h>
#include <LampTypes.h>

class ControlLivingColors;
class SettingsManager;

class LampManager {
public:
    static const int LampCount = 11;
    static const int PresetCount = 5;

    LampManager();

    bool init(ControlLivingColors* control, SettingsManager* settings, uint8_t (*lampAddresses)[9]);
    void update();

    void lock();
    void unlock();
    void waitForUnlock() const;

    void setRainbowActive(bool active);
    bool isRainbowActive() const;
    bool setEffect(const String& effectName);
    void clearEffect();
    bool isEffectActive() const;

    bool savePreset(int slot);
    bool loadPreset(int slot);
    String getPresetSummary() const;
    const LampState (*getPresets() const)[LampCount];

    void setLampColor(int index, uint8_t h, uint8_t s, uint8_t v);
    void turnLampOn(int index);
    void turnLampWhite(int index);
    void turnLampOff(int index);
    void setAllColor(uint8_t h, uint8_t s, uint8_t v);
    void setMasterBrightness(uint8_t value);

private:
    void loadPresetsFromFlash();
    void updateEffect();
    void applyStateToLamp(int index, const LampState& state);
    void applyAllStates(); // Send all lamp states in one batch
    bool validLampIndex(int index) const;

    LampState currentStates[LampCount];
    LampState presets[PresetCount][LampCount];
    ControlLivingColors* control = nullptr;
    SettingsManager* settings = nullptr;
    uint8_t (*lampAddresses)[9] = nullptr;

    enum EffectType {
        EFFECT_NONE,
        EFFECT_RAINBOW,
        EFFECT_DISCO,
        EFFECT_NEON_MADNESS,
        EFFECT_PSYCHEDLIC,
        EFFECT_ENERGY_BURST,
        EFFECT_VOLCANIC_RAGE,
        EFFECT_ICE_COLD,
        EFFECT_PURPLE_DREAM,
        EFFECT_SUNSET_FADE,
        EFFECT_RAVE
    };

    EffectType activeEffect = EFFECT_NONE;
    int effectStep = 0;
    unsigned long lastEffectUpdate = 0;
    volatile uint32_t stateLock = 0;
};

#endif // LAMP_MANAGER_H
