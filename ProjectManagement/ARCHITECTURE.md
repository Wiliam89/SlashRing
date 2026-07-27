## ARCHITECTURE.md

Version: 1.0

Status: Official Technical Architecture

Project: SlashRing Professional Guitar Suite

Author: Wiliam Oliveira

Chief Software Architect: ChatGPT

---

# Purpose

This document defines the official software architecture of SlashRing.

It is the definitive technical reference for every implementation performed during development.

No implementation may contradict this document without an approved architectural revision.

---

# Architecture Principles

SlashRing follows the architecture of a modern commercial guitar processing suite.

Primary goals:

* Commercial reliability
* Modular DSP
* Low latency
* Real-time safety
* High maintainability
* Expandable architecture
* Independent processing stages
* Stable parameter automation
* Professional code organization

---

# System Layers

The project is divided into five major layers.

## Layer 1 — User Interface

Responsibilities:

* Display controls
* Receive user interaction
* Parameter attachments
* Preset selection
* Meter visualization

The UI never performs DSP.

---

## Layer 2 — Parameter Management

Responsible component:

AudioProcessorValueTreeState

Responsibilities:

* Parameter storage
* Automation
* State serialization
* Preset compatibility

The parameter system never processes audio.

---

## Layer 3 — Audio Engine

Responsible component:

PluginProcessor

Responsibilities:

* Build DSP chain
* Prepare modules
* Process audio
* Synchronize parameters
* Manage oversampling
* Own all DSP modules

PluginProcessor is the only component allowed to communicate directly with every DSP module.

DSP modules never communicate directly with each other.

---

## Layer 4 — DSP Modules

Each processing stage is an independent module.

Current modules:

InputStage

OverdriveModule

AmpModule

CabinetModule

DelayModule

ReverbModule

OutputStage

Each module owns only its internal DSP.

No module owns another module.

---

## Layer 5 — Host

Examples:

* Cubase
* Reaper
* Studio One
* Logic
* Pro Tools

The host communicates only with PluginProcessor.

---

# Official Signal Flow

The official audio path is:

Input

↓

InputStage

↓

Oversampling Engine

↓

Overdrive

↓

Amp

↓

Downsampling

↓

Cabinet

↓

Delay

↓

Reverb

↓

OutputStage

↓

Host Output

This signal flow is mandatory.

---

# Oversampling Architecture

Oversampling exists only to improve nonlinear processing.

Modules inside oversampling:

* OverdriveModule
* AmpModule

Modules outside oversampling:

* InputStage
* CabinetModule
* DelayModule
* ReverbModule
* OutputStage

Reason:

Only nonlinear stages generate significant aliasing.

Linear processors do not benefit enough to justify additional CPU usage.

---

# Module Responsibilities

## InputStage

Responsibilities:

* Input gain
* Pickup calibration
* Pickup voicing
* High-pass filtering
* Signal conditioning

Must never generate distortion.

---

## OverdriveModule

Responsibilities:

* Input boost
* Soft saturation
* Gain stage
* Asymmetrical clipping
* Output recovery

Inspired by classic analog overdrive pedals.

Designed as an independent pedal.

---

## AmpModule

Responsibilities:

* Preamp
* Tone Stack
* Power Amp
* Dynamic saturation
* Master section

Inspired by classic British high-gain amplifiers.

---

## CabinetModule

Responsibilities:

Current:

* Cabinet response
* Resonance
* Spectral shaping

Future:

* Convolution IR
* Microphone simulation
* Multi-IR
* Mic blend
* Room simulation

---

## DelayModule

Responsibilities:

* Stereo delay
* Feedback
* Mix control
* Real-time safe processing

---

## ReverbModule

Responsibilities:

* Room simulation
* Wet/Dry mix
* Real-time safe processing

---

## OutputStage

Responsibilities:

* Output gain
* Final protection limiter
* Output conditioning

Must never modify tone intentionally.

---

# Module Lifecycle

Every DSP module follows the same lifecycle.

prepare()

↓

reset()

↓

process()

↓

release()

No dynamic memory allocation is allowed during process().

---

# DSP Processing Rules

Every DSP module must:

* Be real-time safe
* Avoid heap allocation during processing
* Use deterministic execution
* Support stereo processing
* Support parameter smoothing
* Protect against denormals
* Be independent
* Be reusable

---

# Audio Buffer Standard

Official processing format:

juce::AudioBuffer<float>

Internal DSP may convert to:

juce::dsp::AudioBlock<float>

when required.

AudioBlock is the preferred internal representation for DSP processing.

---

# Parameter Flow

Official parameter flow:

User Interface

↓

AudioProcessorValueTreeState

↓

PluginProcessor

↓

DSP Modules

Modules never read APVTS directly.

---

# Memory Ownership

PluginProcessor owns every DSP module through std::unique_ptr.

Modules own only their internal DSP objects.

No shared ownership is permitted.

---

# Thread Safety

UI Thread

Responsible for:

* User interaction
* Parameter editing

Audio Thread

Responsible for:

* Audio processing

DSP modules must never block the audio thread.

---

# Coding Standards

Mandatory:

* RAII
* Modern C++
* Smart pointers
* Deterministic execution
* Explicit ownership
* Modular organization

Forbidden:

* Global mutable state
* Heap allocation inside process()
* Cross-module dependencies
* Hidden parameter synchronization

---

# Future Expansion

The architecture is prepared for:

* Professional IR Loader
* Multiple cabinets
* Parallel routing
* Noise Gate
* Compressor
* Chorus
* Phaser
* Flanger
* MIDI Learn
* Preset Browser
* Scene System
* Standalone Version

These features shall not require architectural redesign.

---

# Architecture Authority

This document is the official architectural reference of SlashRing.

Whenever implementation details conflict with this document, the architecture defined here takes precedence until formally revised.
