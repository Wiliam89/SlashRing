# FILE MAP

PluginProcessor

Purpose

Coordinates the complete DSP chain.

Owns DSP modules.

Does NOT implement audio algorithms.

------------------------------------------------

InputStage

Input calibration

Pickup simulation

Input gain

------------------------------------------------

OverdriveModule

Drive

Tone

Level

Soft clipping

------------------------------------------------

AmpModule

Preamp

Tone Stack

Power Amp

Master

------------------------------------------------

CabinetModule

Coordinates all Cabinet processing.

Owns:

CabinetVoice

IR Loader

------------------------------------------------

CabinetVoice

Cabinet DSP pipeline.

Owns:

CabinetPhysics

CabinetAcoustics

CabinetConvolution

------------------------------------------------

CabinetConvolution

FFT convolution.

Impulse Response processing.

------------------------------------------------

CabinetPhysics

Speaker resonance

Mechanical behaviour

------------------------------------------------

CabinetAcoustics

Cabinet reflections

Air interaction

------------------------------------------------

DelayModule

Delay DSP

------------------------------------------------

ReverbModule

Reverb DSP

------------------------------------------------

OutputStage

Limiter

Output Gain

Final processing