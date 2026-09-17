# Home Controller Architecture Cleanup Plan

## Executive Summary

You're not facing a mysterious bug - you're dealing with a project that contains several partly completed architectural migrations. This plan provides a systematic approach to finish these migrations, bringing your codebase to the clean, intended architecture you originally designed.

## Current Architecture Problems

### Problem A: Hardware Access is Leaking Upward
**Current State:**
```
ScriptManager → HardwareManager
SceneManager  → HardwareManager
WebUI         → ScriptManager → HardwareManager
Core          → HardwareManager
```

**Target State (from issues.md):**
```
Core → HardwareManager
```
Higher-level components should go through the application command path instead of directly accessing hardware.

### Problem B: SceneManager and ScriptManager Overlap
**Current State:**
```
SceneManager     ├── scene storage
                ├── KAKU lookup
                ├── action execution
                └── hardware control

ScriptManager    ├── script storage
                ├── trigger matching
                ├── KAKU→scene
                ├── action execution
                └── hardware control
```

**Target State:**
```
SceneManager     ├── scene storage
                ├── trigger → scene lookup
                └── request script/action

ScriptManager    ├── script storage
                ├── trigger matching
                └── execute actions
```

### Problem C: Translator is One-Directional
**Current State:**
```
IR → VC
```

**Target State:**
```
IR → VC (currently implemented)
VC → hardware (needs implementation)
```

### Problem D: WebUI Contains Hardware Data
**Current State:**
```
action_scripts.html     └── IR codes / remote definitions
```

**Target State:**
```
/data/ir_db/    ← authoritative source of IR data
WebUI           ← queries ESP32 for available data
```

### Problem E: Command Representation is Inconsistent
**Current State:**
- JSON action objects: `{"type": "ir", ...}`
- Script command strings: `living 1 rgb 255 100 20`, `rgbstrip ...`, `ir ...`

**Target State:**
Unified command representation system

## Cleanup Phases

### Phase 1: Establish One Execution Path
**Goal:** Create the canonical execution path before making any other changes

```
Trigger → Scene/Script logic → Application command → Core → HardwareManager
```

**Key Tasks:**
1. Identify and document the actual execution paths in the current codebase
2. Establish a single, clear path from trigger to hardware execution
3. Eliminate direct hardware access from non-Core components

**Focus:** This is the foundation - everything else depends on getting this right.

### Phase 2: Make SceneManager Small
**Goal:** Reduce SceneManager to its core responsibilities

**New Responsibilities:**
- scenes.json storage
- "which scene?" lookup
- "which script/action?" delegation

**Remove:**
- Hardware calls
- Second command engine
- Hardware action parsing

**Alignment:** Directly addresses ISSUE-010.

### Phase 3: Make ScriptManager the Action Engine
**Goal:** Define ScriptManager's proper role in the architecture

**New Responsibilities:**
- scripts.json storage
- trigger matching
- action sequencing

**Remove:**
- Physical hardware access
- Direct command execution

**Result:** All action execution goes through the common command path.

### Phase 4: Make IR Data Authoritative
**Goal:** Centralize IR data management

**New State:**
- JSON files contain: `remote`, `button`, `code`, `VC_*`
- No copies in JavaScript
- No copies in random C++
- WebUI queries ESP32 for available data

**Addresses:** Issues 004, 005, and 013 together.

### Phase 5: Finish IR Output
**Goal:** Complete the IR command flow

**New Flow:**
```
remote = TV
button = POWER
    ↓ IR DB lookup
    ↓ IRCommand
    ↓ Core
    ↓ HardwareManager
    ↓ IRController::send()
```

**Satisfies:** The intent of ISSUE-007.

### Phase 6: Finish RGB-Strip Output
**Goal:** Implement complete RGB-strip command handling

**New Flow:**
```
rgbstrip RGB_24KEY-R1 RED
    ↓ remote lookup
    ↓ nominal/virtual color
    ↓ correct button/code for THAT remote
    ↓ IRCommand
    ↓ IR hardware
```

**Addresses:** ISSUE-006.

### Phase 7: Revisit the Color Model
**Goal:** Finalize the color system architecture

**Key Principle:**
```
VC_RED
```
Should not secretly mean an IR code or #FF0000. It is a semantic color identity. The actual output representation depends on the target hardware.

**Components to Consider:**
- HelperRGB
- VC_*
- hex codes
- IR codes
- LivingColors RGB
- NeoPixel RGB

## Simplified Mental Model

Instead of thinking of your project as "25 classes," think of it as these six boxes:

```
┌───────────────┐
│  Hardware     │
│               │
│ IR  RF  KAKU  │
│ Living  Audio │
└───────┬───────┘
        │
        ▼
┌───────────────┐
│ Hardware      │
│ Manager       │
└───────┬───────┘
        │ events
        ▼
┌───────────────┐
│ Event /       │
│ Translation   │
│               │
│ IR → VC       │
└───────┬───────┘
        │
        ▼
┌────────────────────┐
│ Scenes / Scripts   │
│                    │
│ "what should happen?"
└─────────┬──────────┘
          │
          ▼
┌────────────────────┐
│ Command / Core     │
│                    │
│ "make it happen"   │
└─────────┬──────────┘
          │
          ▼
┌────────────────────┐
│ HardwareManager    │
│                    │
│ "do the physical   │
│  operation"        │
└────────────────────┘
```

**WebUI** sits off to the side:
```
WebUI                    │
               HTTP / JSON
                    │
                    ▼
             Scenes / Scripts
```

NOT:
```
WebUI → HardwareManager
```

## Next Concrete Step

Before changing any code, create a precise "CURRENT → TARGET" call graph for these five critical paths:

1. **KAKU → Scene → Script → LivingColors**
2. **IR → Translator → VC**
3. **WebUI → action/script test**
4. **IR command → IR transmitter**
5. **RGB-strip command → IR transmitter**

Each arrow should be based on actual source code, not assumptions.

## Why This Will Work

You're not redesigning the whole project - you're finishing the migration it has already started. The evidence is unusually clear:

- issues.md explicitly lists the migration problems
- HardwareManager is already a sensible hardware boundary
- Translator already provides a working IR → VC mechanism
- SceneManager and ScriptManager currently overlap
- WebUI contains duplicated IR knowledge
- rgbstrip is explicitly unfinished
- The repository's documented target architecture points toward Translator → Core → HardwareManager

## Expected Outcomes

After completing this plan:

1. **Clean Architecture:** Clear separation of concerns between components
2. **Maintainable Code:** Each component has a well-defined responsibility
3. **Consistent Commands:** Unified command representation system
4. **Centralized Data:** Single source of truth for IR data
5. **Complete Feature Set:** All intended features (including RGB strips) work end-to-end
6. **Foundation for Growth:** Architecture supports future expansion
