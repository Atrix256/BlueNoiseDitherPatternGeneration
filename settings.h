
#pragma once

#define DETERMINISTIC() true  // if true, will use a hard coded seed for everything, else will randomly generate a seed.

#define THRESHOLD_SAMPLES() 9 // the number of samples for threshold testing. If this is N, then the thresholds used are: 1/(N+1) ... N/(N+1)