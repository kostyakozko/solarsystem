#include <iostream>
#include <iomanip>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <omp.h>
#include "types.h"
#include "constants.h"
#include "model.h"

int main()
{
  std::cout.precision(12);
  int year = 2012;
  struct tm timeinfo = {0, 0, 0, 12, 10, 2012 - 1900};
  time_t origin;
  time_t current = origin = mktime(&timeinfo);
  time_t step = 86400*15;
  time_t now = time(NULL);
  printBarycenter(getBarycenter());
  //#pragma omp parallel shared (SolarSystem)
  while(1)
  {
    //#pragma omp for
    for (int i = 0; i < count; ++i)
    {
      acceleration delta{0.0, 0.0, 0.0};
      coord& this_position = SolarSystem[i].position;
      //our method
      for (int j = 0; j < i; ++j)
      {
        attractTo(this_position, delta, j);
      }
      for (int j = i + 1; j < count; ++j)
      {
        attractTo(this_position, delta, j);
      }
      velocity& this_speed = SolarSystem[i].speed;
      this_speed.x += delta.x * dt;
      this_speed.y += delta.y * dt;
      this_speed.z += delta.z * dt;

      this_position.x += (this_speed.x - delta.x * dt / 2) * dt;
      this_position.y += (this_speed.y - delta.y * dt / 2) * dt;
      this_position.z += (this_speed.z - delta.z * dt / 2) * dt;
      
    }
    //#pragma omp master
    {
      if (current >= now)//( (current - origin) >=  step)
      { 
        printCurrentData(current);
        origin = current;
        exit(0);
      }
      current += dt;
    }
  }
  return 0;
}
