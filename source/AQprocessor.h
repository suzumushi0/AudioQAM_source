//
// Copyright (c) 2023 suzumushi
//
// 2023-4-2		AQprocessor.h
//
// Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 (CC BY-NC-SA 4.0).
//
// https://creativecommons.org/licenses/by-nc-sa/4.0/
//

#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"

// suzumushi:
#include "AQparam.h"
#include "AQDDS.h"
#include "AQHilbert.h"
#include "SO2ndordIIRfilters.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

using namespace Steinberg;
using namespace Vst;

namespace suzumushi {

//------------------------------------------------------------------------
//  AudioQAMProcessor
//------------------------------------------------------------------------
class AudioQAMProcessor: public Steinberg::Vst::AudioEffect
{
public:
	AudioQAMProcessor ();
	~AudioQAMProcessor () SMTG_OVERRIDE;

    // Create function
	static Steinberg::FUnknown* createInstance (void* /*context*/) 
	{ 
		return (Steinberg::Vst::IAudioProcessor*)new AudioQAMProcessor; 
	}

	//--- ---------------------------------------------------------------------
	// AudioEffect overrides:
	//--- ---------------------------------------------------------------------
	/** Called at first after constructor */
	Steinberg::tresult PLUGIN_API initialize (Steinberg::FUnknown* context) SMTG_OVERRIDE;
	
	/** Called at the end before destructor */
	Steinberg::tresult PLUGIN_API terminate () SMTG_OVERRIDE;
	
	/** Switch the Plug-in on/off */
	Steinberg::tresult PLUGIN_API setActive (Steinberg::TBool state) SMTG_OVERRIDE;

	/** Will be called before any process call */
	Steinberg::tresult PLUGIN_API setupProcessing (Steinberg::Vst::ProcessSetup& newSetup) SMTG_OVERRIDE;
	
	/** Asks if a given sample size is supported see SymbolicSampleSizes. */
	Steinberg::tresult PLUGIN_API canProcessSampleSize (Steinberg::int32 symbolicSampleSize) SMTG_OVERRIDE;

	/** Here we go...the process call */
	Steinberg::tresult PLUGIN_API process (Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE;
		
	/** For persistence */
	Steinberg::tresult PLUGIN_API setState (Steinberg::IBStream* state) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API getState (Steinberg::IBStream* state) SMTG_OVERRIDE;

//------------------------------------------------------------------------
protected:
	// suzumushi: 
	// GUI and host facing parameters
	struct GUI_param gp;
	struct GUI_param gp_load;						// for setState ()

	// DSP instances 
	static constexpr int HT_IR_LEN = 771;			// impulse response length of Hilbert transformer
	AQDDS <double>						DDS;
	SODDL <double, (HT_IR_LEN - 1) / 2>	DDL_L;
	SODDL <double, (HT_IR_LEN - 1) / 2>	DDL_R;
	AQHilbert <double, HT_IR_LEN>		HT_L;
	AQHilbert <double, HT_IR_LEN>		HT_R;
	SOHPF <double>						I_HPF_L;
	SOHPF <double>						I_HPF_R;
	SOLPF <double, i_l_freq.max>		I_LPF_L;
	SOLPF <double, i_l_freq.max>		I_LPF_R;
	SOHPF <double>						O_HPF_L;
	SOHPF <double>						O_HPF_R;
	SOLPF <double, o_l_freq.max>		O_LPF_L;
	SOLPF <double, o_l_freq.max>		O_LPF_R;

	// internal functions
	void gui_param_loading ();
	void gui_param_update (const ParamID paramID, const ParamValue paramValue);
	void dsp_param_update (IParameterChanges* outParam);
	void reset ();	
};

//------------------------------------------------------------------------
} // namespace suzumushi
