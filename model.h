#ifndef __MODEL_REAL_OPT_H__
#define __MODEL_REAL_OPT_H__
#include <time.h>
#include <cmath>
#include "types.h"
#include "constants.h"

long double dist (coord a, coord b);

// Now use sqrtl to match the corrected original precision
inline void attractTo(coord& this_position, acceleration& delta, int j)
{
  coord& other_position = SolarSystem[j].position;
  
  // Calculate difference vector once to eliminate redundant calculations
  long double dx = other_position.x - this_position.x;
  long double dy = other_position.y - this_position.y;
  long double dz = other_position.z - this_position.z;
  
  long double distance = sqrtl(dx*dx + dy*dy + dz*dz);
  long double normal_acc = G * SolarSystem[j].mass / (distance * distance * distance);
  
  delta.x += normal_acc * dx;
  delta.y += normal_acc * dy;
  delta.z += normal_acc * dz;
}

coord getBarycenter();
void printBarycenter(const coord& barycenter);
void printCurrentData(time_t current);
#endif //__MODEL_REAL_OPT_H__
