//
// Copyright (c) 2023 suzumushi
//
// 2023-5-10		AQparam.h
//
// Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 (CC BY-NC-SA 4.0).
//
// https://creativecommons.org/licenses/by-nc-sa/4.0/
//

#pragma once

#include "SOextparam.h"


namespace suzumushi {

// definitions of GUI and host facing parameter tag

constexpr ParamID C_FREQ {0};			// carrier wave frequency [Hz]
constexpr ParamID WFORM {1};			// carrier waveform
constexpr ParamID AUTO_BL {2};			// automatic input band-limiting 
constexpr ParamID C_SLIDE {3};			// slider position
constexpr ParamID C_RANGE {4};			// slider range
constexpr ParamID C_SCALE {5};			// slider scale
constexpr ParamID I_H_FREQ {10};		// input HPF cutoff frequency [Hz]
constexpr ParamID I_L_FREQ {12};		// input LPF cutoff frequency [Hz]
constexpr ParamID O_H_FREQ {14};		// output HPF cutoff frequency [Hz]
constexpr ParamID O_L_FREQ {16};		// output LPF cutoff frequency [Hz]
constexpr ParamID WET {18};				// wet/dry
constexpr ParamID BYPASS {255};			// bypass flag

// attributes of GUI and host facing parameter

constexpr struct rangeParameter c_freq = {
	C_FREQ,								// tag
	{-3'200.0},							// min
	{3'300.0},							// max
	{0.0},								// default
	{0},								// continuous
	{ParameterInfo::kCanAutomate}		// flags
};

constexpr struct stringListParameter wform = {
	WFORM,								// tag
	{ParameterInfo::kIsList | ParameterInfo::kCanAutomate}	// flags
};
enum class WFORM_L {
	SINE,
	TRIANGLE,
	SQUARE,
	SAWTOOTH,
	LIST_LEN
};

constexpr struct stringListParameter auto_bl = {
	AUTO_BL,							// tag
	{ParameterInfo::kIsList | ParameterInfo::kCanAutomate}	// flags
};
enum class AUTO_BL_L {
	MANUAL,
	AUTOMATIC,
	LIST_LEN
};

constexpr struct rangeParameter c_slide = {
	C_SLIDE,							// tag
	{-1.0},								// min
	{1.0},								// max
	{0.0},								// default
	{0},								// continuous
	{ParameterInfo::kCanAutomate}		// flags
};

constexpr struct stringListParameter c_range = {
	C_RANGE,							// tag
	{ParameterInfo::kIsList | ParameterInfo::kCanAutomate}	// flags
};
enum class C_RANGE_L {
	R50,
	R100,
	R200,
	R400,
	R800,
	R1600,
	R3200,
	LIST_LEN
};
constexpr double c_range_val [(unsigned int)C_RANGE_L::LIST_LEN]  = {
	50.0,
	100.0,
	200.0,
	400.0,
	800.0,
	1'600.0,
	3'200.0
};

constexpr struct stringListParameter c_scale = {
	C_SCALE,							// tag
	{ParameterInfo::kIsList | ParameterInfo::kCanAutomate}	// flags
};
enum class C_SCALE_L {
	LINEAR,
	LOG,
	LIST_LEN
};

constexpr struct logTaperParameter i_h_freq = {
	I_H_FREQ,							// tag
	{20.0},								// min
	{5'000.0},							// max
	{200.0},							// default
	{0},								// continuous
	{ParameterInfo::kCanAutomate}		// flags
};

constexpr struct infLogTaperParameter i_l_freq = {
	I_L_FREQ,							// tag
	{50.0},								// min
	{20'000.0},							// max
	{20'000.0},							// default
	{0},								// continuous
	{ParameterInfo::kCanAutomate}		// flags
};

constexpr struct logTaperParameter o_h_freq = {
	O_H_FREQ,							// tag
	{20.0},								// min
	{5'000.0},							// max
	{20.0},								// default
	{0},								// continuous
	{ParameterInfo::kCanAutomate}		// flags
};

constexpr struct infLogTaperParameter o_l_freq = {
	O_L_FREQ,							// tag
	{50.0},								// min
	{20'000.0},							// max
	{20'000.0},							// default
	{0},								// continuous
	{ParameterInfo::kCanAutomate}		// flags
};

constexpr struct rangeParameter wet = {
	WET,								// tag
	{0.0},								// min
	{1.0},								// max
	{1.0},								// default
	{0},								// continuous
	{ParameterInfo::kCanAutomate}		// flags
};

constexpr struct rangeParameter bypass = {
	BYPASS,								// tag
	{0.0},								// min, false
	{1.0},								// max, true
	{0.0},								// default
	{1},								// toggle
	{ParameterInfo::kCanAutomate | ParameterInfo::kIsBypass}	// flags
};

//  GUI and host facing parameters in processor class

struct GUI_param {
	ParamValue c_freq;
	bool c_freq_changed;
	bool c_sb_switching;		// for side band switching noise reduction
	int32 wform;
	int32 auto_bl;
	ParamValue c_slide;
	bool c_slide_changed;
	int32 c_range;
	bool c_range_changed;
	int32 c_scale;
	bool c_scale_changed;
	ParamValue i_h_freq;
	bool i_h_freq_changed;
	ParamValue i_l_freq;
	bool i_l_freq_changed;
	ParamValue o_h_freq;
	bool o_h_freq_changed;
	ParamValue o_l_freq;
	bool o_l_freq_changed;
	ParamValue wet;
	ParamValue dry;
	int32 bypass;
	bool reset;
	bool load;		// for setState ()
	GUI_param () {
		c_freq = suzumushi::c_freq.def;
		c_freq_changed = false;
		c_sb_switching = false;
		wform = (int32) WFORM_L::SINE;
		auto_bl = (int32) AUTO_BL_L::AUTOMATIC;
		c_slide = suzumushi::c_slide.def;
		c_slide_changed = false;
		c_range = (int32) C_RANGE_L::R3200;
		c_range_changed = false;
		c_scale = (int32) C_SCALE_L::LINEAR;
		c_scale_changed = false;
		i_h_freq = suzumushi::i_h_freq.def;
		i_h_freq_changed = false;
		i_l_freq = suzumushi::i_l_freq.def;
		i_l_freq_changed = false;
		o_h_freq = suzumushi::o_h_freq.def;
		o_h_freq_changed = false;
		o_l_freq = suzumushi::o_l_freq.def;
		o_l_freq_changed = false;
		wet = suzumushi::wet.def;
		dry = 1.0 - suzumushi::wet.def;
		bypass = suzumushi::bypass.def;
		reset = true;
		load = false;
	}
};

} // namespace suzumushi

