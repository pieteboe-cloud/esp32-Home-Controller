# ESP32 Home Controller – Architecture & Ownership Model

## Quick Reference

This document describes the core architecture and ownership rules for the ESP32 Home Controller firmware. Use this as a reference when adding features or debugging behavior.

---

## Ownership Model

Each major module owns a specific domain. **Do not mix responsibilities.**

| Module | Responsibility | Owns |
|--------|-----------------|------|
| **App** (`src/main.cpp`) | Entry point & thin shell | Startup banner, thin begin/update calls to Core |
| **Core** (`lib/Core/Core.h/cpp`) | Central orchestration | Startup sequence, event dispatch, lifecycle |
| **HardwareManager** | Physical I/O and devices | RF, IR, LivingColors, Kaku decoder, storage init |
| **StorageManager** | LittleFS persistence | File I/O, config/script JSON files |
| **WebUI** | HTTP API and dashboard | Routes, AP mode, script/scene CRUD endpoints |
| **ScriptManager** | Action automation | Script execution, trigger matching, script CRUD |
| **SceneManager** | Scene persistence | Scene loading, activation, scene CRUD |
| **Translator** | Event→Action mapping | Device events → scripts via trigger rules |
| **EventBus** | Message broker | All inter-module communication |
| **Debug** | Logging with severity | Level-gated console output, log buffer for web |

---

## Startup Sequence

1. **App::begin()** → calls Core::init()
2. **Core::init()** → orchestrates:
   - Debug::begin() + banner
   - HardwareManager::init() (RF, IR, storage)
   - SceneManager::begin() (loads scenes)
   - Translator::begin() (loads trigger rules)
   - WebUI::begin() (AP mode, web routes)
   - ScriptManager::begin() (loads scripts)
   - Emit "System -> ready" event
3. **App::update()** → calls Core::update()
4. **Core::update()** → polled every loop:
   - HardwareManager::update() (RF/IR decode)
   - EventBus dispatch
   - ScriptManager polling (time-based triggers)

---

## Event Flow

```
[Device Input]  (RF / IR / Kaku)
       ↓
[HardwareManager::handleXXXDecoded]
       ↓
[EventBus::publish(SystemEvent)]
       ↓
[Core::handleSystemEvent]
       ↓
[Translator::matchTriggers]
       ↓
[ScriptManager::executeScript]
       ↓
[HardwareManager callbacks]  (send IR/RF/control output)
```

---

## Debug Levels & Colors

All debug output uses `Debug::println(level, message)` with level-gating:

| Level | Color | Use Case |
|-------|-------|----------|
| **1** | Red | Errors, failures, critical issues |
| **2** | Yellow | Startup sequence, normal info, status |
| **3** | Blue | Detailed debugging, method tracing |
| **4** | Grey | Verbose internals, raw data dumps |

**Setting debug level:**
- Edit `platformio.ini`: `build_flags = -DDEBUG_LEVEL=2` (default is 2)
- Or at runtime: `Debug::setCurrentLevel(int)`

**Log pattern:**
```cpp
#if DEBUG_LEVEL >= 2
    Debug::println(2, "[MODULE][FUNCTION] Message text");
#endif
```

---

## Web API Structure

All API endpoints are in [lib/WebUI/WebUI.cpp](lib/WebUI/WebUI.cpp).

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/` | GET | Serve dashboard (index.html) |
| `/api/status` | GET | CPU load, heap, uptime (polled, no log) |
| `/api/logs` | GET | Fetch circular log buffer as HTML |
| `/api/logs/clear` | POST | Clear log buffer |
| `/api/action-scripts` | GET/POST/DELETE | Script CRUD |
| `/api/execute-script` | POST | Run script by ID immediately |
| `/api/test-action` | POST | Test single action (LED, IR, etc.) |
| `/api/set-time` | POST | Sync device clock with browser |

---

## File Structure

```
src/
  main.cpp              ← App entry point & shell

