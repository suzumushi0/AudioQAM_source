//
// Copyright (c) 2023 suzumushi
//
// 2023-3-30		AQDDS.h
//
// Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 (CC BY-NC-SA 4.0).
//
// https://creativecommons.org/licenses/by-nc-sa/4.0/
//

#pragma once

#ifdef _MSC_VER			// Visual C++
#include <numbers>
using std::numbers::pi;
#endif

#include "AQparam.h"

namespace suzumushi {

// Direct Digital Synthesizer with pi/2 phase lag output

template <typename TYPE, 
	int WT_LEN = 18'000,						// Logical length of a wave table
	int Q_WT_LEN = WT_LEN / 4,					// quadrant	(don't touch this)
	int S_WT_LEN = WT_LEN / 2,					// semi (don't touch this)
	int D_WT_LEN = WT_LEN * 3 / 4>				// dodrant (don't touch this)
class AQDDS {
public:
	AQDDS ();
	void setup (const double samplingRate, const TYPE frequency);
	void process (const int waveform, TYPE &yn, TYPE &yHn);
	void reset ();
private:
	TYPE wave_lookup (const int waveform) const;
	TYPE lagged_wave_lookup (const int waveform) const;
	static TYPE SIN_TBL [Q_WT_LEN + 1];			// sine wave table
	static TYPE TRI_TBL [Q_WT_LEN + 1];			// Hilbert transformed triangle wave table	
	static TYPE SQU_TBL [Q_WT_LEN + 1];			// Hilbert transformed square wave table
	static TYPE SAW_TBL [S_WT_LEN + 1];			// Hilbert transformed sawtooth wave table
	int phase {0};								// phase
	int SR {0};									// sampling rate
	int T {0};									// T = int (N/SR) where N = frequency * WT_LEN
	int phase_error {0};						// 2SR * phase error 
	int phase_error_diff0 {0};					// 2SR * phase error difference for T
	int phase_error_diff1 {0};					// 2SR * phase error difference for T + 1							
};

template <typename TYPE, int WT_LEN, int Q_WT_LEN, int S_WT_LEN, int D_WT_LEN>
TYPE 
AQDDS <TYPE, WT_LEN, Q_WT_LEN, S_WT_LEN, D_WT_LEN>:: 
SIN_TBL [Q_WT_LEN + 1];

template <typename TYPE, int WT_LEN, int Q_WT_LEN, int S_WT_LEN, int D_WT_LEN>
TYPE 
AQDDS <TYPE, WT_LEN, Q_WT_LEN, S_WT_LEN, D_WT_LEN>:: 
TRI_TBL [Q_WT_LEN + 1];

template <typename TYPE, int WT_LEN, int Q_WT_LEN, int S_WT_LEN, int D_WT_LEN>
TYPE
AQDDS <TYPE, WT_LEN, Q_WT_LEN, S_WT_LEN, D_WT_LEN>:: 
SQU_TBL [Q_WT_LEN + 1];

template <typename TYPE, int WT_LEN, int Q_WT_LEN, int S_WT_LEN, int D_WT_LEN>
TYPE
AQDDS <TYPE, WT_LEN, Q_WT_LEN, S_WT_LEN, D_WT_LEN>:: 
SAW_TBL [S_WT_LEN + 1];


template <typename TYPE, int WT_LEN, int Q_WT_LEN, int S_WT_LEN, int D_WT_LEN>
AQDDS <TYPE, WT_LEN, Q_WT_LEN, S_WT_LEN, D_WT_LEN>:: 
AQDDS ()
{
	if (SIN_TBL [Q_WT_LEN] == 0.0) {
		// sine wave table
		for (int i = 0; i <= Q_WT_LEN; i++)
			SIN_TBL [i] = sin (pi * i / S_WT_LEN);
		for (int i = 0; i <= Q_WT_LEN; i++)
			SIN_TBL [i] *= sqrt (2.0);

		// Hilbert transformed square wave table
		for (int i = 1; i <= Q_WT_LEN; i++)
			SQU_TBL [i] = -2.0 / pi * log (abs (1.0 / tan (pi * i / WT_LEN)));
		SQU_TBL [0] = SQU_TBL [1];

		// Hilbert transformed triangle wave table
		TRI_TBL [0] = 0.0;
		for (int i = 1; i <= Q_WT_LEN; i++)
			TRI_TBL [i] = TRI_TBL [i - 1] - SQU_TBL [i - 1] / Q_WT_LEN;
		for (int i = 0; i <= Q_WT_LEN; i++)
			TRI_TBL [i] *= sqrt (3.0);

		// Hilbert transformed square wave table (cont'd)
		for (int i = 0; i <= Q_WT_LEN; i++)
			SQU_TBL [i] *= 0.5;

		// Hilbert transformed sawtooth wave table
		for (int i = 0; i < S_WT_LEN; i++)
			SAW_TBL [i] = -2.0 / pi * log (2.0 * cos (pi * i / WT_LEN));
		SAW_TBL [S_WT_LEN] = SAW_TBL [S_WT_LEN - 1];
		for (int i = 0; i <= S_WT_LEN; i++)
			SAW_TBL [i] *= 0.5;
	}
}

template <typename TYPE, int WT_LEN, int Q_WT_LEN, int S_WT_LEN, int D_WT_LEN>
void
AQDDS <TYPE, WT_LEN, Q_WT_LEN, S_WT_LEN, D_WT_LEN>:: 
setup (const double samplingRate, const TYPE frequency)
{
	int N = frequency * WT_LEN + 0.5;
	int M = samplingRate + 0.5;
	T = N / M;
	phase_error_diff0 = 2 * (N - M * T);
	phase_error_diff1 = phase_error_diff0 - 2 * M;
	if (M != SR) {
		SR = M;
		phase = 0;
		phase_error = phase_error_diff0 - M;
	}
}

template <typename TYPE, int WT_LEN, int Q_WT_LEN, int S_WT_LEN, int D_WT_LEN>
void
AQDDS <TYPE, WT_LEN, Q_WT_LEN, S_WT_LEN, D_WT_LEN>:: 
process (const int waveform, TYPE &yn, TYPE &yHn)
{
	yn = wave_lookup (waveform);
	yHn = lagged_wave_lookup (waveform);

	if (phase_error < 0) {
		phase_error += phase_error_diff0;
		if ((phase += T) >= WT_LEN)
			phase -= WT_LEN;
	} else {
		phase_error += phase_error_diff1;
		if ((phase += T + 1) >= WT_LEN)
			phase -= WT_LEN;
	}
}

template <typename TYPE, int WT_LEN, int Q_WT_LEN, int S_WT_LEN, int D_WT_LEN>
TYPE 
AQDDS <TYPE, WT_LEN, Q_WT_LEN, S_WT_LEN, D_WT_LEN>:: 
wave_lookup (const int waveform) const
{
	switch (waveform){
		case (int)WFORM_L::SINE:
			if (phase < Q_WT_LEN) 
				return (SIN_TBL [phase]);
			if (phase < S_WT_LEN)
				return (SIN_TBL [S_WT_LEN - phase]);
			if (phase < D_WT_LEN)
				return (- SIN_TBL [phase - S_WT_LEN]);
			else
				return (- SIN_TBL [WT_LEN - phase]);
			break;
		case (int)WFORM_L::TRIANGLE:
			if (phase < Q_WT_LEN)
				return (sqrt (3.0) * (TYPE)phase / Q_WT_LEN);
			if (phase < D_WT_LEN)
				return (sqrt (3.0) * (TYPE)(S_WT_LEN - phase) / Q_WT_LEN);	
			else 
				return (sqrt (3.0) * (TYPE)(phase - WT_LEN) / Q_WT_LEN);
			break;
		case (int)WFORM_L::SQUARE:
			if (phase < S_WT_LEN)
				return (0.5);
			else
				return (-0.5);
			break;
		case (int)WFORM_L::SAWTOOTH:
			if (phase == S_WT_LEN)
				return (0.0);
			if (phase < S_WT_LEN)
				return (0.5 * (TYPE)phase / S_WT_LEN);
			else
				return (0.5 * (TYPE)(phase - WT_LEN) / S_WT_LEN);
			break;
		default:
			return (0.0);
			break;
	}
}

template <typename TYPE, int WT_LEN, int Q_WT_LEN, int S_WT_LEN, int D_WT_LEN>
TYPE
AQDDS <TYPE, WT_LEN, Q_WT_LEN, S_WT_LEN, D_WT_LEN>:: 
lagged_wave_lookup (const int waveform) const
{
	switch (waveform){
		case (int)WFORM_L::SINE:
			if (phase < Q_WT_LEN) 
				return (- SIN_TBL [Q_WT_LEN - phase]);
			if (phase < S_WT_LEN)
				return (SIN_TBL [phase - Q_WT_LEN]);
			if (phase < D_WT_LEN)
				return (SIN_TBL [D_WT_LEN - phase]);
			else
				return (- SIN_TBL [phase - D_WT_LEN]);
			break;
		case (int)WFORM_L::TRIANGLE:
			if (phase < Q_WT_LEN) 
				return (- TRI_TBL [Q_WT_LEN - phase]);
			if (phase < S_WT_LEN)
				return (TRI_TBL [phase - Q_WT_LEN]);
			if (phase < D_WT_LEN)
				return (TRI_TBL [D_WT_LEN - phase]);
			else
				return (- TRI_TBL [phase - D_WT_LEN]);
			break;
		case (int)WFORM_L::SQUARE:
			if (phase < Q_WT_LEN) 
				return (SQU_TBL [phase]);
			if (phase < S_WT_LEN)
				return (- SQU_TBL [S_WT_LEN - phase]);
			if (phase < D_WT_LEN)
				return (- SQU_TBL [phase - S_WT_LEN]);
			else
				return (SQU_TBL [WT_LEN - phase]);
			break;
		case (int)WFORM_L::SAWTOOTH:
			if (phase < S_WT_LEN)
				return (SAW_TBL [phase]);
			else
				return (SAW_TBL [WT_LEN - phase]);
			break;
		default:
			return (0.0);
			break;
	}
}

template <typename TYPE, int WT_LEN, int Q_WT_LEN, int S_WT_LEN, int D_WT_LEN>
void
AQDDS <TYPE, WT_LEN, Q_WT_LEN, S_WT_LEN, D_WT_LEN>:: 
reset ()
{
	SR = 0;
}

} // namespace suzumushi

