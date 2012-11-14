#include <iostream>
#include <cmath>
#include "model.h"
#include "constants.h"

long double dist (coord a, coord b)
{
  return sqrt ( (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) + (a.z - b.z) * (a.z - b.z) );
}

void attractTo(coord& this_position, acceleration& delta, int j)
{
  coord& other_position = SolarSystem[j].position;
  long double distance = dist (this_position, other_position);
  long double normal_acc = G * SolarSystem[j].mass / (distance * distance);
  delta.x += normal_acc * (other_position.x - this_position.x) / distance;
  delta.y += normal_acc * (other_position.y - this_position.y) / distance;
  delta.z += normal_acc * (other_position.z - this_position.z) / distance;
}

coord getBarycenter ()
{
  long double massSum = 0;
  for (int i = 0; i < count; i++)
  {
    massSum += SolarSystem[i].mass;
  }
  coord retVal = {0, 0, 0};
  for (int i = 0; i < count; i++)
  {
    retVal.x += SolarSystem[i].mass * SolarSystem[i].position.x;
    retVal.y += SolarSystem[i].mass * SolarSystem[i].position.y;
    retVal.z += SolarSystem[i].mass * SolarSystem[i].position.z;
  }
  retVal.x /= massSum;
  retVal.y /= massSum;
  retVal.z /= massSum;
  return retVal;
}

void printBarycenter (const coord& barycenter)
{
  std::cout<<"Barycenter: ("<<barycenter.x<<";"<<barycenter.y<<";"<<barycenter.z<<")\n";
}
