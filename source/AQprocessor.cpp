//
// Copyright (c) 2023 suzumushi
//
// 2023-4-5		AQprocessor.cpp
//
// Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 (CC BY-NC-SA 4.0).
//
// https://creativecommons.org/licenses/by-nc-sa/4.0/
//

#include "AQprocessor.h"
#include "AQcids.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

using namespace Steinberg;

namespace suzumushi {
//------------------------------------------------------------------------
// AudioQAMProcessor
//------------------------------------------------------------------------
AudioQAMProcessor:: AudioQAMProcessor ()
{
	//--- set the wanted controller for our processor
	setControllerClass (kAudioQAMControllerUID);
}

//------------------------------------------------------------------------
AudioQAMProcessor:: ~AudioQAMProcessor ()
{}

//------------------------------------------------------------------------
tresult PLUGIN_API AudioQAMProcessor:: initialize (FUnknown* context)
{
	// Here the Plug-in will be instantiated
	
	//---always initialize the parent-------
	tresult result = AudioEffect::initialize (context);
	// if everything Ok, continue
	if (result != kResultOk) {
		return result;
	}

	//--- create Audio IO ------
	// suzumushi:
	addAudioInput (STR16 ("Stereo In"), Steinberg::Vst::SpeakerArr::kStereo);
	addAudioOutput (STR16 ("Stereo Out"), Steinberg::Vst::SpeakerArr::kStereo);

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API AudioQAMProcessor:: terminate ()
{
	// Here the Plug-in will be de-instantiated, last possibility to remove some memory!
	
	//---do not forget to call parent ------
	return AudioEffect::terminate ();
}

//------------------------------------------------------------------------
tresult PLUGIN_API AudioQAMProcessor:: setActive (TBool state)
{
	// suzumushi:
	if (state != 0)				// if (state == true)
		reset ();

	//--- called when the Plug-in is enable/disable (On/Off) -----
	return AudioEffect::setActive (state);
}

//------------------------------------------------------------------------
tresult PLUGIN_API AudioQAMProcessor:: process (Vst::ProcessData& data)
{
	//--- First : Read inputs parameter changes-----------

    if (data.inputParameterChanges) {
        int32 numParamsChanged = data.inputParameterChanges->getParameterCount ();
        for (int32 index = 0; index < numParamsChanged; index++) {
            if (auto* paramQueue = data.inputParameterChanges->getParameterData (index)) {
                Vst::ParamValue value;
                int32 sampleOffset;
                int32 numPoints = paramQueue->getPointCount ();
				// suzumushi: get the last change
				if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) == kResultTrue)
					gui_param_update (paramQueue->getParameterId (), value);
			}
		}
	}

	//--- Here you have to implement your processing

	dsp_param_update (data.outputParameterChanges);

	// numInputs == 0 and data.numOutputs == 0 mean parameters update only
	if (data.numInputs == 0 || data.numOutputs == 0) {
		return kResultOk;
	}

	// Speaker arrangements (stereo in and out are required) check.
	if (data.inputs[0].numChannels < 2 || data.outputs[0].numChannels < 2) {
		return kResultOk;
	}

	Vst::Sample32* in_L = data.inputs[0].channelBuffers32[0];
	Vst::Sample32* in_R = data.inputs[0].channelBuffers32[1];
	Vst::Sample32* out_L = data.outputs[0].channelBuffers32[0];
	Vst::Sample32* out_R = data.outputs[0].channelBuffers32[1];

