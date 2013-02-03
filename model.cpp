#include <iostream>
#include <cmath>
#include <iomanip>

#include "model.h"
#include "constants.h"

static float_type dist (coord a, coord b)
{
  return sqrt ( (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) + (a.z - b.z) * (a.z - b.z) );
}

static void attractTo(coord& this_position, acceleration& delta, int j)
{
  coord& other_position = SolarSystem[j].position;
  float_type distance = dist (this_position, other_position);
  float_type normal_acc = G * SolarSystem[j].mass / (distance * distance * distance); // division takes a lot of time so we fuck the physics laws to boost speed
  delta.x += normal_acc * (other_position.x - this_position.x);
  delta.y += normal_acc * (other_position.y - this_position.y);
  delta.z += normal_acc * (other_position.z - this_position.z);
}

static coord getBarycenter ()
{
  float_type massSum = 0;
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

static void printBarycenter (const coord& barycenter)
{
  std::cout<<"Barycenter: ("<<barycenter.x<<";"<<barycenter.y<<";"<<barycenter.z<<")\n";
}

Model::Model(time_t _step)
: step(_step)
{
  std::cout.precision(12);
  struct tm timeinfo = {0, 0, 0, 12, 10, 2012 - 1900};
  current = origin = mktime(&timeinfo);
}

std::shared_ptr<Model> Model::getBestModel(time_t _step)
{
  try
  {
    return std::shared_ptr<Model>(new OpenCLModel(_step));
  }
  catch (...)
  {
    return std::shared_ptr<Model>(new FallbackModel(_step));
  }
}

void Model::print()
{
  coord barycenter = getBarycenter();
  printBarycenter (barycenter);
  struct tm * timeinfo = localtime(&current);
  std::cout << std::endl << std::string(162, '=') << std::endl;
  std::cout << asctime (timeinfo) << std::endl;
  for (int i = 0; i < count; i++)
  {
    std::cout << std::setw(15) << SolarSystem[i].name
              << std::setw(21) << std::scientific << (SolarSystem[i].position.x - barycenter.x)
              << std::setw(21) << std::scientific << (SolarSystem[i].position.y - barycenter.y)
              << std::setw(21) << std::scientific << (SolarSystem[i].position.z - barycenter.z)
              << std::setw(21) << std::scientific << dist(SolarSystem[i].position, barycenter)
              << std::setw(21) << std::scientific << SolarSystem[i].speed.x
              << std::setw(21) << std::scientific << SolarSystem[i].speed.y
              << std::setw(21) << std::scientific << SolarSystem[i].speed.z
              << std::endl;
  }
}

void FallbackModel::nextStep()
{
  while ( (current - origin) < step)
  {
    for (int i = 0; i < count; ++i)
    {
      acceleration delta{0.0, 0.0, 0.0};
      coord& this_position = SolarSystem[i].position;
      // a simple Euler method
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
    current += dt;
  }
  origin = current;
}

OpenCLModel::OpenCLModel(time_t _step, bool useAllDevices)
: Model(_step)
{
  throw std::string("Not implemented yet!\n");
}

OpenCLModel::~OpenCLModel()
{
}

void OpenCLModel::nextStep()
{
}
