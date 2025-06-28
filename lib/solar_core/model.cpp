#include "model.h"

#include <cmath>
#include <iomanip>
#include <iostream>

#include "constants.h"

long double dist(coord a, coord b) {
  return sqrtl((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) + (a.z - b.z) * (a.z - b.z));
}

coord getBarycenter() {
  long double massSum = 0;
  for (int i = 0; i < count; i++) {
    massSum += SolarSystem[i].mass;
  }
  coord retVal = {0, 0, 0};
  long double invMassSum = 1 / massSum;
  for (int i = 0; i < count; i++) {
    retVal.x += SolarSystem[i].mass * SolarSystem[i].position.x;
    retVal.y += SolarSystem[i].mass * SolarSystem[i].position.y;
    retVal.z += SolarSystem[i].mass * SolarSystem[i].position.z;
  }
  retVal.x *= invMassSum;
  retVal.y *= invMassSum;
  retVal.z *= invMassSum;
  return retVal;
}

void printBarycenter(const coord& barycenter) {
  std::cout << "Barycenter: (" << barycenter.x << ";" << barycenter.y << ";" << barycenter.z << ";"
            << dist(barycenter, {0, 0, 0}) << ")\n";
}

void printCurrentData(time_t current) {
  coord barycenter = getBarycenter();
  printBarycenter(barycenter);
  struct tm* timeinfo = localtime(&current);
  std::cout << asctime(timeinfo) << std::endl;
  for (int i = 0; i < count; i++) {
    std::cout << std::setw(15) << SolarSystem[i].name << std::setw(21) << std::scientific
              << (SolarSystem[i].position.x - barycenter.x) << std::setw(21) << std::scientific
              << (SolarSystem[i].position.y - barycenter.y) << std::setw(21) << std::scientific
              << (SolarSystem[i].position.z - barycenter.z) << std::setw(21) << std::scientific
              << dist(SolarSystem[i].position, barycenter) << std::setw(21) << std::scientific
              << SolarSystem[i].speed.x << std::setw(21) << std::scientific
              << SolarSystem[i].speed.y << std::setw(21) << std::scientific
              << SolarSystem[i].speed.z << std::endl;
  }
}
// Functions for accessing celestial bodies
int get_body_count() { return count; }

const planet& get_body(int index) {
  if (index < 0 || index >= count) {
    static planet empty_body = {0, {0, 0, 0}, {0, 0, 0}, "Invalid"};
    return empty_body;
  }
  return SolarSystem[index];
}
