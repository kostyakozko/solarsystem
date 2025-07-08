#include "model.h"

#include <cmath>
#include <iomanip>
#include <iostream>

#include "constants.h"

long double dist(const coord& a, const coord& b) {
  return sqrtl((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) + (a.z - b.z) * (a.z - b.z));
}

coord getBarycenter() {
  long double massSum = 0;
  for (int i = 0; i < count; i++) {
    massSum += SolarSystem_[i].mass;
  }
  coord retVal = {0, 0, 0};
  long double invMassSum = 1 / massSum;
  for (int i = 0; i < count; i++) {
    retVal.x += SolarSystem_[i].mass * SolarSystem_[i].position.x;
    retVal.y += SolarSystem_[i].mass * SolarSystem_[i].position.y;
    retVal.z += SolarSystem_[i].mass * SolarSystem_[i].position.z;
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
  const struct tm* timeinfo = localtime(&current);
  char time_buffer[100];
  strftime(time_buffer, sizeof(time_buffer), "%a %b %d %H:%M:%S %Y\n", timeinfo);
  std::cout << time_buffer;
  for (int i = 0; i < count; i++) {
    std::cout << std::setw(15) << SolarSystem_[i].name << std::setw(21) << std::scientific
              << (SolarSystem_[i].position.x - barycenter.x) << std::setw(21) << std::scientific
              << (SolarSystem_[i].position.y - barycenter.y) << std::setw(21) << std::scientific
              << (SolarSystem_[i].position.z - barycenter.z) << std::setw(21) << std::scientific
              << dist(SolarSystem_[i].position, barycenter) << std::setw(21) << std::scientific
              << SolarSystem_[i].speed.x << std::setw(21) << std::scientific
              << SolarSystem_[i].speed.y << std::setw(21) << std::scientific
              << SolarSystem_[i].speed.z << std::endl;
  }
}
// Functions for accessing celestial bodies
int get_body_count() { return count; }

const planet& get_body(int index) {
  if (index < 0 || index >= count) {
    static planet empty_body = {0, {0, 0, 0}, {0, 0, 0}, "Invalid"};
    return empty_body;
  }
  return SolarSystem_[index];
}
