# 🎛️ CrossMod: Polyphonic Cross-Modulation Synthesizer
### Developed by **mtyas** | VST3 • CLAP • AU • Standalone (Windows / macOS / Linux)

[![C++20](https://img.shields.io/badge/Language-C%2B%2B20-blue.svg)](https://isocpp.org/)
[![JUCE 8](https://img.shields.io/badge/Framework-JUCE_8-orange.svg)](https://juce.com/)
[![Format](https://img.shields.io/badge/Format-VST3_%7C_CLAP_%7C_Standalone-green.svg)](https://github.com/free-audio/clap)
[![Tests](https://img.shields.io/badge/Tests-18%2F18%20Passing-brightgreen.svg)]()
[![Latest Release](https://img.shields.io/github/v/release/mtyas/CrossMod?color=brightgreen&label=Release%20v1.2.0)](https://github.com/mtyas/CrossMod/releases/latest)
[![License](https://img.shields.io/badge/License-Proprietary-red.svg)]()

---

**CrossMod** is an analog-inspired, cross-modulation polyphonic synthesizer. Rather than relying on static wavetables or sample playback, CrossMod treats its oscillators and voice stages as dynamic, coupled physical and mathematical systems.

From lush vintage brass and silky pads to glassy FM bells, cybernetic leads, tearing through-zero distortion, and organic acoustic soundboard resonances, CrossMod offers unprecedented sonic depth paired with a vintage hardware skeuomorphic interface and an authentic phosphor-style **CRT Lissajous Phase Vectorscope**.

---

## 📸 Overview & Interface

![CrossMod Synthesizer](docs/images/crossmod_screenshot.png)

```
+----------------------------------------------------------------------------------------------------+
|  CROSSMOD                       [ PRESET: 04 - Inter-Voice Shimmer ]               [MIDI LEARN]   |
+----------------------------------------------------------------------------------------------------+
|  OSC 1 (PolyBLEP)    |  CROSS-MODULATION     |  OSC 2 (PolyBLEP)    |  SUB OSC (Direct Path)       |
|  - Shape: Saw/Pulse  |  - LinFM / PM / TZFM  |  - Shape: Saw/Pulse  |  - Shape: Sine/Tri/Saw/Pulse |
|  - Tune: 0 semi      |  - AM / RingMod       |  - Tune: +7 semi     |  - Octave: -1 / -2 Oct       |
|  - Glide: 15 ms      |  - Bidirectional 1<->2|  - Glide: 0 ms       |  - Level: -6 dB              |
+----------------------+-----------------------+----------------------+------------------------------+
|  ZERO-DELAY FILTER   |  FILTER ENVELOPE (ADSR)| AMPLIFIER ENV (ADSR)|  CRT VECTORSCOPE             |
|  - Type: 24dB Ladder |  - Attack: 5 ms       |  - Attack: 1 ms      |  - Mode: Phase Lissajous     |
|  - Cutoff / Reso     |  - Decay: 250 ms      |  - Decay: 350 ms     |  - Phosphor Glow Trail       |
|  - 1V/Oct Tracking   |  - Sustain: 40%       |  - Sustain: 80%      |  - 2.8x Analog Boost         |
|  - C^2 Soft Sat      |  - Release: 400 ms    |  - Release: 300 ms   |  - Real-Time Orbit Trace     |
+----------------------+-----------------------+----------------------+------------------------------+
|  STUDIO MODULATION   |  STUDIO DELAY (Hermite)| STUDIO REVERB (Plate)| MASTER & VOICING            |
|  - Chorus / Flanger  |  - Ping-Pong Stereo   |  - 4-Stage Algorith  |  - Poly / Mono / Unison      |
|  - Pre-FX / Post-FX  |  - Zero-Discontinuity |  - Decay / Damp / Mix|  - 7 Voice Topologies        |
|  - Rate / Depth / Mix|  - Flutter Modulation |  - Zero-Alloc Buffer |  - Pan / Master Volume       |
+----------------------------------------------------------------------------------------------------+
```

---

## 📦 Downloads & Pre-Built Binaries

Pre-compiled production releases for Windows, macOS, and Linux are available from the [**CrossMod Releases Page**](https://github.com/mtyas/CrossMod/releases/latest):

| OS / Platform | Download Package | Included Formats | Architecture |
| :--- | :--- | :--- | :--- |
| **Windows** | [📥 **CrossMod-v1.2.0-Windows.zip**](https://github.com/mtyas/CrossMod/releases/download/v1.2.0/CrossMod-v1.2.0-Windows.zip) | VST3, CLAP, Standalone (`CrossMod.exe`) | x86_64 |
| **macOS** | [📥 **CrossMod-v1.2.0-macOS-Universal.zip**](https://github.com/mtyas/CrossMod/releases/download/v1.2.0/CrossMod-v1.2.0-macOS-Universal.zip) | VST3, CLAP, AudioUnit (`.component`), Standalone (`CrossMod.app`) | Universal (Apple Silicon & Intel) |
| **Linux** | [📥 **CrossMod-v1.2.0-Linux-x64.zip**](https://github.com/mtyas/CrossMod/releases/download/v1.2.0/CrossMod-v1.2.0-Linux-x64.zip) | VST3, CLAP, Standalone | x86_64 |

---

## ✨ Key Features

### 1. Dual PolyBLEP Oscillators + Isolated 3rd Sub Oscillator
- **Oscillator 1 & 2**: Select between **Sine**, **Triangle**, **Saw**, and **Square** waveforms, all rendered with analytical bandlimited **PolyBLEP anti-aliasing** for pristine high-register response without aliasing harshness.
- **Independent Pitch & Glide**: Bipolar coarse tune ($\pm 36\text{ semitones}$), fine tune ($\pm 100\text{ cents}$), and **Dual Independent Glide** per oscillator.
- **3rd Sub Oscillator (Isolated Direct Path)**: Dedicated sub-oscillator with $-1\text{ Oct}$ or $-2\text{ Oct}$ pitch shift, 4 selectable waveforms, routed directly into the filter to guarantee tight, undistorted low-end foundations under extreme cross-modulation.

### 2. 5 Cross-Modulation Algorithms
- **Linear Frequency Modulation (Linear FM)**: Direct linear pitch-rate modulation with symmetrical harmonic sidebands.
- **Phase Modulation (PM)**: Pure analog phase modulation, delivering bright 80s metallic bells, glassy electric pianos, and modern digital timbres.
- **Through-Zero FM (TZFM)**: True four-quadrant through-zero frequency modulation. When modulation swings negative, phase cleanly reverses without pitch drift or instability.
- **Amplitude Modulation (AM)**: Unipolar amplitude scaling creating warm organic sidebands.
- **Ring Modulation (RM)**: Four-quadrant multiplication ($A \times B$) generating inharmonic bell spectra and sci-fi ring textures.
- **Bidirectional Controls**: Independent modulation amounts for **Osc 1 $\to$ Osc 2** and **Osc 2 $\to$ Osc 1**.

### 3. 7 Voice Modes & Polyphonic Coupling Topologies
- **Mono**: Single voice priority with smooth legato portamento.
- **Mono Unison**: 8-voice stacked unison with **$\pm 12\text{ semitones}$** continuous detune spread controlled by `VOICE X-MOD`, with automatic loudness normalization.
- **Poly (Multi-Mono)**: Standard 16-voice polyphony with independent voice envelopes and per-voice filter states.
- **Cyclic Ring**: Ring-modulation topology looped across active voices ($0 \to 1 \to 2 \dots \to 0$).
- **Sympathetic All**: Acoustic soundboard body resonance simulation. Modulates filter formant and resonance across all voices based on aggregate string vibration.
- **Root Driver**: The lowest played pitch acts as a master carrier driver, heavily modulating upper chord tones while upper voices return second-harmonic shimmer.
- **Chaos Diffuse**: Non-linear chaotic differential phase mapping creating evolving organic distortion and analog drift.

### 4. Zero-Delay Resonant Filter (VCF) with Exact 1V/Oct Key Tracking
- **Filter Topologies**: 24dB/oct Ladder Low-Pass, 12dB/oct Low-Pass, Band-Pass, and High-Pass.
- **Self-Oscillating Resonance**: Self-oscillates cleanly into a pure sine wave at high resonance settings.
- **$C^2$ Soft Saturation**: Internal state non-linearities use continuous rational hyperbolic tangent saturation (`fast_tanh`) to eliminate harsh digital clipping.
- **Precise 1V/Oct Key Tracking**:
  - `KeyTrack = 0.5` $\to$ **Exact 1.0 V/Oct** tuning (filter tracks playable MIDI keyboard scale in perfect pitch).
  - `KeyTrack = 1.0` $\to$ **Double tracking (2.0 V/Oct)**.

### 5. Audio-Rate LFOs (0.01 Hz - 2000 Hz) with Key Tracking
- Two versatile LFOs featuring Sine, Triangle, Saw, Square, and Sample & Hold shapes with **PolyBLEP anti-aliasing**.
- Continuous frequency range from subtle micro-drifts ($0.01\text{ Hz}$) up to screaming audio rates ($2000.0\text{ Hz}$).
- Host tempo synchronization (1/64 to 32 bars) or free-running.
- Key tracking support via the modulation matrix to play LFOs in musical pitch tuning across the keyboard.

### 6. 8-Slot Modulation Matrix with Dynamic Active-Slot Caching
- Connect 8 internal modulation sources (LFO 1/2, Amp/Filter Envelopes, KeyTrack, Velocity, ModWheel, Aftertouch) to any synth destination.
- Real-time active-slot caching ensures **0% idle CPU overhead** on unused matrix slots.

### 7. Real-Time Phosphor CRT Lissajous Vectorscope
- Authentic oscilloscope visualizer with selectable modes:
  - **Pre-FX Lissajous**: Dual-channel oscillator phase interaction.
  - **Post-FX Lissajous**: Full stereo master output field visualization.
  - **Pre-FX / Post-FX Waveforms**: Oscilloscope trace view.
- $2.8\times$ analog sensitivity boost with soft saturation for bold, vivid screen presence.

### 8. Studio Multi-FX Chain (Reorderable)
- **Modulation**: Stereo Chorus, Flanger, and Phaser with variable rate, depth, and feedback.
- **Studio Delay**: 
  - Dual independent Flutter phase accumulators preventing phase-wrap discontinuities.
  - 4-point Hermite cubic interpolation for artifact-free time slewing and pitch-bending tape sweeps.
  - Alternating Stereo Ping-Pong and Digital / Tape modes.
- **Studio Reverb**: 4-stage algorithmic reverb (Room, Hall, Plate, Shimmer) with pre-allocated zero-allocation scratch buffers.
- **Signal Order Routing**: Reconfigure the signal order (`Mod -> Delay -> Reverb`, `Delay -> Mod -> Reverb`, `Mod -> Reverb -> Delay`, etc.).

### 9. MIDI Learn & Preset Management
- **MIDI Learn**: Right-click or toggle MIDI Learn mode to bind any hardware MIDI CC knob or fader directly to any parameter.
- **11 Built-in Factory Presets**:
  1. `01 - Init Dual Sine`
  2. `02 - Deep Cross Bass`
  3. `03 - Galactic Bell Matrix`
  4. `04 - Inter-Voice Shimmer (X-Voice)`
  5. `05 - Coupled Harmonic String (X-Voice)`
  6. `06 - Cybernetic Lead`
  7. `07 - Lush Vintage Brass`
  8. `08 - Alien Ring Mod Poly (X-Voice)`
  9. `09 - BigBass`
  10. `10 - Sick Robot`
  11. `11 - Unstable keys`

---

## 🛠️ Building from Source

### Prerequisites
- **CMake** 3.22 or higher
- **C++20** compatible compiler (MSVC 2022, GCC 11+, or Clang 14+)
- **Git** with Submodule support

### 1. Clone the Repository
```bash
git clone --recursive https://github.com/mtyas/CrossMod.git
cd CrossMod
```

*(If cloned without `--recursive`, run `git submodule update --init --recursive`)*

### 2. Configure & Build with CMake

#### Windows (Visual Studio 2022 / Ninja)
```powershell
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target CrossMod_All
```

#### macOS (Xcode / Ninja - Universal Binary)
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build --config Release --target CrossMod_All
```

#### Linux (GCC / Clang)
```bash
sudo apt-get update && sudo apt-get install -y libasound2-dev libjack-jackd2-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libgl1-mesa-dev

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target CrossMod_All
```

### 3. Run Automated Tests
```bash
cmake --build build --config Release --target CrossModTests
./build/CrossModTests_artefacts/Release/CrossModTests
```

---

## 📖 Documentation

- [Full User Manual (`CROSSMOD_MANUAL.md`)](CROSSMOD_MANUAL.md) - Comprehensive technical guide, DSP equations, MIDI CC maps, and sound design tutorials.
- [Promotional Material (`PROMO_MATERIAL.md`)](PROMO_MATERIAL.md) - Features, specifications, and architecture breakdown.

---

## 📄 License

Copyright © 2026 **mtyas**. All rights reserved.
Developed with the [JUCE Framework](https://juce.com) and [CLAP Extensions](https://github.com/free-audio/clap).