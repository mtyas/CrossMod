# 📖 OmniSplit User Manual & Technical Reference
### Multi-Domain Modular Audio Processing & Spectral Splitting Suite
#### Developed by **mtyas** | Version 1.0.0

---

## 📑 Table of Contents
1. [Introduction & Philosophy](#1-introduction--philosophy)
2. [Signal Flow & System Architecture](#2-signal-flow--system-architecture)
3. [The 6 Pre-Processor Splitters](#3-the-6-pre-processor-splitters)
   - [3.1 Transient / Sustain Splitter](#31-transient--sustain-splitter)
   - [3.2 3-Band Zero-Delay Feedback (ZDF) Filterbank](#32-3-band-zero-delay-feedback-zdf-filterbank)
   - [3.3 Mid / Side & L/R Matrix](#33-mid--side--lr-matrix)
   - [3.4 Dynamic Amplitude Splitter](#34-dynamic-amplitude-splitter)
   - [3.5 Spectral & Harmonic Decomposition](#35-spectral--harmonic-decomposition)
   - [3.6 Phase & Spatial Correlation Splitter](#36-phase--spatial-correlation-splitter)
4. [Modular Effects Rack (8 Slots & 35 Algorithms)](#4-modular-effects-rack-8-slots--35-algorithms)
   - [4.1 Universal 25-Source Input Routing](#41-universal-25-source-input-routing)
   - [4.2 Effect Categories & Algorithms](#42-effect-categories--algorithms)
   - [4.3 Tempo Synchronization & Musical Subdivisions](#43-tempo-synchronization--musical-subdivisions)
5. [Audio Routing Matrix & Interactive Patching](#5-audio-routing-matrix--interactive-patching)
6. [Modulation Engine & Matrix](#6-modulation-engine--matrix)
   - [6.1 3 PolyBLEP LFOs](#61-3-polyblep-lfos)
   - [6.2 3 Multi-Source Envelope Followers](#62-3-multi-source-envelope-followers)
   - [6.3 2 Turing Machine Shift Registers](#63-2-turing-machine-shift-registers)
   - [6.4 4 Global Macros & 2 Loopable XY Pads](#64-4-global-macros--2-loopable-xy-pads)
   - [6.5 8-Slot Live Modulation Matrix](#65-8-slot-live-modulation-matrix)
7. [Master Section & 4 Stereo Output Buses](#7-master-section--4-stereo-output-buses)
8. [Preset Management & Undo / Redo](#8-preset-management--undo--redo)
9. [Technical Specifications & Performance](#9-technical-specifications--performance)

---

## 1. Introduction & Philosophy

**OmniSplit** is an advanced modular audio processor designed to break free from traditional serial and parallel effect topologies. Instead of processing an entire mix or instrument as a single monolithic block, OmniSplit decomposes the incoming signal across **6 physical and mathematical decomposition domains**:

1. **Time / Envelope**: Transient attack onsets vs decaying sustain body.
2. **Frequency**: 3-band parametric Zero-Delay Feedback (ZDF) isolation.
3. **Stereo Field**: Mid (mono center), Side (stereo difference), and discrete Left/Right channels.
4. **Dynamics**: High-energy peaks/loud sections vs low-level quiet floor/ghost notes.
5. **Harmonics**: Steady-state pitched sinusoids vs stochastic noise/residual texture.
6. **Spatial Phase**: Correlated in-phase center energy vs diffuse decorrelated spatial cues.

Each of these **16 discrete audio streams** can be routed into an **8-slot modular effects rack** equipped with 35 studio-grade algorithms (delays, reverbs, distortions, modulations, glitch/granular, and dynamics), inter-slot feedforward/feedback patching, and a modulation matrix with live visual feedback.

---

## 2. Signal Flow & System Architecture

```
                                      [ INCOMING AUDIO (L/R) ]
                                                 │
  ┌──────────────────────────────────────────────┴──────────────────────────────────────────────┐
  │                                   6 PRE-PROCESSOR SPLITTERS                                │
  │  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌──────────┐ ┌──────────┐ │
  │  │  Transient   │ │ 3-Band ZDF   │ │ Mid / Side   │ │ Dynamic      │ │ Spectral │ │ Phase /  │ │
  │  │  Splitter    │ │ Filterbank   │ │ & L/R Matrix │ │ Splitter     │ │ Harmonic │ │ Spatial  │ │
  │  └──────┬───────┘ └──────┬───────┘ └──────┬───────┘ └──────┬───────┘ └───┬──────┘ └───┬──────┘ │
  │         │ Attack         │ Band 1         │ Mid/Side       │ Dyn High    │ Harm       │ InPhase│
  │         │ Body           │ Band 2         │ Left/Right     │ Dyn Low     │ Noise      │ Spatial│
  │                          │ Band 3                                                              │
  └─────────┼────────────────┼────────────────┼────────────────┼─────────────┼────────────┼────────┘
            └────────────────┴────────┬───────┴────────────────┴─────────────┴────────────┘
                                      ▼
                        [ 16 REAL-TIME AUDIO STREAMS ]
                                      │
                                      ▼
             [ 8-SLOT MODULAR EFFECTS RACK (Topological DAG Engine) ]
              Each slot selects from 16 Sources + 8 Inter-Slot Out Feeds
                                      │
                                      ▼
          [ 4 STEREO OUTPUT BUSES: Bus 1 (Main), Bus 2, Bus 3, Bus 4 ]
```

All 6 pre-processors operate with **zero real-time memory allocations** and **unconditionally stable DSP**, guaranteeing bit-exact complementary reconstruction ($\text{Split A} + \text{Split B} = \text{Input}$).

---

## 3. The 6 Pre-Processor Splitters

### 3.1 Transient / Sustain Splitter
Separates percussive attack onsets from sustain body energy using dual fast/slow ballistics and lookahead delay.
- **Attack (ms)**: Detection integration window ($1.0 \dots 300.0\text{ ms}$).
- **Release (ms)**: Sustain tracking recovery time ($10.0 \dots 600.0\text{ ms}$).
- **Sensitivity**: Transient threshold sensitivity ($0.1 \dots 3.0\text{x}$).
- **Threshold (dB)**: Minimum energy required for attack isolation ($-60.0 \dots 0.0\text{ dB}$).
- **Lookahead**: Sample lookahead buffer ($0 \dots 128\text{ samples}$) ensuring zero attack transient clipping.
- **Outputs**: `Attack`, `Body`.

### 3.2 3-Band Zero-Delay Feedback (ZDF) Filterbank
3 independent, fully parametric filter bands utilizing Andy Simper ZDF State Variable Filter topologies.
- **Filter Shapes**: LowPass 12/24dB, HighPass 12/24dB, BandPass, Peak/Bell, Notch, LowShelf, HighShelf.
- **Frequency**: $20.0\text{ Hz} \dots 20.0\text{ kHz}$.
- **Q**: $0.1 \dots 10.0$.
- **Gain**: $\pm 18.0\text{ dB}$.
- **Drive**: $1.0\text{x} \dots 4.0\text{x}$ analog warmth saturation.
- **Solo / Mute**: Per-band auditioning.
- **Outputs**: `Filter 1`, `Filter 2`, `Filter 3`.

### 3.3 Mid / Side & L/R Matrix
Encodes stereo audio into sum (Mid) and difference (Side) components, alongside isolated Left and Right feeds.
- **Stereo Width**: Mid/Side ratio scaling ($0.0 = \text{Mono}, 1.0 = \text{Original}, 2.0 = \text{Extra Wide}$).
- **L/R Balance**: Panning offset ($-1.0\dots +1.0$).
- **Outputs**: `Mid (M)`, `Side (S)`, `Left (L)`, `Right (R)`.

### 3.4 Dynamic Amplitude Splitter
Separates loud peaks and forte transients from quiet floor details, ghost notes, and ambient reverb tails.
- **Threshold**: Separation level ($-60.0 \dots 0.0\text{ dB}$).
- **Attack / Release**: Envelope detector tracking ballistics ($0.5 \dots 300\text{ ms}$ / $5 \dots 1500\text{ ms}$).
- **Knee (dB)**: Soft-knee transition curve ($0.1 \dots 24.0\text{ dB}$) with Hermite interpolation.
- **Outputs**: `Dynamic High`, `Dynamic Low`.

### 3.5 Spectral & Harmonic Decomposition
An 8-band Zero-Delay Feedback tracking filter bank separates pitched steady-state sinusoids from unpitched noise and residual texture.
- **Harmonic Sensitivity**: Tracking threshold ($0.0 \dots 1.0$).
- **Spectral Focus**: Selectivity exponent ($0.0 \dots 1.0$).
- **Smoothing Time**: Time constant ($1.0 \dots 100.0\text{ ms}$).
- **Outputs**: `Harmonic` (Pitched / Tonal), `Noise` (Stochastic / Residual).

### 3.6 Phase & Spatial Correlation Splitter
Instantaneous cross-channel correlation estimator ($\rho = \frac{2\langle L,R\rangle}{\langle L,L\rangle + \langle R,R\rangle}$) separating centered mono energy from diffuse out-of-phase stereo ambience.
- **Spatial Threshold**: Correlation boundary cutoff ($0.0 \dots 1.0$).
- **Window (ms)**: Correlation window length ($2.0 \dots 80.0\text{ ms}$).
- **Spatial Spread**: Stereo diffusion multiplier ($0.5 \dots 2.0$).
- **Outputs**: `In-Phase` (Mono Coherent), `Spatial` (Decorrelated Diffuse).

---

## 4. Modular Effects Rack (8 Slots & 35 Algorithms)

### 4.1 Universal 25-Source Input Routing
Each of the 8 effect slots can select its input feed independently from 25 available choices:
- `[0] Disconnected`
- `[1] Stereo In`
- `[2] Attack` | `[3] Body`
- `[4..6] Filter 1, Filter 2, Filter 3`
- `[7..8] Left (L), Right (R)`
- `[9..10] Mid (M), Side (S)`
- `[11..12] Dynamic High, Dynamic Low`
- `[13..14] Harmonic, Noise`
- `[15..16] In-Phase, Spatial`
- `[17..24] Slot 1..8 Output (Direct Inter-Slot Patching)`

### 4.2 Effect Categories & Algorithms

1. **Dynamics**:
   - `VCA Compressor`: Snappy, punchy modern feedforward compressor.
   - `Opto Compressor`: Smooth, program-dependent optical leveling amplifier.
   - `FET Compressor`: Ultra-fast peak limiter with aggressive saturation.
   - `Noise Gate`: Downward expander/gate with variable threshold and decay.

2. **Equalizers & Filters**:
   - `Parametric EQ`: 4-band surgical bell/shelf equalizer.
   - `Graphic EQ`: 8-band octave graphic equalizer.
   - `Tilt EQ`: One-knob tonal balance tilt filter.
   - `Dual HP/LP Filter`: Cascade 24dB Butterworth high-pass and low-pass filters.
   - `Bandpass & Notch`: Dual resonant peak/notch filter.

3. **Distortions & Saturation**:
   - `Hard Clip`: Brickwall digital clipper for crisp, aggressive transients.
   - `Tube Saturation`: Asymmetric 12AX7 triode model with warm even-order harmonics.
   - `Wavefolder`: Multi-stage West Coast analog wavefolder.
   - `Germanium Fuzz`: Vintage soft-knee germanium diode fuzz.
   - `Octave Fuzz`: Full-wave rectified octave-up fuzz.
   - `Overdrive`: Symmetrical soft-clipping TS-style overdrive.
   - `Tape Saturation`: Magnetic hysteresis saturation with HF loss and tape compression.

4. **Glitch & Granular**:
   - `Bitcrusher`: Variable sample rate reduction and bit resolution down to 1-bit.
   - `Stutter Repeater`: Beat-synced buffer repeater with loop decay.
   - `Tape Stop Glitch`: Motor inertia tape stop effect.
   - `Reverse Slice`: Real-time audio buffer reversal with crossfade windowing.
   - `Granular Jitter`: Multi-grain pitch jitter and position scatter.

5. **Pitch & Modulation**:
   - `Pitch Shifter`: Phase-vocoder style pitch shifter ($\pm 24\text{ semitones}$).
   - `Frequency Shifter`: Bode frequency shifter with independent up/down sidebands.
   - `Stereo Chorus`: Multi-voice modulated bucket-brigade delay chorus.
   - `Flanger`: Through-zero flanger with negative and positive feedback.
   - `Phaser`: 8-stage allpass phaser with feedback resonance.
   - `Tremolo / Auto-Pan`: LFO amplitude modulation with stereo phase offset.

6. **Delays & Reverberation**:
   - `Stereo Delay`: Dual-channel delay with independent L/R time and crossfeed.
   - `Ping-Pong Delay`: Bouncing stereo delay with stereo width control.
   - `Tape Echo`: Analog tape delay with wow, flutter, and tape head saturation.
   - `Room Reverb`: Dense early reflections for acoustic drum and vocal spaces.
   - `Hall Reverb`: Large lush concert hall reverberation with HF damping.
   - `Plate Reverb`: Bright, dense metallic plate reverb.
   - `Spring Reverb`: Vintage dual-spring reverb with physical boing dispersion.
   - `Shimmer Reverb`: Dattorro reverb tank with integrated pitch-shifted feedback loop ($\pm 24\text{ semitones}$).

### 4.3 Tempo Synchronization & Musical Subdivisions
All time-based parameters feature 14 musical subdivisions synchronized to host DAW tempo:
- `Free` (Millisecond / Hz rate)
- Subdivisions: `1/32`, `1/16`, `1/8T`, `1/16D`, `1/8`, `1/4T`, `1/8D`, `1/4`, `1/2T`, `1/4D`, `1/2`, `1 Bar`, `2 Bars`, `4 Bars`.

---

## 5. Audio Routing Matrix & Interactive Patching

The **Audio Routing Diagram** provides a visual modular cable matrix:
- **Left Column**: 16 Pre-Processor stream pins.
- **Center Grid**: 8 Draggable effect slot nodes with mini In/Out knobs, power toggles, and algorithm dropdowns.
- **Right Column**: 4 Stereo Output Bus jacks.
- **Interactive Patching**: Click and drag from any output pin to any input pin to establish real-time routing cables. The engine automatically runs topological DAG cycle-prevention to eliminate feedback loops.

---

## 6. Modulation Engine & Matrix

### 6.1 3 PolyBLEP LFOs
- **Waveforms**: Sine, Triangle, Saw Up, Saw Down, Square, Sample & Hold, Smooth Random.
- **Rate**: $0.01\text{ Hz} \dots 50.0\text{ Hz}$ (Free) or $1/32 \dots 8\text{ Bars}$ (Host Sync).
- **Smoothing**: Ultra-deep exponential slewing ($1\text{ ms} \dots 2000\text{ ms}$).

### 6.2 3 Multi-Source Envelope Followers
- **Audio Inputs**: Any of the 16 Pre-Processor streams.
- **Controls**: Attack ($0.5\dots 300\text{ ms}$), Release ($5\dots 1500\text{ ms}$), Sensitivity ($0.1\dots 5.0\text{x}$).

### 6.3 2 Turing Machine Shift Registers
- Generative pseudo-random shift registers generating evolving melodies and modulation.
- **Controls**: Probability ($0\dots 100\%$), Step Length (4, 8, 16, 32 steps), Clock Sync, Glide.

### 6.4 4 Global Macros & 2 Loopable XY Pads
- **Macros 1..4**: Global macro dials assignable to multiple parameters.
- **XY Pads 1 & 2**: Bipolar 2D vector pads with integrated physics orbit loopers.

### 6.5 8-Slot Live Modulation Matrix
- Route any of **22 modulation sources** to **75 plugin destinations** with bipolar depth ($\pm 100\%$) and live activity meters.

---

## 7. Master Section & 4 Stereo Output Buses

OmniSplit features **4 independent stereo output buses**:
- **Bus 1 (Main 1+2)**: Default master mix bus with split balance and master dry/wet crossfading.
- **Bus 2 (Aux 3+4)**: Secondary stem / external sidechain send.
- **Bus 3 (Aux 5+6)**: Multi-channel stem 2.
- **Bus 4 (Aux 7+8)**: Multi-channel stem 3 / FX return.

---

## 8. Preset Management & Undo / Redo

- **8 Curated Factory Presets**: Showcasing transient distortion, harmonic shimmering, M/S widening, spectral vocal splitting, and dynamic glitching.
- **User Presets**: Save and load custom `.xml` presets to `%USERPROFILE%/Documents/mtyas/OmniSplit/Presets`.
- **Full Undo / Redo**: Multi-level state history tracking all parameter changes.

---

## 9. Technical Specifications & Performance

- **Supported Formats**: VST3, CLAP, Standalone (Windows, macOS, Linux).
- **Audio Processing**: 64-bit internal floating-point DSP, 32-bit/64-bit host streaming.
- **Sample Rates**: $44.1\text{ kHz}, 48\text{ kHz}, 88.2\text{ kHz}, 96\text{ kHz}, 176.4\text{ kHz}, 192\text{ kHz}$.
- **Real-Time Safety**: Zero heap allocations on the audio thread, lock-free parameter reads, SIMD vectorized buffer operations.