	if (gp.bypass) {
		// bypass mode
		for (int32 i = 0; i < data.numSamples; i++) {
			if (data.inputs[0].silenceFlags == 0) {
				// all silenceFlags are false
				*out_L++ = *in_L++;
				*out_R++ = *in_R++;
			} else {
				// some silenceFlags are true
				*out_L++ = *out_R++ = 0.0;
				data.outputs[0].silenceFlags = data.inputs[0].silenceFlags;
			}
		}
	} else {
		// DSP mode
		for (int32 i = 0; i < data.numSamples; i++) {	
			double xn, xHn;
			DDS.process (gp.wform, xn, xHn);

			DDL_L.enqueue (*in_L);
			DDL_R.enqueue (*in_R);

			double yn_L, yn_R;
			yn_L = I_LPF_L.process (I_HPF_L.process (*in_L++));
			yn_R = I_LPF_R.process (I_HPF_R.process (*in_R++));

			double zn_L, zHn_L, zn_R, zHn_R;
			HT_L.process (yn_L , zn_L, zHn_L);
			HT_R.process (yn_R , zn_R, zHn_R);

			if (gp.c_sb_switching && abs (xHn) < 0.01)	// side band switching noise reduction
				gp.c_sb_switching = false;

			if (! gp.c_sb_switching && gp.c_freq < 0.0 || gp.c_sb_switching && gp.c_freq >= 0.0) {	// LSB
				yn_L = zn_L * xn + zHn_L * xHn;
				yn_R = zn_R * xn + zHn_R * xHn;	
			} else {									// USB
				yn_L = zn_L * xn - zHn_L * xHn;
				yn_R = zn_R * xn - zHn_R * xHn;	
			}

			*out_L = gp.wet * O_LPF_L.process (O_HPF_L.process (yn_L));
			*out_R = gp.wet * O_LPF_R.process (O_HPF_R.process (yn_R));

			*out_L++ += gp.dry * DDL_L.read ();
			*out_R++ += gp.dry * DDL_R.read ();
		}
	}
	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API AudioQAMProcessor:: setupProcessing (Vst::ProcessSetup& newSetup)
{
	//--- called before any processing ----
	return AudioEffect::setupProcessing (newSetup);
}

//------------------------------------------------------------------------
tresult PLUGIN_API AudioQAMProcessor:: canProcessSampleSize (int32 symbolicSampleSize)
{
	// by default kSample32 is supported
	if (symbolicSampleSize == Vst::kSample32)
		return kResultTrue;

	// disable the following comment if your processing support kSample64
	/* if (symbolicSampleSize == Vst::kSample64)
		return kResultTrue; */

	return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API AudioQAMProcessor:: setState (IBStream* state)
{
	// called when we load a preset, the model has to be reloaded
	IBStreamer streamer (state, kLittleEndian);

	// suzumushi:
	if (gp_load.load == true)
		return (kResultFalse);

	int version;
	if (streamer.readInt32 (version) == false)
		return (kResultFalse);

	if (streamer.readDouble (gp_load.c_freq) == false)
		return (kResultFalse);
	if (streamer.readInt32 (gp_load.wform) == false)
		return (kResultFalse);
	if (streamer.readDouble (gp_load.c_slide) == false)
		return (kResultFalse);
	if (streamer.readInt32 (gp_load.c_range) == false)
		return (kResultFalse);
	if (streamer.readInt32 (gp_load.c_scale) == false)
		return (kResultFalse);

	if (streamer.readDouble (gp_load.i_h_freq) == false)
		return (kResultFalse);
	if (streamer.readDouble (gp_load.i_l_freq) == false)
		return (kResultFalse);
	if (streamer.readDouble (gp_load.o_h_freq) == false)
		return (kResultFalse);
	if (streamer.readDouble (gp_load.o_l_freq) == false)
		return (kResultFalse);
	if (streamer.readDouble (gp_load.wet) == false)
		return (kResultFalse);

	if (streamer.readInt32 (gp_load.bypass) == false)
		return (kResultFalse);

	gp_load.load = true;

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API AudioQAMProcessor:: getState (IBStream* state)
{
	// here we need to save the model
	IBStreamer streamer (state, kLittleEndian);

	// suzumushi:
	int version = 0;
	if (streamer.writeInt32 (version) == false)
		return (kResultFalse);

	if (streamer.writeDouble (gp.c_freq) == false)
		return (kResultFalse);
	if (streamer.writeInt32 (gp.wform) == false)
		return (kResultFalse);
	if (streamer.writeDouble (gp.c_slide) == false)
		return (kResultFalse);
	if (streamer.writeInt32 (gp.c_range) == false)
		return (kResultFalse);
	if (streamer.writeInt32 (gp.c_scale) == false)
		return (kResultFalse);

	if (streamer.writeDouble (gp.i_h_freq) == false)
		return (kResultFalse);
	if (streamer.writeDouble (gp.i_l_freq) == false)
		return (kResultFalse);
	if (streamer.writeDouble (gp.o_h_freq) == false)
		return (kResultFalse);
	if (streamer.writeDouble (gp.o_l_freq) == false)
		return (kResultFalse);
	if (streamer.writeDouble (gp.wet) == false)
		return (kResultFalse);

	if (streamer.writeInt32 (gp.bypass) == false)
		return (kResultFalse);

	return kResultOk;
}
//------------------------------------------------------------------------
// suzumushi:

void AudioQAMProcessor:: gui_param_loading ()
{
	if (gp_load.load) {
		gp.c_freq = gp_load.c_freq;
		gp.wform = gp_load.wform;
		gp.c_slide = gp_load.c_slide;
		gp.c_range = gp_load.c_range;
		gp.c_scale = gp_load.c_scale;
		gp.i_h_freq = gp_load.i_h_freq;
		gp.i_l_freq = gp_load.i_l_freq;
		gp.o_h_freq = gp_load.o_h_freq;
		gp.o_l_freq = gp_load.o_l_freq;
		gp.wet = gp_load.wet;
		gp.dry = 1.0 - gp.wet;
		gp.bypass = gp_load.bypass;

		gp_load.load = false;
		reset ();
	}
}

void AudioQAMProcessor:: gui_param_update (const ParamID paramID, const ParamValue paramValue)
{
	Vst::ParamValue update;

	switch (paramID) {
		case c_freq.tag:
			update = rangeParameter::toPlain (paramValue, c_freq.min, c_freq.max);
			if (gp.c_freq != update) {
				double sgn = gp.c_freq * update;
				if (sgn < 0.0 || (sgn == 0.0 && (gp.c_freq < 0.0 || update < 0.0)))
					gp.c_sb_switching = true;		// side band switching 
				gp.c_freq = update;
				gp.c_freq_changed = true;
			}
			break;
		case wform.tag:
			gp.wform = stringListParameter::toPlain (paramValue, (int32)WFORM_L::LIST_LEN);
			break;
		case c_slide.tag:
			update = rangeParameter::toPlain (paramValue, c_slide.min, c_slide.max);
			if (gp.c_slide != update) {
				double sgn = gp.c_slide * update;
				if (sgn < 0.0 || (sgn == 0.0 && (gp.c_slide < 0.0 || update < 0.0)))
					gp.c_sb_switching = true;		// side band switching 
				gp.c_slide = update;
				gp.c_slide_changed = true;
			}
			break;
		case c_range.tag:
			update = stringListParameter::toPlain (paramValue, (int32)C_RANGE_L::LIST_LEN);
			if (gp.c_range != update) {
				gp.c_range = update;
				gp.c_range_changed = true;
			}
			break;
		case c_scale.tag:
			update = stringListParameter::toPlain (paramValue, (int32)C_SCALE_L::LIST_LEN);
			if (gp.c_scale != update) {
				gp.c_scale = update;
				gp.c_scale_changed = true;
			}
			break;
		case i_h_freq.tag:
			update = Vst::LogTaperParameter::toPlain (paramValue, i_h_freq.min, i_h_freq.max);
			if (gp.i_h_freq != update) {
				gp.i_h_freq = update;
				gp.i_h_freq_changed = true;
			}
			break;
		case i_l_freq.tag:
			update = Vst::LogTaperParameter::toPlain (paramValue, i_l_freq.min, i_l_freq.max);
			if (gp.i_l_freq != update) {
				gp.i_l_freq = update;
				gp.i_l_freq_changed = true;
			}
			break;
		case o_h_freq.tag:
			update = Vst::LogTaperParameter::toPlain (paramValue, o_h_freq.min, o_h_freq.max);
			if (gp.o_h_freq != update) {
				gp.o_h_freq = update;
				gp.o_h_freq_changed = true;
			}
			break;
		case o_l_freq.tag:
			update = Vst::LogTaperParameter::toPlain (paramValue, o_l_freq.min, o_l_freq.max);
			if (gp.o_l_freq != update) {
				gp.o_l_freq = update;
				gp.o_l_freq_changed = true;
			}
			break;
		case wet.tag:
			gp.wet = rangeParameter::toPlain (paramValue, wet.min, wet.max);
			gp.dry = 1.0 - gp.wet;
			break;
		case bypass.tag:
			gp.bypass = paramValue;
			if (! gp.bypass)
				reset ();
			break;
	}
}

void AudioQAMProcessor:: dsp_param_update (IParameterChanges* outParam)
{
	gui_param_loading ();

	if (gp.reset || gp.c_freq_changed || gp.c_range_changed || gp.c_scale_changed) {
		if (gp.reset || gp.c_freq_changed)
			DDS.setup (processSetup.sampleRate, abs (gp.c_freq));
		if (gp.c_freq >= c_range_val [gp.c_range])
			gp.c_slide = 1.0;
		else if (gp.c_freq <= - c_range_val [gp.c_range])
			gp.c_slide = -1.0;
		else {
			if (gp.c_scale == (int32)C_SCALE_L::LINEAR)
				gp.c_slide = abs (gp.c_freq) / c_range_val [gp.c_range];
			else					// Logarithmic
				gp.c_slide = Vst::LogTaperParameter::toNormalized (abs (gp.c_freq), 0.0, c_range_val [gp.c_range]);
			if (gp.c_freq < 0.0)
				gp.c_slide = - gp.c_slide;
		}
		gp.c_freq_changed = gp.c_range_changed = gp.c_slide_changed = gp.c_scale_changed = false;
		// feedback gp.c_slide
		if (outParam) {
			int32 q_index = 0;		// paramQueue index
			int32 p_index = 0;		// parameter index
			int32 p_offset = 0;		// parameter offset
			IParamValueQueue* paramQueue = outParam->addParameterData (c_slide.tag, q_index);
			if (paramQueue)
				paramQueue->addPoint (p_offset, rangeParameter::toNormalized (gp.c_slide, c_slide.min, c_slide.max), p_index);;
		}
	}

	if (gp.c_slide_changed) {
		gp.c_slide_changed = false;
		if (gp.c_scale == (int32)C_SCALE_L::LINEAR)		
			gp.c_freq = abs (gp.c_slide) * c_range_val [gp.c_range];
		else						// Logarithmic
			gp.c_freq = Vst::LogTaperParameter::toPlain (abs (gp.c_slide), 0.0, c_range_val [gp.c_range]);
		DDS.setup (processSetup.sampleRate, gp.c_freq);
		if (gp.c_slide < 0.0)
			gp.c_freq = - gp.c_freq;
		// feedback gp.c_freq
		if (outParam) {
			int32 q_index = 0;		// paramQueue index
			int32 p_index = 0;		// parameter index
			int32 p_offset = 0;		// parameter offset
			IParamValueQueue* paramQueue = outParam->addParameterData (c_freq.tag, q_index);
			if (paramQueue)
				paramQueue->addPoint (p_offset, rangeParameter::toNormalized (gp.c_freq, c_freq.min, c_freq.max), p_index);;
		}
	}

	if (gp.reset || gp.i_h_freq_changed) {
		gp.i_h_freq_changed = false;
		I_HPF_L.setup (processSetup.sampleRate, gp.i_h_freq);
		I_HPF_R.setup (processSetup.sampleRate, gp.i_h_freq);
	}

	if (gp.reset || gp.i_l_freq_changed) {
		gp.i_l_freq_changed = false;
		I_LPF_L.setup (processSetup.sampleRate, gp.i_l_freq);
		I_LPF_R.setup (processSetup.sampleRate, gp.i_l_freq);
	}

	if (gp.reset || gp.o_h_freq_changed) {
		gp.o_h_freq_changed = false;
		O_HPF_L.setup (processSetup.sampleRate, gp.o_h_freq);
		O_HPF_R.setup (processSetup.sampleRate, gp.o_h_freq);
	}

	if (gp.reset || gp.o_l_freq_changed) {
		gp.o_l_freq_changed = false;
		O_LPF_L.setup (processSetup.sampleRate, gp.o_l_freq);
		O_LPF_R.setup (processSetup.sampleRate, gp.o_l_freq);
	}

	gp.reset = false;
}

void AudioQAMProcessor:: reset ()
{
	DDS.reset ();
	DDL_L.reset ();
	DDL_R.reset ();
	HT_L.reset ();
	HT_R.reset ();
	I_HPF_L.reset ();
	I_HPF_R.reset ();
	I_LPF_L.reset ();
	I_LPF_R.reset ();
	O_HPF_L.reset ();
	O_HPF_R.reset ();
	O_LPF_L.reset ();
	O_LPF_R.reset ();
	gp.reset = true;
}

//------------------------------------------------------------------------
} // namespace suzumushi
