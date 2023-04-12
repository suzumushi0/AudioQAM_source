//
// Copyright (c) 2023 suzumushi
//
// 2023-3-5		AQcids.h
//
// Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 (CC BY-NC-SA 4.0).
//
// https://creativecommons.org/licenses/by-nc-sa/4.0/
//

#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

namespace suzumushi {
//------------------------------------------------------------------------
static const Steinberg::FUID kAudioQAMProcessorUID (0xABBF37A8, 0x5C5951B5, 0x867058CE, 0x44D5B3BB);
static const Steinberg::FUID kAudioQAMControllerUID (0x18D960CC, 0x92D75444, 0xB2B3284D, 0x34FC6013);

#define AudioQAMVST3Category "Fx"

//------------------------------------------------------------------------
} // namespace suzumushi
