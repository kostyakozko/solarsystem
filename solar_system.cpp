#include <iostream>
#include <iomanip>
#include <cmath>
#include <cstdlib>
#include <omp.h>
#include "types.h"
#include "constants.h"
#include "model.h"

int main()
{
  std::cout.precision(12);
  int year = 2012;
  long double yearStep = 86400*365/dt;
  int counter = yearStep;
  printBarycenter(getBarycenter());
  #pragma omp parallel
  while(1)
  {
    #pragma omp for
    for (int i = 0; i < count; ++i)
    {
      acceleration delta = {0, 0, 0};
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
    #pragma omp master
    if (counter>=yearStep)
    { 
      counter = 0;
      coord barycenter = getBarycenter();
      printBarycenter (barycenter);
      for (int i = 0; i < count; i++)
      {
        std::cout << std::setw(12) << "Year " << year
                  << std::setw(15) << SolarSystem[i].name
                  << std::setw(21) << std::scientific << (SolarSystem[i].position.x - barycenter.x)
                  << std::setw(21) << std::scientific << (SolarSystem[i].position.y - barycenter.y)
                  << std::setw(21) << std::scientific << (SolarSystem[i].position.z - barycenter.z)
                  << std::setw(21) << std::scientific << dist(SolarSystem[i].position, barycenter)
                  << std::endl;
      }      
      ++year;

    }
    else
    {
      ++counter;
    }
    if (year>2080)
    {
      exit(0);
    }
  }
  return 0;
}
