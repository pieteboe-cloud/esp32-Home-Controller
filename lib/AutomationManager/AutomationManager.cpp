#include "AutomationManager.h"
#include "ControlLivingColors.h"


void AutomationSequence::execute(RF433& rf, IRController& ir, ControlLivingColors* lampControl, uint8_t (*addresses)[9]) {
    Debug::print("[AUTOMATION] Executing sequence: ");
    Debug::println(name.c_str());

    for (const auto& action : actions) {
        switch (action.type) {
            case ACTION_RF_SEND:
                Debug::print("[AUTOMATION] RF Send: ");
                Debug::print(action.value);
                Debug::print(" (");
                Debug::print(action.bitLength);
                Debug::print("bit, Protocol: ");
                Debug::print(action.protocol);
                Debug::println(")");
                rf.send(action.value, action.bitLength);
                break;

            case ACTION_IR_SEND:
                Debug::print("[AUTOMATION] IR Send: 0x");
                Debug::println(String(action.value, HEX));
                ir.send(action.value, action.bitLength);
                break;

            case ACTION_DELAY:
                Debug::print("[AUTOMATION] Delay: ");
                Debug::print(action.value);
                Debug::println("ms");
                delay(action.value);
                break;

            case ACTION_RGB_SET:
                Debug::print("[AUTOMATION] RGB Set: R=");
                Debug::print(action.rgbRed);
                Debug::print(" G=");
                Debug::print(action.rgbGreen);
                Debug::print(" B=");
                Debug::println(action.rgbBlue);
                
                // Bridge to LivingColors: We treat R/G/B fields as H/S/V for these lamps
                if (lampControl && addresses) {
                    for (int i = 0; i < 11; i++) {
                        // For original 2006 LivingColors, use turnOnWithColor to avoid white flash
                        lampControl->turnOnWithColor(addresses[i], (uint8_t)action.rgbRed, (uint8_t)action.rgbGreen, (uint8_t)action.rgbBlue);
                        // Tiny delay to let radio settle between commands
                        delayMicroseconds(100);
                    }
                }
                break;
        }
    }

    Debug::println("[AUTOMATION] Sequence complete");
    
}

