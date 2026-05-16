#include "LampManager.h"
#include "../ControlLivingColors/ControlLivingColors.h"
#include "../SettingsManager/SettingsManager.h"
#include "../Debug/Debug.h"

LampManager::LampManager() {
}

bool LampManager::init(ControlLivingColors* controlPtr, SettingsManager* settingsPtr, uint8_t (*lampAddressesPtr)[9]) {
    control = controlPtr;
    settings = settingsPtr;
    lampAddresses = lampAddressesPtr;

    if (!control || !settings || !lampAddresses) {
        Debug::println("[ERROR][LAMP] LampManager initialization missing required controllers");
        return false;
    }

    if (!settings->init()) {
        Debug::println("[ERROR][LAMP] SettingsManager failed to initialize during LampManager startup");
        return false;
    }

    loadPresetsFromFlash();

    for (int i = 0; i < LampCount; i++) {
        currentStates[i] = {0, 0, 255};
    }

    return true;
}

void LampManager::update() {
    if (activeEffect != EFFECT_NONE && millis() - lastEffectUpdate >= 45) {
        updateEffect();
    }
}

void LampManager::lock() {
    waitForUnlock();
    stateLock = 1;
}

void LampManager::unlock() {
    stateLock = 0;
}

void LampManager::waitForUnlock() const {
    while (stateLock) {
        yield();
    }
}

void LampManager::setRainbowActive(bool active) {
    if (active) {
        setEffect("rainbow_effect");
    } else {
        clearEffect();
    }
}

bool LampManager::isRainbowActive() const {
    return activeEffect == EFFECT_RAINBOW;
}

bool LampManager::setEffect(const String& effectName) {
    EffectType nextEffect = EFFECT_NONE;
    if (effectName == "rainbow_effect" || effectName == "rainbow") {
        nextEffect = EFFECT_RAINBOW;
    } else if (effectName == "disco_party") {
        nextEffect = EFFECT_DISCO;
    } else if (effectName == "neon_madness") {
        nextEffect = EFFECT_NEON_MADNESS;
    } else if (effectName == "psychedelic") {
        nextEffect = EFFECT_PSYCHEDLIC;
    } else if (effectName == "energy_burst") {
        nextEffect = EFFECT_ENERGY_BURST;
    } else if (effectName == "volcanic_rage") {
        nextEffect = EFFECT_VOLCANIC_RAGE;
    } else if (effectName == "ice_cold") {
        nextEffect = EFFECT_ICE_COLD;
    } else if (effectName == "purple_dream") {
        nextEffect = EFFECT_PURPLE_DREAM;
    } else if (effectName == "sunset_fade") {
        nextEffect = EFFECT_SUNSET_FADE;
    } else if (effectName == "rave_mode") {
        nextEffect = EFFECT_RAVE;
    } else {
        return false;
    }

    if (activeEffect == nextEffect) {
        return true;
    }

    lock();
    activeEffect = nextEffect;
    effectStep = 0;
    lastEffectUpdate = 0;
    for (int i = 0; i < LampCount; i++) {
        if (currentStates[i].v == 0) {
            currentStates[i].v = 255;
        }
    }
    unlock();
    return true;
}

void LampManager::clearEffect() {
    lock();
    activeEffect = EFFECT_NONE;
    effectStep = 0;
    lastEffectUpdate = 0;
    unlock();
}

bool LampManager::isEffectActive() const {
    return activeEffect != EFFECT_NONE;
}

bool LampManager::savePreset(int slot) {
    if (slot < 0 || slot >= PresetCount) {
        Debug::println("[ERROR][LAMP] Invalid preset slot");
        return false;
    }

    lock();
    for (int i = 0; i < LampCount; i++) {
        presets[slot][i] = currentStates[i];
    }
    bool success = settings->savePresets((const uint8_t*)presets, sizeof(presets));
    unlock();
    return success;
}

bool LampManager::loadPreset(int slot) {
    if (slot < 0 || slot >= PresetCount) {
        Debug::println("[ERROR][LAMP] Invalid preset slot");
        return false;
    }

    lock();
    for (int i = 0; i < LampCount; i++) {
        currentStates[i] = presets[slot][i];
    }
    applyAllStates();
    unlock();
    return true;
}

String LampManager::getPresetSummary() const {
    String summary = ",\"lamp_data\":{";
    for (int slot = 0; slot < PresetCount; slot++) {
        int configured = 0;
        for (int i = 0; i < LampCount; i++) {
            if (presets[slot][i].v > 0) {
                configured++;
            }
        }
        summary += "\"Preset_" + String(slot) + "\":\"" + String(configured) + " lamps configured\"";
        if (slot < PresetCount - 1) {
            summary += ",";
        }
    }
    summary += "}";
    return summary;
}

const LampState (*LampManager::getPresets() const)[LampCount] {
    return presets;
}

void LampManager::setLampColor(int index, uint8_t h, uint8_t s, uint8_t v) {
    if (!validLampIndex(index)) return;

    lock();
    currentStates[index] = {h, s, v};
    applyStateToLamp(index, currentStates[index]);
    unlock();
}

void LampManager::turnLampOn(int index) {
    if (!validLampIndex(index)) return;

    lock();
    currentStates[index].v = 255;
    applyStateToLamp(index, currentStates[index]);
    unlock();
}

void LampManager::turnLampWhite(int index) {
    if (!validLampIndex(index)) return;

    lock();
    currentStates[index] = {0, 0, 255};
    control->turnOnWhite(lampAddresses[index]);
    unlock();
}

