#pragma once
#include "additive_synth.h"
#include "arp.h"
#include "bitcrusher.h"
#include "compressor.h"
#include "decibel.h"
#include "delay.h"
#include "distortion.h"
#include "equalizer.h"
#include "filter.h"
#include "flanger.h"
#include "fm.h"
#include "group.h"
#include "improviser.h"
#include "keyboard.h"
#include "midi_player.h"
#include "oscillator.h"
#include "oscilloscope.h"
#include "pan.h"
#include "phaser.h"
#include "sampler.h"
#include "sequencer.h"
#include "speaker.h"
#include "spectrum.h"
#include "split.h"
#include "vectorscope.h"
#include "vocoder.h"

#include "meta.h"

template<typename T>
concept IsComponent = std::derived_from<T, Component>;

template<IsComponent ... Ts>
using ComponentTypeList = meta::TypeList<Ts ...>;

using AllComponents = ComponentTypeList<
	AdditiveSynthComponent,
	ArpComponent,
	BitCrusherComponent,
	CompressorComponent,
	DecibelComponent,
	DelayComponent,
	DistortionComponent,
	EqualizerComponent,
	FilterComponent,
	FlangerComponent,
	FMComponent,
	GainComponent,
	ImproviserComponent,
	KeyboardComponent,
	OscillatorComponent,
	OscilloscopeComponent,
	PanComponent,
	PhaserComponent,
	MIDIPlayerComponent,
	SamplerComponent,
	SequencerComponent,
	SpeakerComponent,
	SpectrumComponent,
	SplitComponent,
	VectorscopeComponent,
	VocoderComponent
>; // TypeList of all Components, used for deserialization. Synth::add_component<T> will only accept T that occur in this TypeList.