void AutomationManager::initDefaultScenes() {
    Debug::println("[AUTOMATION] Initializing default scenes...");

    // ===== CALM SCENES =====
    // Scene 1: "Movie Night" - Dim red lighting
    AutomationSequence* movieNight = createSequence("movie_night");
    movieNight->addAction(ACTION_RGB_SET, 0, 0, 0, 0, 200, 180); 

    // Scene 2: "Warm Sunset" - Golden warm tones
    AutomationSequence* warmSunset = createSequence("warm_sunset");
    warmSunset->addAction(ACTION_RGB_SET, 0, 0, 0, 30, 255, 200);

    // Scene 3: "Night Mode" - Deep blue, very dim
    AutomationSequence* nightMode = createSequence("night_mode");
    nightMode->addAction(ACTION_RGB_SET, 0, 0, 0, 240, 200, 80);

    // Scene 4: "Relaxation" - Soft cyan, medium brightness
    AutomationSequence* relaxation = createSequence("relaxation");
    relaxation->addAction(ACTION_RGB_SET, 0, 0, 0, 180, 100, 150);

    // ===== ALL OFF =====
    // Scene 5: "All Off"
    AutomationSequence* allOff = createSequence("all_off");
    allOff->addAction(ACTION_RF_SEND, 7654321, 24, 1);
    allOff->addAction(ACTION_RF_SEND, 7654322, 24, 1);
    allOff->addAction(ACTION_RGB_SET, 0, 0, 0, 0, 0, 0);

    // ===== STANDARD SCENES =====
    // Scene 6: "Warm White"
    AutomationSequence* warmWhite = createSequence("warm_white");
    warmWhite->addAction(ACTION_RGB_SET, 0, 0, 0, 25, 150, 255);

    // ===== WILD SCENES =====
    // Scene 7: "Disco Party" - Bright red, pulsing
    AutomationSequence* discoParty = createSequence("disco_party");
    discoParty->addAction(ACTION_RGB_SET, 0, 0, 0, 0, 255, 255);
    discoParty->addAction(ACTION_DELAY, 200, 0, 0);
    discoParty->addAction(ACTION_RGB_SET, 0, 0, 0, 0, 255, 0);
    discoParty->addAction(ACTION_DELAY, 200, 0, 0);

    // Scene 8: "Neon Madness" - Bright magenta at full blast
    AutomationSequence* neonMadness = createSequence("neon_madness");
    neonMadness->addAction(ACTION_RGB_SET, 0, 0, 0, 280, 255, 255);
    neonMadness->addAction(ACTION_DELAY, 150, 0, 0);
    neonMadness->addAction(ACTION_RGB_SET, 0, 0, 0, 120, 255, 255);
    neonMadness->addAction(ACTION_DELAY, 150, 0, 0);

    // Scene 9: "Psychedelic" - Cycling through vibrant colors
    AutomationSequence* psychedelic = createSequence("psychedelic");
    psychedelic->addAction(ACTION_RGB_SET, 0, 0, 0, 0, 255, 255);      // Red
    psychedelic->addAction(ACTION_DELAY, 100, 0, 0);
    psychedelic->addAction(ACTION_RGB_SET, 0, 0, 0, 60, 255, 255);     // Yellow
    psychedelic->addAction(ACTION_DELAY, 100, 0, 0);
    psychedelic->addAction(ACTION_RGB_SET, 0, 0, 0, 120, 255, 255);    // Green
    psychedelic->addAction(ACTION_DELAY, 100, 0, 0);
    psychedelic->addAction(ACTION_RGB_SET, 0, 0, 0, 240, 255, 255);    // Blue
    psychedelic->addAction(ACTION_DELAY, 100, 0, 0);

    // Scene 10: "Energy Burst" - Bright cyan and lime flashing
    AutomationSequence* energyBurst = createSequence("energy_burst");
    energyBurst->addAction(ACTION_RGB_SET, 0, 0, 0, 180, 255, 255);    // Cyan
    energyBurst->addAction(ACTION_DELAY, 80, 0, 0);
    energyBurst->addAction(ACTION_RGB_SET, 0, 0, 0, 90, 255, 255);     // Lime
    energyBurst->addAction(ACTION_DELAY, 80, 0, 0);

    // Scene 11: "Volcanic Rage" - Deep red to bright orange flashing
    AutomationSequence* volcanicRage = createSequence("volcanic_rage");
    volcanicRage->addAction(ACTION_RGB_SET, 0, 0, 0, 0, 255, 200);     // Deep red
    volcanicRage->addAction(ACTION_DELAY, 120, 0, 0);
    volcanicRage->addAction(ACTION_RGB_SET, 0, 0, 0, 15, 255, 255);    // Bright orange
    volcanicRage->addAction(ACTION_DELAY, 120, 0, 0);

    // Scene 12: "Ice Cold" - Bright blue and white alternating
    AutomationSequence* iceCold = createSequence("ice_cold");
    iceCold->addAction(ACTION_RGB_SET, 0, 0, 0, 240, 255, 255);        // Blue
    iceCold->addAction(ACTION_DELAY, 100, 0, 0);
    iceCold->addAction(ACTION_RGB_SET, 0, 0, 0, 0, 0, 255);            // White
    iceCold->addAction(ACTION_DELAY, 100, 0, 0);

    // Scene 13: "Purple Dream" - Deep purple pulsing
    AutomationSequence* purpleDream = createSequence("purple_dream");
    purpleDream->addAction(ACTION_RGB_SET, 0, 0, 0, 270, 200, 200);    // Soft purple
    purpleDream->addAction(ACTION_DELAY, 300, 0, 0);
    purpleDream->addAction(ACTION_RGB_SET, 0, 0, 0, 270, 200, 100);    // Dimmer purple
    purpleDream->addAction(ACTION_DELAY, 300, 0, 0);

    // Scene 14: "Sunset Fade" - Gradual orange to red
    AutomationSequence* sunsetFade = createSequence("sunset_fade");
    sunsetFade->addAction(ACTION_RGB_SET, 0, 0, 0, 30, 255, 200);      // Orange
    sunsetFade->addAction(ACTION_DELAY, 500, 0, 0);
    sunsetFade->addAction(ACTION_RGB_SET, 0, 0, 0, 10, 255, 150);      // Red-orange
    sunsetFade->addAction(ACTION_DELAY, 500, 0, 0);
    sunsetFade->addAction(ACTION_RGB_SET, 0, 0, 0, 0, 200, 100);       // Deep red
    sunsetFade->addAction(ACTION_DELAY, 500, 0, 0);

    // Scene 15: "Rave Mode" - Crazy multi-color flashing
    AutomationSequence* raveMode = createSequence("rave_mode");
    raveMode->addAction(ACTION_RGB_SET, 0, 0, 0, 0, 255, 255);         // Red
    raveMode->addAction(ACTION_DELAY, 50, 0, 0);
    raveMode->addAction(ACTION_RGB_SET, 0, 0, 0, 120, 255, 255);       // Green
    raveMode->addAction(ACTION_DELAY, 50, 0, 0);
    raveMode->addAction(ACTION_RGB_SET, 0, 0, 0, 240, 255, 255);       // Blue
    raveMode->addAction(ACTION_DELAY, 50, 0, 0);
    raveMode->addAction(ACTION_RGB_SET, 0, 0, 0, 180, 255, 255);       // Cyan
    raveMode->addAction(ACTION_DELAY, 50, 0, 0);
    raveMode->addAction(ACTION_RGB_SET, 0, 0, 0, 60, 255, 255);        // Yellow
    raveMode->addAction(ACTION_DELAY, 50, 0, 0);

    // Scene 16: "Groovy Galaxy" - Purple and blue with slow transitions
    AutomationSequence* groovyGalaxy = createSequence("groovy_galaxy");
    groovyGalaxy->addAction(ACTION_RGB_SET, 0, 0, 0, 240, 100, 255);   // Purple
    groovyGalaxy->addAction(ACTION_DELAY, 1000, 0, 0);
    groovyGalaxy->addAction(ACTION_RGB_SET, 0, 0, 0, 180, 100, 255);   // Lighter purple
    groovyGalaxy->addAction(ACTION_DELAY, 1000, 0, 0);
    groovyGalaxy->addAction(ACTION_RGB_SET, 0, 0, 0, 120, 100, 255);   // Blue-purple
    groovyGalaxy->addAction(ACTION_DELAY, 1000, 0, 0);

    // Scene 17: "Chase the Light" - Fast moving red and green
    AutomationSequence* chaseTheLight = createSequence("chase_the_light");
    chaseTheLight->addAction(ACTION_RGB_SET, 0, 0, 0, 0, 255, 0);      // Red
    chaseTheLight->addAction(ACTION_DELAY, 30, 0, 0);
    chaseTheLight->addAction(ACTION_RGB_SET, 0, 0, 0, 120, 255, 0);     // Green
    chaseTheLight->addAction(ACTION_DELAY, 30, 0, 0);
    chaseTheLight->addAction(ACTION_RGB_SET, 0, 0, 0, 0, 255, 0);      // Red
    chaseTheLight->addAction(ACTION_DELAY, 30, 0, 0);
    chaseTheLight->addAction(ACTION_RGB_SET, 0, 0, 0, 120, 255, 0);     // Green
    chaseTheLight->addAction(ACTION_DELAY, 30, 0, 0);

    // Scene 18: "Mystery Whisper" - Dark blue with purple accents
    AutomationSequence* mysteryWhisper = createSequence("mystery_whisper");
    mysteryWhisper->addAction(ACTION_RGB_SET, 0, 0, 0, 240, 50, 100);  // Dark blue
    mysteryWhisper->addAction(ACTION_DELAY, 800, 0, 0);
    mysteryWhisper->addAction(ACTION_RGB_SET, 0, 0, 0, 270, 100, 50);  // Purple
    mysteryWhisper->addAction(ACTION_DELAY, 800, 0, 0);
}

