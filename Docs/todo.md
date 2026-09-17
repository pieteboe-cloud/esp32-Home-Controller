# ESP32 Home Controller — TODO

## Phase 1 — Foundation

- [ ] Fix LittleFS ownership: find all `LittleFS.begin()` calls and keep one application-level initialization, preferably owned by Core.
- [ ] Make Translator, ScriptManager, SceneManager, and WebUI use the already-mounted LittleFS without mounting it themselves.
- [ ] Remove leftover C++ references to obsolete `backup/` and `config/`.
- [ ] Clean up logging while keeping the existing `Debug::println()` system.
- [x] Fix or remove `/api/set-debug-level` if the UI still requests an endpoint that no longer exists. → Implemented both `/api/set-debug-level` POST endpoint and `/api/debug-level` GET endpoint in WebUI.cpp.

## Phase 2 — ScriptManager

- [ ] Fix `commands` JSON corruption. Current bad value resembles `[]["living 2 rgb 255 100 20","rgbstrip RGB_44KEY-R1 #00FF00"]`; it must be a normal JSON array.
- [ ] Verify new script save/update, aliases, commands, reload after reboot, and valid JSON.
- [ ] Fix temporary editor Test execution. It currently passes the entire temporary JSON object to `runScript(name)`.
- [ ] If needed, add one small ScriptManager method for executing an unsaved command list; do not add WebUI-specific or HardwareManager logic.
- [ ] Keep ScriptManager architecture: `ScriptManager → CommandSink → Core`.

## Phase 3 — Translator

- [ ] Make IR JSON the single source of truth.
- [ ] Remove hardcoded remote/color definitions from `action_scripts.html`, including `fallbackRemotes`.
- [ ] Finish `nominal_color` migration while preserving actual working IR codes.
- [ ] Expose loaded IR database information to WebUI/editor so the editor can populate remotes and colors dynamically.
- [ ] Decide how normal IR buttons such as `POWER` and `PLAY_PAUSE` are represented alongside color mappings.
- [ ] Support canonical commands such as `ir TV POWER`.

## Phase 4 — Core command handling

- [ ] Finish canonical `ir TV POWER` end-to-end.
- [ ] Finish `rgbstrip RGB_44KEY-R1 #00FF00` end-to-end.
- [ ] Resolve remote + nominal color to the actual IR button/code.
- [ ] Keep Core as the only component with HardwareManager access.

## Phase 5 — Script editor

- [ ] Remove `fallbackRemotes`.
- [ ] Populate remote dropdown dynamically from ESP32/Translator data.
- [ ] Populate colors dynamically using `nominal_color` and button name.
- [ ] Keep editor script-centric: name, aliases, ordered commands, add/delete/reorder, save, run, test.
- [ ] Keep scene/Kaku editor logic out of the script editor.

## Phase 6 — Scenes

- [ ] Clean SceneManager so it is not a copy of ScriptManager.
- [ ] Keep scene-specific state in SceneManager.
- [ ] Allow scenes to reference scripts by name/alias.
- [ ] Preserve static scenes.
- [ ] Add animated scene architecture without duplicating ScriptManager.
- [ ] Kaku activates scenes through Core → SceneManager.
- [ ] WebUI activates scenes through the same scene path.
- [ ] No direct HardwareManager access from SceneManager.

## Phase 7 — Audio / automation

- [ ] Route automation effects through Core.
- [ ] Use beat detection as timing/speed control for animated scenes/effects.
- [ ] Keep Audio from directly owning hardware actions.

## Phase 8 — Final cleanup and testing

- [ ] Search the entire project for duplicate LittleFS initialization.
- [ ] Search for obsolete `backup`, `config`, old action APIs, old scene APIs, obsolete capture APIs, and `fallbackRemotes`.
- [ ] Search for direct HardwareManager access outside Core.
- [ ] Search for hardcoded IR codes outside `/data/ir_db/`.
- [ ] Search for hardcoded remote/color definitions outside the IR JSON files.
- [ ] Remove dead and duplicate WebUI code.
- [ ] Test rebuild/upload of LittleFS and verify `scripts.json` and `scenes.json` survive.
- [ ] Full functional test: script creation, save, reload, run, LivingColors, RGB strip, IR transmit, Kaku → scene, scene → script, and audio/animation.

## Recommended order

1. LittleFS cleanup
2. ScriptManager `commands` JSON bug
3. Editor Test execution
4. Translator cleanup
5. RGB-strip + IR
6. Scene cleanup
7. Audio/automation
8. Final cleanup and full test