void LampManager::turnLampOff(int index) {
    if (!validLampIndex(index)) return;

    lock();
    currentStates[index].v = 0;
    control->turnOff(lampAddresses[index]);
    unlock();
}

void LampManager::applyAllStates() {
    // Separate ON and OFF lamps into two batches
    const uint8_t* onAddrs[LampCount];
    uint8_t onH[LampCount], onS[LampCount], onV[LampCount];
    uint8_t onCount = 0;

    const uint8_t* offAddrs[LampCount];
    uint8_t offCount = 0;

    for (int i = 0; i < LampCount; i++) {
        if (currentStates[i].v == 0) {
            offAddrs[offCount++] = lampAddresses[i];
        } else {
            onAddrs[onCount] = lampAddresses[i];
            onH[onCount] = currentStates[i].h;
            onS[onCount] = currentStates[i].s;
            onV[onCount] = currentStates[i].v;
            onCount++;
        }
    }

    if (onCount > 0)
        control->turnOnWithColorBatch(onAddrs, onCount, onH, onS, onV);
    if (offCount > 0)
        control->turnOffBatch(offAddrs, offCount);
}

void LampManager::setAllColor(uint8_t h, uint8_t s, uint8_t v) {
    lock();
    for (int i = 0; i < LampCount; i++) {
        currentStates[i] = {h, s, v};
    }
    applyAllStates();
    unlock();
}

void LampManager::setMasterBrightness(uint8_t value) {
    lock();
    for (int i = 0; i < LampCount; i++) {
        currentStates[i].v = value;
    }
    applyAllStates();
    unlock();
}

void LampManager::loadPresetsFromFlash() {
    settings->loadPresets((uint8_t*)presets, sizeof(presets));
}

void LampManager::updateEffect() {
    lock();
    unsigned long now = millis();

    if (activeEffect == EFFECT_NONE || now - lastEffectUpdate < 40) {
        unlock();
        return;
    }

    switch (activeEffect) {
        case EFFECT_RAINBOW: {
            uint8_t h = (uint8_t)(now / 25);
            const uint8_t hueStep = 255 / LampCount;
            for (int i = 0; i < LampCount; i++) {
                currentStates[i].h = h;
                currentStates[i].s = 255;
                if (currentStates[i].v == 0) currentStates[i].v = 255;
                h += hueStep;
            }
            break;
        }
        case EFFECT_DISCO: {
            for (int i = 0; i < LampCount; i++) {
                if (random(10) > 4) {
                    uint8_t hue = (uint8_t)random(256);
                    currentStates[i] = {hue, 255, 255};
                }
            }
            break;
        }
        case EFFECT_NEON_MADNESS: {
            for (int i = 0; i < LampCount; i++) {
                uint8_t hue = (random(2) == 0) ? 230 : 130;
                uint8_t val = (random(10) > 2) ? 255 : 100;
                currentStates[i] = {hue, 255, val};
            }
            break;
        }
        case EFFECT_PSYCHEDLIC: {
            for (int i = 0; i < LampCount; i++) {
                uint8_t hue = (uint8_t)((now / 5) + (i * 40));
                currentStates[i] = {hue, 255, 255};
            }
            break;
        }
        case EFFECT_ENERGY_BURST: {
            bool flash = (random(20) > 8);
            for (int i = 0; i < LampCount; i++) {
                currentStates[i] = flash ? LampState{35, 255, 255} : LampState{0, 0, 255};
            }
            break;
        }
        case EFFECT_VOLCANIC_RAGE: {
            for (int i = 0; i < LampCount; i++) {
                uint8_t hue = (uint8_t)random(0, 25);
                uint8_t val = (uint8_t)random(150, 255);
                currentStates[i] = {hue, 255, val};
            }
            break;
        }
        case EFFECT_ICE_COLD: {
            for (int i = 0; i < LampCount; i++) {
                uint8_t hue = (uint8_t)random(140, 170);
                uint8_t sat = (random(5) > 2) ? 255 : 50;
                currentStates[i] = {hue, sat, 255};
            }
            break;
        }
        case EFFECT_PURPLE_DREAM: {
            float breathe = (sin(now / 1000.0) + 1.0) / 2.0;
            uint8_t hue = 190 + (uint8_t)(breathe * 40);
            uint8_t val = 100 + (uint8_t)(breathe * 155);
            for (int i = 0; i < LampCount; i++) {
                currentStates[i] = {hue, 255, val};
            }
            break;
        }
        case EFFECT_SUNSET_FADE: {
            uint8_t hue = (uint8_t)(10 + sin(now / 5000.0) * 15);
            for (int i = 0; i < LampCount; i++) {
                currentStates[i] = {hue, 220, 200};
            }
            break;
        }
        case EFFECT_RAVE: {
            uint8_t hue = (uint8_t)(now % 256);
            uint8_t val = (effectStep % 2 == 0) ? 255 : 0;
            for (int i = 0; i < LampCount; i++) {
                currentStates[i] = {hue, 255, val};
            }
            break;
        }
        case EFFECT_NONE:
        default:
            break;
    }

    applyAllStates();
    effectStep++;
    lastEffectUpdate = now;
    unlock();
}

void LampManager::applyStateToLamp(int index, const LampState& state) {
    if (state.v == 0) {
        control->turnOff(lampAddresses[index]);
    } else {
        // Use turnOnWithColor to avoid white flash
        control->turnOnWithColor(lampAddresses[index], state.h, state.s, state.v);
    }
}

bool LampManager::validLampIndex(int index) const {
    return index >= 0 && index < LampCount;
}
