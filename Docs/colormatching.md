# Color Matching System

## Overview
The color matching system is a fundamental part of the ESP32 Home Controller's unified lighting architecture. It provides a hardware-agnostic way to control colors across different types of lighting devices through the use of virtual colors.

---

## Color System Architecture

### 1. Virtual Colors (VC_) - The Universal Language
Virtual colors are semantic identifiers that represent color concepts rather than hardware-specific details.

#### Key Features:
- Used in scripts and scenes
- Can be renamed (e.g., `VC_RED` → `VC_SALON_TABLE_RED`)
- Hardware-agnostic
- Map to hex values (nominal colors)
- Work across all device types

#### Examples:
- `VC_RED`
- `VC_MAGENTA`
- `VC_TEAL`
- `VC_LIGHT_BLUE`
- `VC_DARK_PURPLE`
- `VC_SALON_TABLE_RED`
- `VC_TV_AMBIENT_BLUE`

### 2. Nominal Colors (Hex)
Every virtual color has an associated nominal hex value that represents the actual color.

#### Examples:
- `VC_MAGENTA` → `#FF00FF`
- `VC_TEAL` → `#008080`
- `VC_LIGHT_BLUE` → `#ADD8E6`
- `VC_DARK_PURPLE` → `#4B0082`

#### Hardware Usage:
- **LivingColors**: Hex values are converted to RGB
- **NeoPixel-style ICs**: Hex values are converted to RGB
- **RGB-IR Strips**: Hex values are converted to IR codes

### 3. Hardware Mapping
Different hardware types consume color data in different ways:

#### LivingColors (CC2500 RF)
- Takes RGB values
- Example: `living 3 rgb 138 43 226`

#### RGB-IR Strips
- Take IR codes, but the Translator maps:
- Example: `VC_MAGENTA` → `#FF00FF` → IR code `0xe916ef00`

#### NeoPixel-style ICs
- Take RGB values (same as LivingColors)
- Example: `neopixel 12 rgb 138 43 226`

---

## Data Sources

### 1. IR Database (data/ir_db/*.json)
Contains IR remote configurations with:
- Button name (physical label)
- IR code
- Virtual color (VC_*)
- Nominal color (hex)

#### Example:
```json
{
  "name": "MAGENTA",
  "code": "0xe916ef00",
  "virtual_color": "VC_MAGENTA",
  "nominal_color": "#FF00FF"
}
```

### 2. HelperRGB (HelperRGB/*.json)
Reference color tables with:
- Human-friendly names
- RGB values
- Hex values

#### Example:
```json
{
  "name": "Blue Violet",
  "rgb": [138, 43, 226],
  "hex": "#8A2BE2"
}
```

---

## How Color Matching Works

### The Translator Component
The Translator is the most important component, converting virtual colors to hardware-specific actions:

#### RGB-IR Strip Conversion:
```
VC_MAGENTA → #FF00FF → IR code 0xe916ef00
```

#### LivingColors Conversion:
```
VC_MAGENTA → #FF00FF → rgb(255,0,255) → CC2500 packet
```

#### NeoPixel-style IC Conversion:
```
VC_MAGENTA → #FF00FF → rgb(255,0,255) → WS2812 frame
```

### Typical Workflow:
1. A script or scene references a virtual color
2. The Translator looks up the virtual color's nominal hex value
3. The Translator converts the hex to the appropriate format for the target hardware
4. The hardware-specific command is executed

---

## Benefits of Virtual Colors

1. **Flexibility**: Colors can be renamed later without changing scripts
2. **Consistency**: The same color can be applied to multiple hardware types
3. **Hardware Agnosticism**: Scripts remain independent of specific hardware details
4. **Future-Proof**: New devices can be added without changing the script language
5. **Abstraction**: Avoids using hex codes, IR codes, or RGB values directly in scripts

---

## Usage Examples

### Script Example:
```
living 1 VC_SALON_TABLE_RED
rgbstrip RGB_24KEY-R1 VC_MAGENTA
neopixel 12 VC_TV_AMBIENT_BLUE
```

### Scene Example:
```
KAKU D-4 → Scene "Purple Madness" → Script "Purple Madness"
```

---

## WebUI Integration

### Script Editor:
- LivingColors section with RGB picker
- RGB-IR Strips section with remote selector and virtual color selector
- Commands generated: `living <lamp> rgb <r> <g> <b>` and `rgbstrip <remoteId> <virtual_color>`

### Scene Editor:
- Maps triggers (e.g., KAKU RF buttons) to scripts
- Example: Trigger: KAKU D-4 → Script: Purple Madness

---

## Future Expansion

The virtual color system allows for easy expansion with:
- New IR remotes
- New RGB strips
- New LivingColors lamps
- New NeoPixel chains
- New RF devices
- New scenes
- New scripts

All without changing the underlying script language.
