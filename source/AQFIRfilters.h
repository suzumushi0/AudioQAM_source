//
// Copyright (c) 2023 suzumushi
//
// 2023-5-11		AQFIRfilters.h
//
// Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 (CC BY-NC-SA 4.0).
//
// https://creativecommons.org/licenses/by-nc-sa/4.0/
//

#pragma once

#include "SODDL.h"
#ifdef _MSC_VER			// Visual C++
#include <numbers>
using std::numbers::pi;
#endif


namespace suzumushi {

// linear phase FIR LPF and HPF

template <typename TYPE, 
	int IR_LEN = 67,							// Logical length of impulse response. IR_LEN must be an odd number.
	bool LPF = true,							// set true for LPF and false for HPF
	TYPE FC_MAX = 20'000.0,						// pass through frequency (LPF only)
	int IR_CENTER = (IR_LEN - 1) / 2>			// Center of impulse response (don't touch this)
class AQFIRfilters {
public:
	void setup (const TYPE SR, const TYPE fc);
	TYPE process (const TYPE xn);
	void reset ();
private:
	TYPE IR_TBL [IR_CENTER + 1];				// Impulse response table
	SODDL <TYPE, IR_LEN> IDL;					// Input delay line
	bool pass_through {false};					// pass through mode
};

template <typename TYPE, int IR_LEN, bool LPF, TYPE FC_MAX, int IR_CENTER>
void
AQFIRfilters <TYPE, IR_LEN, LPF, FC_MAX, IR_CENTER>:: 
setup (const TYPE SR, const TYPE fc)
{	
	if (fc < FC_MAX || !LPF) {
		// sinc function 
		IR_TBL [IR_CENTER] = 2.0 * fc / SR;
		TYPE omega_cT = IR_TBL [IR_CENTER] * pi;
		for (int i = 0, j = -IR_CENTER; i < IR_CENTER; i++, j++)
			IR_TBL [i] = sin (j * omega_cT) / (j * pi);

		// hamming window
		for (int i = 0; i <= IR_CENTER; i++)
			IR_TBL [i]  *= 0.54 - 0.46 * cos (pi * i / IR_CENTER);

		// normalization
		TYPE sum = 0.0;
		for (int i = 0; i < IR_CENTER; i++)
			sum += IR_TBL [i];
		sum = sum * 2.0 + IR_TBL [IR_CENTER];
		for (int i = 0; i <= IR_CENTER; i++)
			IR_TBL [i] /= sum;

		if (!LPF)	// HPF
			IR_TBL [IR_CENTER] = 1.0 - IR_TBL [IR_CENTER];

		pass_through = false;
	} else
		pass_through = true;
}

template <typename TYPE, int IR_LEN, bool LPF, TYPE FC_MAX, int IR_CENTER>
TYPE
AQFIRfilters <TYPE, IR_LEN, LPF, FC_MAX, IR_CENTER>:: 
process (const TYPE xn)
{
	IDL.enqueue (xn);
	if (pass_through)
		return (IDL.read (IR_CENTER));
	else {
		TYPE yn = 0.0;
		for (int i = 0, j = IR_LEN - 1; i < IR_CENTER; i++, j--)
			yn += IR_TBL [i] * (IDL.read (i) + IDL.read (j));
		if (LPF)
			return (yn + IR_TBL [IR_CENTER] * IDL.read (IR_CENTER));
		else // HPF
			return (-yn + IR_TBL [IR_CENTER] * IDL.read (IR_CENTER));
	}
}

template <typename TYPE, int IR_LEN, bool LPF, TYPE FC_MAX, int IR_CENTER>
void
AQFIRfilters <TYPE, IR_LEN, LPF, FC_MAX, IR_CENTER>:: 
reset ()
{
	IDL.reset ();
}

} // namespace suzumushi

