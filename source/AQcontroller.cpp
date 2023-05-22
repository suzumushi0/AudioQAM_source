//
// Copyright (c) 2023 suzumushi
//
// 2023-5-8		AQcontroller.cpp
//
// Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 (CC BY-NC-SA 4.0).
//
// https://creativecommons.org/licenses/by-nc-sa/4.0/
//

#include "AQcontroller.h"
#include "AQcids.h"
#include "vstgui/plugin-bindings/vst3editor.h"

// suzumushi:
#include "base/source/fstreamer.h"

using namespace Steinberg;

namespace suzumushi {

//------------------------------------------------------------------------
// AudioQAMController Implementation
//------------------------------------------------------------------------
tresult PLUGIN_API AudioQAMController:: initialize (FUnknown* context)
{
	// Here the Plug-in will be instantiated

	//---do not forget to call parent ------
	tresult result = EditControllerEx1:: initialize (context);
	if (result != kResultOk) {
		return result;
	}

	// Here you could register some parameters

	// suzumushi: registration of GUI and host facing parameters 

	Vst::RangeParameter* c_freq_param = new Vst::RangeParameter (
		STR16 ("Carrier wave frequency"), c_freq.tag, STR16 ("Hz"),
		c_freq.min, c_freq.max, c_freq.def, c_freq.steps, c_freq.flags);
	c_freq_param -> setPrecision (precision2);
	parameters.addParameter (c_freq_param);

	Vst::StringListParameter* wform_param = new Vst::StringListParameter (
		STR16 ("Carrier waveform"), wform.tag, nullptr, wform.flags);
	wform_param -> appendString (STR16 ("Sine"));
	wform_param -> appendString (STR16 ("Triangle"));
	wform_param -> appendString (STR16 ("Square"));
	wform_param -> appendString (STR16 ("Sawtooth"));
	parameters.addParameter (wform_param);

	Vst::StringListParameter* auto_bl_param = new Vst::StringListParameter (
		STR16 ("Input band-limiting"), auto_bl.tag, nullptr, auto_bl.flags);
	auto_bl_param -> appendString (STR16 ("Manual"));
	auto_bl_param -> appendString (STR16 ("Automatic"));
	parameters.addParameter (auto_bl_param);

	Vst::RangeParameter* c_slide_param = new Vst::RangeParameter (
		STR16 ("Slider position"), c_slide.tag, STR16 (""),
		c_slide.min, c_slide.max, c_slide.def, c_slide.steps, c_slide.flags);
	parameters.addParameter (c_slide_param);

	Vst::StringListParameter* c_range_param = new Vst::StringListParameter (
		STR16 ("Slider range"), c_range.tag, nullptr, c_range.flags);
	c_range_param -> appendString (STR16 ("\xB1 50 Hz"));		// U+00B1: +/-
	c_range_param -> appendString (STR16 ("\xB1 100 Hz"));
	c_range_param -> appendString (STR16 ("\xB1 200 Hz"));
	c_range_param -> appendString (STR16 ("\xB1 400 Hz"));
	c_range_param -> appendString (STR16 ("\xB1 800 Hz"));
	c_range_param -> appendString (STR16 ("\xB1 1,600 Hz"));
	c_range_param -> appendString (STR16 ("\xB1 3,200 Hz"));
	parameters.addParameter (c_range_param);

	Vst::StringListParameter* c_scale_param = new Vst::StringListParameter (
		STR16 ("Slider scale"), c_scale.tag, nullptr, c_scale.flags);
	c_scale_param -> appendString (STR16 ("Linear"));
	c_scale_param -> appendString (STR16 ("Logarithmic"));
	parameters.addParameter (c_scale_param);

	Vst::LogTaperParameter* i_h_freq_param = new Vst::LogTaperParameter (
		STR16 ("Input HPF cutoff frequency"), i_h_freq.tag, STR16 ("Hz"),
		i_h_freq.min, i_h_freq.max, i_h_freq.def, i_h_freq.steps, i_h_freq.flags);
	i_h_freq_param -> setPrecision (precision0);
	parameters.addParameter (i_h_freq_param);

	Vst::InfLogTaperParameter* i_l_freq_param = new Vst::InfLogTaperParameter (
		STR16 ("Input LPF cutoff frequency"), i_l_freq.tag, STR16 ("Hz"),
		i_l_freq.min, i_l_freq.max, i_l_freq.def, i_l_freq.steps, i_l_freq.flags);
	i_l_freq_param -> setPrecision (precision0);
	parameters.addParameter (i_l_freq_param);

	Vst::LogTaperParameter* o_h_freq_param = new Vst::LogTaperParameter (
		STR16 ("Output HPF cutoff frequency"), o_h_freq.tag, STR16 ("Hz"),
		o_h_freq.min, o_h_freq.max, o_h_freq.def, o_h_freq.steps, o_h_freq.flags);
	o_h_freq_param -> setPrecision (precision0);
	parameters.addParameter (o_h_freq_param);

	Vst::InfLogTaperParameter* o_l_freq_param = new Vst::InfLogTaperParameter (
		STR16 ("Output LPF cutoff frequency"), o_l_freq.tag, STR16 ("Hz"),
		o_l_freq.min, o_l_freq.max, o_l_freq.def, o_l_freq.steps, o_l_freq.flags);
	o_l_freq_param -> setPrecision (precision0);
	parameters.addParameter (o_l_freq_param);

	Vst::RangeParameter* wet_param = new Vst::RangeParameter (
		STR16 ("wet/dry"), wet.tag, STR16 (""),
		wet.min, wet.max, wet.def, wet.steps, wet.flags);
	wet_param -> setPrecision (precision2);
	parameters.addParameter (wet_param);

	Vst::RangeParameter* bypass_param = new Vst::RangeParameter (
		STR16 ("Bypass"), bypass.tag, nullptr,
		bypass.min, bypass.max, bypass.def, bypass.steps, bypass.flags);
	parameters.addParameter (bypass_param);

	return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API AudioQAMController:: terminate ()
{
	// Here the Plug-in will be de-instantiated, last possibility to remove some memory!

	//---do not forget to call parent ------
	return EditControllerEx1::terminate ();
}

//------------------------------------------------------------------------
tresult PLUGIN_API AudioQAMController:: setComponentState (IBStream* state)
{
	// Here you get the state of the component (Processor part)
	if (!state)
		return kResultFalse;

	// suzumushi:
	ParamValue dtmp;
	int32 itmp;
	int32 version;
	IBStreamer streamer (state, kLittleEndian);

	if (streamer.readInt32 (version) == false)
		return (kResultFalse);

	if (streamer.readDouble (dtmp) == false)
		return (kResultFalse);
	setParamNormalized (c_freq.tag, plainParamToNormalized (c_freq.tag, dtmp));

	if (streamer.readInt32 (itmp) == false)
		return (kResultFalse);
	setParamNormalized (wform.tag, plainParamToNormalized (wform.tag, (ParamValue)itmp));

	if (version == 0)
		itmp = (int32) AUTO_BL_L::AUTOMATIC;
	else {
		if (streamer.readInt32 (itmp) == false)
			return (kResultFalse);
	}
	setParamNormalized (auto_bl.tag, plainParamToNormalized (auto_bl.tag, (ParamValue)itmp));

	if (streamer.readDouble (dtmp) == false)
		return (kResultFalse);
	setParamNormalized (c_slide.tag, plainParamToNormalized (c_slide.tag, dtmp));

	if (streamer.readInt32 (itmp) == false)
		return (kResultFalse);
	setParamNormalized (c_range.tag, plainParamToNormalized (c_range.tag, (ParamValue)itmp));

	if (streamer.readInt32 (itmp) == false)
		return (kResultFalse);
	setParamNormalized (c_scale.tag, plainParamToNormalized (c_scale.tag, (ParamValue)itmp));

	if (streamer.readDouble (dtmp) == false)
		return (kResultFalse);
	setParamNormalized (i_h_freq.tag, plainParamToNormalized (i_h_freq.tag, dtmp));

	if (streamer.readDouble (dtmp) == false)
		return (kResultFalse);
	setParamNormalized (i_l_freq.tag, plainParamToNormalized (i_l_freq.tag, dtmp));

	if (streamer.readDouble (dtmp) == false)
		return (kResultFalse);
	setParamNormalized (o_h_freq.tag, plainParamToNormalized (o_h_freq.tag, dtmp));

	if (streamer.readDouble (dtmp) == false)
		return (kResultFalse);
	setParamNormalized (o_l_freq.tag, plainParamToNormalized (o_l_freq.tag, dtmp));

	if (streamer.readDouble (dtmp) == false)
		return (kResultFalse);
	setParamNormalized (wet.tag, plainParamToNormalized (wet.tag, dtmp));

	if (streamer.readInt32 (itmp) == false)
		return (kResultFalse);
	setParamNormalized (bypass.tag, plainParamToNormalized (bypass.tag, (ParamValue)itmp));

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API AudioQAMController:: setState (IBStream* state)
{
	// Here you get the state of the controller

	return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API AudioQAMController:: getState (IBStream* state)
{
	// Here you are asked to deliver the state of the controller (if needed)
	// Note: the real state of your plug-in is saved in the processor

	return kResultTrue;
}

//------------------------------------------------------------------------
IPlugView* PLUGIN_API AudioQAMController:: createView (FIDString name)
{
	// Here the Host wants to open your editor (if you have one)
	if (FIDStringsEqual (name, Vst::ViewType::kEditor))
	{
		// create your editor here and return a IPlugView ptr of it
		auto* view = new VSTGUI::VST3Editor (this, "view", "AudioQAM.uidesc");
		return view;
	}
	return nullptr;
}

//------------------------------------------------------------------------
tresult PLUGIN_API AudioQAMController:: setParamNormalized (Vst::ParamID tag, Vst::ParamValue value)
{
	// called by host to update your parameters
	tresult result = EditControllerEx1::setParamNormalized (tag, value);
	return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API AudioQAMController:: getParamStringByValue (Vst::ParamID tag, Vst::ParamValue valueNormalized, Vst::String128 string)
{
	// called by host to get a string for given normalized value of a specific parameter
	// (without having to set the value!)
	return EditControllerEx1::getParamStringByValue (tag, valueNormalized, string);
}

//------------------------------------------------------------------------
tresult PLUGIN_API AudioQAMController:: getParamValueByString (Vst::ParamID tag, Vst::TChar* string, Vst::ParamValue& valueNormalized)
{
	// called by host to get a normalized value from a string representation of a specific parameter
	// (without having to set the value!)
	return EditControllerEx1::getParamValueByString (tag, string, valueNormalized);
}

//------------------------------------------------------------------------
} // namespace suzumushi
