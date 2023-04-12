//
// Copyright (c) 2023 suzumushi
//
// 2023-4-6		AQHilbert.h
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

// FIR filter-based Hilbert transformer

template <typename TYPE, 
	int IR_LEN = 259,							// Logical length of impulse response. (IR_LEN - 1) / 2 must be an odd number.
	int IR_CENTER = (IR_LEN - 1) / 2,			// Center of impulse response (don't touch this)
	int IR_TBL_LEN = (IR_LEN + 1) / 4>			// Length of impulse response table (don't touch this)
class AQHilbert {
public:
	AQHilbert ();
	void process (const TYPE xn, TYPE &yn, TYPE &yHn);
	void reset ();
private:
	static TYPE IR_TBL [IR_TBL_LEN];			// Impulse response table
	SODDL <TYPE, IR_LEN> IDL;					// Input delay line
};

template <typename TYPE, int IR_LEN, int IR_CENTER, int IR_TBL_LEN>
TYPE
AQHilbert <TYPE, IR_LEN, IR_CENTER, IR_TBL_LEN>:: 
IR_TBL [IR_TBL_LEN];

template <typename TYPE, int IR_LEN, int IR_CENTER, int IR_TBL_LEN>
AQHilbert <TYPE, IR_LEN, IR_CENTER, IR_TBL_LEN>:: 
AQHilbert ()
{	
	if (IR_TBL [IR_TBL_LEN - 1] == 0.0) {
		for (int i = 0; i < IR_CENTER; i += 2)
			IR_TBL [i / 2] = 2.0 / (pi * (i - IR_CENTER));
		// Blackman window
		for (int i = 0; i < IR_CENTER; i += 2)
			IR_TBL [i / 2] *= 0.42 - 0.5 * cos (pi * i / IR_CENTER) + 0.08 * cos (2.0 * pi * i / IR_CENTER);
	}
}

template <typename TYPE, int IR_LEN, int IR_CENTER, int IR_TBL_LEN>
void
AQHilbert <TYPE, IR_LEN, IR_CENTER, IR_TBL_LEN>:: 
process (const TYPE xn, TYPE &yn, TYPE &yHn)
{
	IDL.enqueue (xn);
	yn = IDL.read (IR_CENTER);
	yHn = 0.0;
	for (int i = 0, j = IR_LEN - 1; i < IR_CENTER; i += 2, j -= 2)
		yHn += IR_TBL [i / 2] * (IDL.read (j) - IDL.read (i));
}

template <typename TYPE, int IR_LEN, int IR_CENTER, int IR_TBL_LEN>
void
AQHilbert <TYPE, IR_LEN, IR_CENTER, IR_TBL_LEN>:: 
reset ()
{
	IDL.reset ();
}

} // namespace suzumushi