lib/
  Core/                 ← Central orchestrator
    Core.h/cpp
    EventBus.h          ← Pub/sub broker
  
  Debug/                ← Logging system
    Debug.h/cpp
  
  HardwareManager/      ← Physical I/O
    HardwareManager.h/cpp
    Config.h            ← Pin definitions
    Storage/
      StorageManager.h/cpp  ← LittleFS persistence
    IR/                 ← IR send/receive
    RF/                 ← RF send/receive
    RCSwitch/           ← RC-Switch protocol
    KakuDecoder/        ← Kaku protocol decoder
    LivingColors/       ← GE Enbrighten protocol
  
  WebUI/                ← HTTP API & dashboard
    WebUI.h/cpp
    ScriptManager.h/cpp ← Action automation
  
  SceneManager/         ← Scene persistence
    SceneManager.h/cpp
  
  Translator/           ← Event→Action mapping
    Translator.h/cpp

data/                   ← Web UI static files (uploaded via PlatformIO)
  index.html
  action_scripts.html
  ir_db/                ← IR code databases
  config/               ← Persisted JSON configs

unpacked_fs/            ← Local mirror of data/ (for development)
  config/
    devices.json
    mappings.json
    schedules.json
    settings.json
  scripts.json
  ir_db/
```

---

## Persistence

All JSON files live in LittleFS (mounted at `/`):
- `/config/settings.json` — global settings
- `/config/devices.json` — device definitions
- `/config/mappings.json` — device→GPIO mappings
- `/scripts.json` — action scripts
- `updatefs_preserve.py` — helper to keep config files during OTA uploads

---

## Key Design Decisions

1. **Event-Driven** — All device input goes through EventBus; no direct callbacks between modules.
2. **Single Debug Instance** — All logging is centralized; level-gated to reduce verbosity.
3. **Storage Independence** — Scripts and scenes persist in JSON; can be edited directly on device.
4. **Thin App Shell** — Main.cpp is a stub; all logic lives in Core and its delegates.
5. **Stateless Routes** — Web endpoints are stateless; state lives in managers and JSON files.

---

## Before Adding Features

✅ **Do:**
- Add new devices to HardwareManager with event callbacks.
- Add new scenes or scripts via WebUI or JSON editing.
- Create new EventBus event types as needed.
- Add debug lines at the appropriate level.
- Keep ownership boundaries clear.

❌ **Don't:**
- Bypass EventBus with direct module-to-module calls.
- Add business logic to App or WebUI routes.
- Mix device I/O and persistence concerns.
- Assume debug output will always be visible (respect DEBUG_LEVEL).
- Create new singletons without documenting them in Core.h.

---

## Troubleshooting

**"Module X is not initialized"**
- Check Core::init() startup order. Core owns the sequence.

**"Script is not executing"**
- Check Translator::matchTriggers() for condition matching.
- Check ScriptManager::executeScript() logs for action execution errors.
- Verify EventBus is publishing the expected event.

**"IR/RF command not decoded"**
- Check HardwareManager::update() is called in Core::update().
- Check Debug level ≥ 3 to see raw decode messages.
- Verify RF/IR hardware pins match Config.h.

**"Web API is slow or unresponsive"**
- Check CPU load via `/api/status`.
- Check RAM heap pressure — ScriptManager or EventBus may have memory leaks.
- Reduce DEBUG_LEVEL if logging is consuming CPU.

**"Filesystem full or corrupted"**
- Use `/api/logs` to monitor storage events.
- Check StorageManager debug output at level ≥ 2.
- May need to reupload filesystem via `platformio run --target uploadfs`.

---

## Environment & Build

- **Board:** ESP32 DOIT DEVKIT V1
- **Framework:** Arduino (PlatformIO)
- **Upload Port:** `/dev/ttyACM0` (configure in `platformio.ini`)
- **Build Command:** `platformio run`
- **Upload Command:** `platformio run --target upload --upload-port /dev/ttyACM0`
- **Preserve Config on Upload:** `python updatefs_preserve.py && platformio run --target uploadfs`

---

## Next Steps for Expansion

- **New Protocols** → Add to HardwareManager and emit SystemEvent.
- **New Automation Types** → Add to ScriptManager actions and SceneManager.
- **New Dashboard Features** → Add WebUI routes; data flows from managers.
- **New Storage Schema** → Version JSON files in StorageManager; document migration path.

---

Last updated: 2026-09-02  
Firmware built with: PlatformIO + Arduino framework
