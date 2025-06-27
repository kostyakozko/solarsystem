#ifndef __MODEL_H__
#define __MODEL_H__
#include <time.h>
#include <cmath>
#include "types.h"
#include "constants.h"

long double dist (coord a, coord b);

inline void attractTo(coord& this_position, acceleration& delta, int j)
{
  coord& other_position = SolarSystem[j].position;
  long double distance = dist (this_position, other_position);
  long double normal_acc = G * SolarSystem[j].mass / (distance * distance * distance); // division takes a lot of time so we fuck the physics laws to boost speed
  delta.x += normal_acc * (other_position.x - this_position.x);
  delta.y += normal_acc * (other_position.y - this_position.y);
  delta.z += normal_acc * (other_position.z - this_position.z);
}

coord getBarycenter ();

void printBarycenter(const coord& barycenter);

void printCurrentData(time_t current);
#endif //__MODEL_H__
