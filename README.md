# ⚡ MidiFlux: Modular Generative MIDI Effect Rack (VST3 / CLAP / Standalone)

**MidiFlux** is a professional, modular, re-orderable MIDI transformation rack for Windows, developed in modern C++20 using the **JUCE Framework** and **CLAP extensions**. It takes incoming MIDI streams and routes them through an arbitrary, user-customizable pipeline of generative, harmonic, timing, and filtering effect blocks.

![C++20](https://img.shields.io/badge/Language-C%2B%2B20-blue.svg)
![JUCE 8](https://img.shields.io/badge/Framework-JUCE_8-orange.svg)
![Format](https://img.shields.io/badge/Format-VST3_%7C_CLAP_%7C_Standalone-green.svg)
![Platform](https://img.shields.io/badge/Platform-Windows_x64-lightgrey.svg)

---

## 🎹 Built Formats & Binary Locations

After building, the binaries are located in:
- **CLAP Plugin**: `build/MidiFlux_artefacts/Release/CLAP/MidiFlux.clap`
- **VST3 Plugin**: `build/MidiFlux_artefacts/Release/VST3/MidiFlux.vst3/Contents/x86_64-win/MidiFlux.vst3`
- **Standalone App**: `build/MidiFlux_artefacts/Release/Standalone/MidiFlux.exe`

### Installing into your DAW:
- **VST3**: Copy `MidiFlux.vst3` to your system VST3 directory (typically `C:\Program Files\Common Files\VST3\`).
- **CLAP**: Copy `MidiFlux.clap` to your system CLAP directory (typically `C:\Program Files\Common Files\CLAP\`).
- **Standalone**: Launch `MidiFlux.exe` directly to test and play without needing a DAW open.

---

## 🎛️ The 14 Modular Effect Blocks

MidiFlux offers 14 dedicated effect blocks that can be added in any quantity, edited, bypassed, re-ordered, and individually randomized:

### 1. Generative & Stochastic Modules (Cyan / Emerald / Sky Blue / Teal / Coral)
* **Probability** (`#00E5FF` Neon Cyan):
  * **Gate Prob**: 0–100% chance an incoming note triggers.
  * **Min / Max Vel**: Restricts or remaps velocities.
  * **Vel Random**: Blends random velocity variations with input dynamics.
  * **Ghost Chance**: Stochastic micro-ghost note injector.
  * **Condition Mode**: Cycle-based trig conditions (*Always*, *1 of 2*, *2 of 4*, *1 of 4*, *3 of 4*).
* **Mutator** (`#10B981` Emerald):
  * **Mutate Pitch**: Stochastic pitch mutation probability.
  * **Pitch Range**: Pitch shift window (1 to 12 semitones).
  * **Octave Jump**: Chance of jumping ±1 octave.
  * **Rhythm Slip**: Micro-delay slip probability and amount in milliseconds.
  * **Snap to Scale**: Forces all mutated pitches to remain musically in-scale.
* **Arpeggiator** (`#38BDF8` Sky Blue):
  * **Modes**: *Up*, *Down*, *Up/Down*, *Converge*, *Diverge*, *Random*, *Brownian Walk*, and *Chord Triggers*.
  * **Rate**: Host tempo-synced (1/4, 1/8, 1/16, 1/32, 1/8T, 1/16T, 1/8D, 1/16D).
  * **Octaves**: 1 to 4 octave range expansion.
  * **Gate & Swing**: Note gate length (10%–150%) and swing timing (0%–75%).
  * **Euclidean Hits & Steps**: Algorithmic Euclidean pulse masking.
  * **Skip Prob & Mutate**: Generative pauses and stochastic step mutations.
* **Ratchet & Roll** (`#F43F5E` Coral):
  * **Roll Chance**: Probability a note bursts into rapid sub-repeats.
  * **Repeats**: 2x, 3x, 4x, 6x, 8x, or random burst count.
  * **Speed**: Burst rate (1/16, 1/32, 1/64).
  * **Dynamics**: Velocity decay or crescendo across the roll.
* **Euclidean Rhythms** (`#14B8A6` Teal):
  * Distributes $k$ pulses across $n$ steps with phase rotation.
  * Rhythmic gating of held chords or melodies.

### 2. Harmonic & Pitch Modules (Electric Purple / Violet / Indigo)
* **Chord Generator** (`#A855F7` Electric Purple):
  * **Chord Types**: *Diatonic Auto* (follows current scale degrees), *Major*, *Minor*, *Dominant 7*, *Major 7*, *Minor 7*, *Sus2*, *Sus4*, *Diminished*, *9th*, *Power Chord*.
  * **Inversion**: Root, 1st, 2nd, 3rd, or Random Inversion.
  * **Voicings**: Close, Drop-2, Drop-3, Spread Open.
  * **Strum Engine**: Strum speed (0–100 ms), direction (*Up*, *Down*, *Alternate*, *Random*), and velocity ramp with delayed queue across buffers.
  * **Drop Note**: Stochastic drop of inner chord voices for sparse voicings.
* **Scale Quantizer** (`#6366F1` Indigo):
  * Dedicated harmonic scale snapping engine.
  * **Snap Amount**: 0% leaves notes completely untouched/unquantized; 100% hard snaps every note into the key.
  * **Global Sync**: Automatically follows the top banner Root Key & Scale or uses custom settings.
  * **19 Scales**: Major, Minor, Dorian, Phrygian, Lydian, Mixolydian, Locrian, Pentatonic, Blues, Diminished, Arabic, Hirajoshi, etc.
  * **Direction**: *Closest*, *Upward*, *Downward*.
  * **Degree Shift**: Diatonic step transposition within key (-7 to +7 degrees).
* **Harmonizer** (`#9333EA` Deep Purple):
  * Multi-voice diatonic harmonizer adding parallel 3rds, 5ths, 6ths, or octaves in key.
* **Transpose** (`#8B5CF6` Violet):
  * Chromatic semitones (-36 to +36), Diatonic scale degrees (-14 to +14), Octaves (-3 to +3).
  * Stochastic random jump chance (jumping by octaves, fifths, or thirds).

### 3. Timing & Groove Modules (Amber / Cyan / Gold)
* **Time Quantizer** (`#06B6D4` Cyan):
  * Dedicated sample-accurate rhythmic snapping engine with delayed event queue.
  * **Expanded Grid**: 13 divisions (*1/4*, *1/4T*, *1/4D*, *1/8*, *1/8T*, *1/8D*, *1/16*, *1/16T*, *1/16D*, *1/32*, *1/32T*, *1/32D*, *1/64*).
  * **Strength**: 0% (Off/unquantized) to 100% (hard lock to grid).
  * **Swing**: 0% to 75% groove swing.
  * **Tolerance Window**: *Full Grid*, *Tight (25ms)*, *Medium (50ms)*, *Loose (100ms)*.
* **Humanizer** (`#F59E0B` Amber Glow):
  * Pronounced organic microtiming & dynamics engine.
  * **Time Jitter**: 0 to 100 ms microtiming variance.
  * **Push / Pull**: -50 ms (rushing ahead) to +50 ms (dragging behind).
  * **Vel Jitter**: 0 to 60 dynamics variance.
  * **Gate Jitter**: 0% to 100% note duration variance.
  * **Groove Feel**: *Natural*, *Loose / Drunk*, *Laid-Back (Drag)*, *Rushing (Push)*.
* **MIDI Delay & Cascade** (`#D97706` Gold Amber):
  * Host-synced feedback repeats (1 to 8 echoes) with velocity decay.
  * Pitch shift per tap (e.g. +7 semitones for rising fifth cascades, -12 for octave drops) with scale snap.

### 4. Utility & Modulation Modules (Slate / Blue Grey / Magenta / Electric Violet)
* **Filter & Split** (`#64748B` Slate Blue):
  * MIDI Channel filter (All, 1..16).
  * Note pitch range cutoff (for keyboard splitting).
  * Velocity threshold window.
  * CC and Pitch Bend pass-through toggles.
* **Mapper** (`#0EA5E9` Blue Grey):
  * Velocity curves (*Linear*, *Compress*, *Expand*, *Logarithmic*, *Exponential*, *Invert*, *Random*).
  * Velocity min/max clamps.
  * CC Remapper (e.g., CC 1 Mod Wheel -> CC 74 Filter Cutoff).
* **Invert & Mirror** (`#EC4899` Magenta):
  * Dedicated pitch and velocity transform module.
  * **Pivot Note**: Center note for musical inversion (default C4 = 60).
  * **Pitch Invert**: Inverts melody intervals upside down around pivot.
  * **Scale Snap**: Forces inverted melody to snap musically to active scale.
  * **Invert Velocity**: Flips dynamics ($v \to 128 - v$, soft notes become loud and vice versa).
  * **Octave Wrap**: Constrains wide inversions to within $\pm 1$, $\pm 2$, or $\pm 3$ octaves.
* **MIDI LFO** (`#D946EF` Electric Violet):
  * Dedicated high-resolution continuous MIDI CC and Pitch Bend modulator.
  * **Target**: *CC 1 (Mod Wheel)*, *CC 11 (Expression)*, *CC 74 (Filter Cutoff)*, *CC 71 (Resonance)*, *CC 10 (Pan)*, *CC 7 (Volume)*, or *Pitch Bend*.
  * **Sync Mode**: *Tempo Synced* vs *Free Rate (Hz)* (0.05 Hz to 20.0 Hz).
  * **Tempo Rates**: 11 musical divisions (*4 Bars*, *2 Bars*, *1 Bar*, *1/2*, *1/4*, *1/8*, *1/16*, *1/8T*, *1/16T*, *1/8D*, *1/16D*).
  * **Shapes**: *Sine*, *Triangle*, *Saw Up*, *Saw Down*, *Square*, *Random Steps (S&H)*, *Smooth Random Walk*.
  * **Depth & Center Offset**: Full control over modulation amplitude and center point.
  * **Retrigger**: *Free Run* or *Retrigger on Note-On*.

---

## 🖥️ User Interface & Experience

- **Re-orderable Rack View**:
  - **Full Drag & Drop**: Click and drag any module card by its header or grip handle (`⋮⋮`) to easily drop and reorder modules anywhere in the rack with a visual insertion slot indicator.
  - Optional **◀** and **▶** buttons for single-click nudging.
  - **Power Switch (ON/OFF)**: High-contrast toggle badge to bypass individual blocks.
  - **Dice Button (🎲)**: Instant parameter randomization for serendipitous sound design.
  - **Live LED Activity**: Real-time glowing indicator on each card flashing whenever notes pass through or are generated.
  - **+ Add Effect Card**: Clicking anywhere brings up a categorized popup menu of all 14 blocks.
- **Top Header Utility Bar**:
  - **Custom Vector Logo**: Clean vector lightning bolt logo with crisp modern typography.
  - **Undo / Redo ("UNDO" / "REDO")**: Full undo and redo history for rack edits, plus keyboard shortcuts (`Ctrl+Z`, `Ctrl+Y` / `Ctrl+Shift+Z`).
  - **Save & Load User Presets ("SAVE" / "LOAD")**: Export and import your custom modular chains as `.midiflux` files.
  - **Keyboard Toggle ("KBD")**: Easily toggle the on-screen keyboard to give the modular rack maximum vertical screen real estate.
  - **Global Scale & Key Selector**: Sets root note and scale across all scale-aware modules (with 19 verified musical scales & modes).
  - **Preset Selector**: Factory presets (*Generative Ambient Arp*, *Neo-Soul Strummer*, *Glitch Mutator & Ratchet*, *Acid Cyber-Arp*, *Euclidean Polyrhythms*).
  - **Randomize All ("DICE")**: Re-rolls parameters across the entire rack simultaneously.
  - **ALL OFF ("PANIC")**: Instantly silences and clears all note-ons across all 16 MIDI channels.
  - **Master Bypass ("BYPASS")**: Toggles the complete rack on and off.
- **MIDI Event Monitor**:
  - Real-time dual-rail display showing input notes (Cyan) vs transformed output notes (Pink/Purple) across the 128-note MIDI pitch spectrum with glow decay.
- **Playable Virtual MIDI Keyboard**:
  - Integrated 3-octave keyboard at the bottom of the window with show/hide toggle.
  - **Vertical Touch Velocity**: Press near the top for soft/piano (~35 velocity) and near the bottom for loud/forte (up to 127 velocity).
  - Click & drag with mouse or play using your PC QWERTY keyboard (`A`, `W`, `S`, `E`, `D`, `F`, `T`, `G`...).
  - Active keys illuminate with neon glow upon note trigger.

---

## 🛠️ Building From Source

### Requirements:
- Windows 10 / 11 (64-bit)
- CMake 3.22+
- Visual Studio 2022 (with MSVC C++ x64 tools)
- Git

### Build Commands:
```powershell
# 1. Clone repository
git clone <repo-url> F:\VSTdev
cd F:\VSTdev

# 2. Configure with Visual Studio 2022
cmake -B build -G "Visual Studio 17 2022" -A x64

# 3. Compile Release build
cmake --build build --config Release --parallel

# 4. Run automated test suite
.\build\Release\MidiFluxTests.exe
```

---

## 📄 License
MIT License. Built with the JUCE 8 Framework and clap-juce-extensions.