void AutomationManager::initDefaultTriggers() {
    Debug::println("[AUTOMATION] Initializing default RF triggers...");
    
    // Add your RF button mappings here
    // Example: addTrigger(0x123456, 24, 1, "movie_night");
}

AutomationSequence* AutomationManager::findSequence(String name) {
    for (auto& seq : sequences) {
        if (seq.getName() == name) {
            return &seq;
        }
    }
    return nullptr;
}

AutomationSequence* AutomationManager::createSequence(String name) {
    // Check if sequence already exists
    if (findSequence(name) != nullptr) {
        Debug::print("[Automation] Sequence already exists: ");
        Debug::println(name.c_str());
        return findSequence(name);
    }

    sequences.emplace_back(name);
    Debug::print("[Automation] Created new sequence: ");
    Debug::println(name.c_str());
    return &sequences.back();
}

void AutomationManager::addTrigger(unsigned long rfCode, int bitLength, int protocol, String sceneName) {
    triggers.push_back({rfCode, bitLength, protocol, sceneName});
    Debug::print("[Automation] Added trigger: RF=");
    Debug::print(rfCode);
    Debug::print(" -> Scene: ");
    Debug::println(sceneName.c_str());
}

bool AutomationManager::checkTrigger(unsigned long rfCode, int bitLength, int protocol) {
    for (const auto& trigger : triggers) {
        if (trigger.rfCode == rfCode &&
            trigger.bitLength == bitLength &&
            trigger.protocol == protocol) {
            Debug::print("[Automation] Trigger activated: ");
            Debug::println(trigger.sceneName.c_str());
            executeScene(trigger.sceneName);
            return true;
        }
    }
    return false;
}

bool AutomationManager::executeScene(String name) {
    AutomationSequence* seq = findSequence(name);
    if (seq != nullptr) {
        seq->execute(rfModule, irModule, lampController, lampAddresses);
        return true;
    }
    Debug::print("[Automation] Scene not found: ");
    Debug::println(name.c_str());
    return false;
}

std::vector<String> AutomationManager::getSceneNames() {
    std::vector<String> names;
    for (const auto& seq : sequences) {
        names.push_back(seq.getName());
    }
    return names;
}
