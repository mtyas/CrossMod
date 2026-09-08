# 🎛️ OmniSplit: Multi-Domain Modular Audio Processing & Spectral Splitting Suite
### Developed by **mtyas** | VST3 • CLAP • Standalone (Windows / macOS / Linux)

[![C++20](https://img.shields.io/badge/Language-C%2B%2B20-blue.svg)](https://isocpp.org/)
[![JUCE 8](https://img.shields.io/badge/Framework-JUCE_8-orange.svg)](https://juce.com/)
[![Format](https://img.shields.io/badge/Format-VST3_%7C_CLAP_%7C_Standalone-green.svg)](https://github.com/free-audio/clap)
[![Tests](https://img.shields.io/badge/Tests-18%2F18%20Passing-brightgreen.svg)]()
[![License](https://img.shields.io/badge/License-Proprietary-red.svg)]()

---

**OmniSplit** is a modular multi-domain audio processor and creative sound design workstation. It deconstructs incoming audio across **6 real-time physical & mathematical decomposition domains** (transient/sustain, 3-band ZDF filterbank, Mid/Side stereo, dynamic amplitude, spectral harmonicity, and phase/spatial correlation) and routes each stream into an **8-slot modular effects rack** with 35 studio-grade algorithms, complete inter-slot DAG patching, and a modulation engine with LFOs, multi-source envelope followers, Turing machines, XY pads, and an 8-slot live modulation matrix.

---

## 📸 Architecture & Feature Matrix

```
+----------------------------------------------------------------------------------------------------+
|  OMNISPLIT                      [ PRESET: 01 - Harmonic Shimmer & Spatialize ]     [UNDO] [REDO]   |
+----------------------------------------------------------------------------------------------------+
|  1. PRE-PROCESSORS (6 Splitters) |  2. EFFECTS RACK (8 Modular Slots)  |  3. MODULATION ENGINE     |
|  - Transient Split (Attack/Body) |  - Slot 1..8 with 35 FX Algorithms  |  - 3 LFOs (0.01-50Hz/Sync)|
|  - 3-Band ZDF Filterbank         |  - Universal 25-Source Input Choice |  - 3 Multi-Source Env Foll|
|  - Mid / Side & L/R Matrix       |  - 15 Flexible Routing Destinations |  - 2 Turing Shift Regs    |
|  - Dynamic Split (Loud / Quiet)  |  - Level Staging (In / Out / Mix)   |  - 4 Global Macros        |
|  - Spectral Harmonic / Noise     |  - Full Tempo Sync Subdivisions     |  - 2 Loopable XY Pads     |
|  - Phase / Spatial Diffuse       |  - Real-Time Zero-Allocation Audio  |  - 8-Slot Live Mod Matrix |
+----------------------------------+-------------------------------------+---------------------------+
|  4. AUDIO ROUTING DIAGRAM        |  5. MASTER & BUS ROUTING (4 Stereo Buses)                       |
|  - Interactive Cable Patching    |  - Bus 1 (Main 1+2), Bus 2 (Aux 3+4), Bus 3 (5+6), Bus 4 (7+8)  |
|  - Visual DAG Connection Cords   |  - Split Balance, Master Dry/Wet, Master Level Staging          |
+----------------------------------------------------------------------------------------------------+
```

---

## ✨ Core Features

### 1. 🎛️ 6 Advanced Real-Time Pre-Processor Splitters
1. **Transient Splitter**:
   - Energy-envelope ratio detector splitting incoming audio into **Attack (Transient)** and **Body (Sustain)** with zero-latency complementary reconstruction.
   - Controls: *Attack Time (1–300 ms)*, *Release Time (10–600 ms)*, *Sensitivity*, *Threshold (dB)*, *Lookahead (0–128 samples)*.
2. **3-Band Zero-Delay Feedback (ZDF) Filterbank**:
   - 3 independent, fully parametric ZDF State Variable Filter channels with customizable filter curves (LowPass, HighPass, BandPass, Peak, Notch, Shelf).
   - Controls: *Frequency (20 Hz–20 kHz)*, *Q (0.1–10.0)*, *Gain ($\pm 18\text{ dB}$)*, *Drive (1–4x)*, *Solo / Mute*.
3. **Mid / Side & L/R Decomposition Matrix**:
   - Pure linear matrixing ($\text{Mid} = \frac{L+R}{\sqrt{2}}$, $\text{Side} = \frac{L-R}{\sqrt{2}}$) and discrete Left/Right channels with bit-exact complementary sum.
   - Controls: *Stereo Width ($0.0\dots 2.0$)*, *L/R Balance ($-1.0\dots +1.0$)*.
4. **Dynamic Amplitude Splitter**:
   - Splits audio into **Dynamic High (Loud Peaks / Forte)** vs **Dynamic Low (Quiet Floor / Ghost Notes / Tails)** using smooth ballistics with soft-knee interpolation.
   - Controls: *Threshold ($-60\dots 0\text{ dB}$)*, *Attack ($0.5\dots 300\text{ ms}$)*, *Release ($5\dots 1500\text{ ms}$)*, *Knee Width ($0.1\dots 24\text{ dB}$)*.
5. **Spectral & Harmonic Decomposition**:
   - 8-band Zero-Delay Feedback (ZDF) tracking filter bank separating steady pitched **Harmonic / Tonal** audio from stochastic **Noise / Residual** texture with exact unity reconstruction.
   - Controls: *Harmonic Sensitivity*, *Spectral Focus*, *Smoothing Time (1–100 ms)*.
6. **Phase & Spatial Correlation Splitter**:
   - Instantaneous cross-channel correlation estimator ($\rho = \frac{2\langle L,R\rangle}{\langle L,L\rangle + \langle R,R\rangle}$) isolating centered **In-Phase (Mono Coherent)** audio from wide decorrelated **Spatial (Diffuse Ambience)**.
   - Controls: *Spatial Threshold*, *Window Time (2–80 ms)*, *Spatial Spread*.

---

### 2. 🔌 Universal 25-Source Input Matrix & 8 Modular Slots
Every effect slot in OmniSplit can select from **16 pre-processor audio sources** or **8 inter-slot feedback/feedforward feeds**:

| Index | Audio Stream Source | Index | Audio Stream Source |
|---|---|---|---|
| `[0]` | **Disconnected** | `[13]` | **Harmonic (Tonal)** |
| `[1]` | **Stereo In** | `[14]` | **Noise (Residual)** |
| `[2]` | **Attack (Transient)** | `[15]` | **In-Phase (Mono Coherent)** |
| `[3]` | **Body (Sustain)** | `[16]` | **Spatial (Diffuse Ambience)** |
| `[4..6]` | **Filter 1, 2, 3** | `[17..24]` | **Slot 1..8 Output (Inter-Slot)** |
| `[7..8]` | **Left (L), Right (R)** | | |
| `[9..10]`| **Mid (M), Side (S)** | | |
| `[11..12]`| **Dynamic High, Dynamic Low** | | |

---

### 3. 🧩 35 Studio-Grade Effect Algorithms
- **Dynamics**: VCA Compressor, Opto Compressor, FET Compressor, Noise Gate.
- **Equalizers & Filters**: 4-Band Parametric EQ, 8-Band Graphic EQ, Tilt EQ, Dual HP/LP ZDF Filter, Bandpass & Notch Filter.
- **Distortion & Saturation**: Hard Clipper, Tube Saturation, Wavefolder, Germanium Fuzz, Octave Fuzz, Overdrive, Analog Tape Saturation.
- **Glitch & Granular**: Bitcrusher, Stutter Repeater, Tape Stop Glitch, Reverse Slice, Granular Jitter.
- **Pitch & Modulation**: Pitch Shifter ($\pm 24\text{ st}$), Frequency Shifter, Multi-Voice Stereo Chorus, Flanger, 8-Stage Phaser, Tremolo / Auto-Pan.
- **Delays & Echo**: Stereo Delay, Ping-Pong Delay, Analog Tape Echo (with wow/flutter and saturation).
- **Reverberation**: Algorithmic Room Reverb, Hall Reverb, Plate Reverb, Spring Reverb, Dattorro Shimmer Reverb ($\pm 24\text{ st}$ pitch shift in feedback loop).
- **Tempo Sync**: 14 musical subdivisions (`Free`, `1/32`, `1/16`, `1/8T`, `1/16D`, `1/8`, `1/4T`, `1/8D`, `1/4`, `1/2T`, `1/4D`, `1/2`, `1 Bar`, `2 Bars`, `4 Bars`) on all time-based effects.

---

### 4. 🎛️ Modulation Engine & Live Mod Matrix
- **3 PolyBLEP LFOs**: Sine, Triangle, Saw Up, Saw Down, Square, Sample & Hold, Smooth Random (0.01 Hz to 50.0 Hz or synced subdivisions from 1/32 to 8 Bars).
- **3 Multi-Source Envelope Followers**: Fed by any of the 16 pre-processor audio sources.
- **2 Turing Machine Shift Registers**: Generative musical shift registers with probability, step lengths (4, 8, 16, 32), clock sync, and glide smoothing.
- **4 Global Macro Knobs & 2 Loopable XY Pads**: Real-time multi-parameter macro controllers with physics-based circular and Lissajous orbit loopers.
- **8-Slot Modulation Matrix**: 22 modulation sources assignable to 75 plugin parameters with live visual metering.

---

### 5. 🔊 4 Stereo Output Buses
- **Bus 1 (Main 1+2)**: Primary stereo mix output.
- **Bus 2 (Aux 3+4)**: Secondary stereo stem / sidechain output.
- **Bus 3 (Aux 5+6)**: Multi-channel sub-mix output.
- **Bus 4 (Aux 7+8)**: Multi-channel ambient / fx send output.

---

## 🛠️ Building OmniSplit

### Prerequisites
- **CMake 3.22+**
- **C++20 compliant compiler** (MSVC 2022, GCC 11+, Clang 14+)

### Build Commands
```bash
# Configure build
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build all plugin formats & unit tests
cmake --build build --config Release --target OmniSplit_VST3 OmniSplit_CLAP OmniSplit_Standalone OmniSplitTests

# Run automated test suite
./build/Release/OmniSplitTests.exe
```

---

## 📄 License
Proprietary software. Developed by **mtyas**. All rights reserved.
