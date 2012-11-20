#ifndef __MODEL_H__
#define __MODEL_H__
#include <time.h>
#include "types.h"

long double dist (coord a, coord b);

void attractTo(coord& this_position, acceleration& delta, int j);

coord getBarycenter ();

void printBarycenter(const coord& barycenter);

void printCurrentData(time_t current);
#endif //__MODEL_H__
