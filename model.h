#ifndef __MODEL_H__
#define __MODEL_H__
#include "types.h"

long double dist (coord a, coord b);

void attractTo(coord& this_position, acceleration& delta, int j);

coord getBarycenter ();

void printBarycenter(const coord& barycenter);

#endif //__MODEL_H__
