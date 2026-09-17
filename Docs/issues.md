# ESP32 Home Controller — Issues

## Active

### ISSUE-001 — Duplicate LittleFS initialization
**Priority:** High

LittleFS appears to be initialized/mounted in multiple components.

**Goal:** Core owns filesystem initialization; other components only access the mounted filesystem.

**Acceptance criteria**
- Only one application `LittleFS.begin()`.
- Translator reads `/ir_db/`.
- ScriptManager reads/writes `/scripts.json`.
- SceneManager reads/writes `/scenes.json`.
- WebUI serves files normally.
- Boot remains stable.

### ISSUE-002 — Script commands JSON corruption
**Priority:** High

Saved commands can become:

```text
[]["living 2 rgb 255 100 20","rgbstrip RGB_44KEY-R1 #00FF00"]
```

Expected:

```json
[
  "living 2 rgb 255 100 20",
  "rgbstrip RGB_44KEY-R1 #00FF00"
]
```

**Acceptance criteria**
- New and updated scripts contain valid command arrays.
- Commands reload correctly after reboot.
- `runScript(ID)` reports the correct command count.

### ISSUE-003 — Temporary editor Test calls `runScript()` incorrectly
**Priority:** High

The editor currently causes a call resembling:

```text
runScript(name): {"name":"__EDITOR_TEST__","aliases":[],"commands":[...]}
```

The entire JSON object is being treated as the script name.

**Acceptance criteria**
- Test does not create a permanent script.
- Test executes the selected commands through CommandSink/Core.
- WebUI does not access HardwareManager directly.

### ISSUE-004 — Hardcoded `fallbackRemotes` in action_scripts.html
**Priority:** High

The editor contains a duplicate JavaScript database of remotes/colors even though IR JSON files and Translator are intended to be the source of truth.

**Acceptance criteria**
- Remove `fallbackRemotes`.
- No hardcoded remote/color list in the editor.
- Editor obtains available data from ESP32/Translator.
- Adding a new IR JSON automatically makes it available.

### ISSUE-005 — Translator does not expose all IR buttons
**Priority:** Medium

Current Translator loading only adds mappings when `virtual_color` is non-empty. Normal buttons such as `POWER` and `PLAY_PAUSE` therefore do not use the same mapping structure.

**Acceptance criteria**
- Define a clean representation for normal IR buttons.
- Support `ir TV POWER`.
- Preserve color/nominal-color support.

### ISSUE-006 — `rgbstrip` command is not implemented end-to-end
**Priority:** High

The editor generates commands such as:

```text
rgbstrip RGB_44KEY-R1 #00FF00
```

but the complete Core → IR → HardwareManager path is not finished.

**Acceptance criteria**
- Resolve remote from IR database.
- Resolve nominal color to the correct button/code.
- Transmit the actual IR code.
- Different remotes may use different codes for the same nominal color.

### ISSUE-007 — `ir TV POWER` currently fails in Core
**Priority:** High

Current behavior reaches Core but returns failure.

**Acceptance criteria**
- Resolve remote/button.
- Transmit through HardwareManager.
- Script reports success when transmission succeeds.

### ISSUE-008 — `/api/set-debug-level` is requested but not found ✅ RESOLVED
**Priority:** Low

The log currently shows requests to:

```text
/api/set-debug-level
```

which return NOTFOUND.

**Resolution:** Implemented both POST and GET endpoints in WebUI.cpp:

1. `/api/set-debug-level` (POST): Accepts a JSON body with a `level` field (1-5) and calls `Debug::setDebugLevel()` to update the debug level at runtime.

2. `/api/debug-level` (GET): Returns the current debug level in JSON format.

**Acceptance criteria**
- Either implement it using the existing Debug system, or remove the frontend request if obsolete.

### ISSUE-009 — WebUI contains obsolete/duplicate code
**Priority:** Medium

WebUI has accumulated old routes and APIs during the architecture changes.

**Acceptance criteria**
- Remove unsupported calls and obsolete routes.
- Do not add methods to ScriptManager/SceneManager merely to satisfy old WebUI code.
- Keep WebUI as the HTTP/UI layer.

### ISSUE-010 — SceneManager architecture cleanup
**Priority:** Medium

SceneManager should not duplicate ScriptManager's macro engine.

**Target responsibilities**
- Scene definitions.
- Static/animated scene state.
- Kaku scene activation.
- WebUI scene activation.
- Calling ScriptManager when a scene references a script.

**Acceptance criteria**
- No HardwareManager access.
- No duplicated script engine.
- Scenes remain in `/scenes.json`.

### ISSUE-011 — Audio/automation ownership
**Priority:** Medium

Audio beat detection and automation need to use the Core command path.

**Acceptance criteria**
- Audio provides beat/timing information.
- Animated scene speed/timing can use beat information.
- Hardware actions still go through Core.

### ISSUE-012 — Direct HardwareManager access outside Core
**Priority:** High

Core should be the only higher-level component that knows about and accesses HardwareManager.

**Acceptance criteria**
- Search the entire project for HardwareManager references.
- Remove unnecessary higher-level access.

### ISSUE-013 — Hardcoded IR data outside `/data/ir_db/`
**Priority:** Medium

IR codes, remote definitions, and color mappings should not be duplicated in HTML or application code.

**Acceptance criteria**
- IR JSON is the source of truth.
- No duplicated IR codes in WebUI/editor.
- No duplicated remote/color tables in JavaScript.

## Closed / verified

### ISSUE-014 — Runtime filesystem preservation
**Status:** Closed

The Python filesystem update process now preserves:

```text
/scripts.json
/scenes.json
```

`backup/` and `config/` are no longer used.

Verified after rebuild/upload:
- `scripts.json` survives.
- `scenes.json` survives.

### ISSUE-015 — WebUI raw JSON POST body
**Status:** Closed

WebUI raw JSON POST handling was corrected.

Verified by:

```text
[WEBUI][SCRIPTS] Save request received
[WEBUI][SCRIPTS] Save successful
```

### ISSUE-016 — LivingColors canonical command path
**Status:** Closed

Working path:

```text
ScriptManager
    ↓
CommandSink
    ↓
Core
    ↓
HardwareManager
    ↓
LivingColors
```

Verified commands include:

```text
living 1 rgb 255 100 20
living 2 rgb 255 100 20
```

## Architecture target

```text
WEB / KAKU / IR / AUDIO
          ↓
       Translator
          ↓
         Core
       /  |    Scenes Scripts Effects
       \  |  /
      Core commands
          ↓
   HardwareManager
```

### Ownership rules

- Core owns hardware access.
- HardwareManager owns physical devices.
- Translator translates external/device representations into canonical forms.
- ScriptManager owns scripts/macros.
- SceneManager owns scenes and scene activation.
- WebUI owns HTTP/UI interaction.
- LittleFS is initialized once.
- `/scripts.json` is the runtime script store.
- `/scenes.json` is the runtime scene store.
- `/data/ir_db/*.json` is the IR database source of truth.
