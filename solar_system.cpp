#include <iostream>
#include <iomanip>
#include <cmath>

const double dt = 10.0f;

const double G = 6.673848E-11;
struct coord
{
  double x, y, z;
};

double dist (coord a, coord b)
{
  return sqrt ( (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) + (a.z - b.z) * (a.z - b.z) );
}

typedef struct coord velocity;

typedef struct coord acceleration;

const coord center = {0, 0, 0};

struct planet
{
  double mass;
  coord position;
  velocity speed;
  const char* name;
};

planet SolarSystem [] = { {1.9891E30, {2.378294701483334E+08, -3.554940854778244E+08, -5.317780295720475E+06}, {1.049384137411123E+01, -3.208975269155431, -2.265299333673586E-01}, "Sun" },
                          {3.3022E23, {4.548763468074692E+10, 1.870856785359237E+10, 2.643117577390042E+09}, {2.820685350474736E+04, 4.712154253557772E+04, 6.439182158884210E+03}, "Mercury" },
                          {4.8680E24, {-9.300901927892263E+10, 5.363380347996668E+10, 6.088588870440500E+09}, {-1.774964829958531E+04, -3.044298930319684E+04, 6.076738453833201E+02}, "Venus" },
                          {5.9746E24, {9.528222122169644E+10, 1.127907384796359E+11, -8.677905781924685E+06}, {-2.324277008699390E+04, 1.911334254009245E+04, -1.819867911761157}, "Earth" },
                          {6.4185E23, {7.335827465318605E+10, -1.992279783380762E+11, -5.979008528975585E+09}, {2.365062032354612E+04, 1.048358210902060E+04, -3.609429262323488E+02},  "Mars" },
                          {1.8986E27, {2.672114642281701E+11, 7.058238524511377E+11, -8.922955139413936E+09}, {-1.237825897071175E+04, 5.250818572797908E+03, 2.551614040829192E+02}, "Jupiter" },
                          {5.6846E26, {-1.229866547680459E+12, -7.904683052447389E+11, 6.268969749973371E+10}, {4.699489217425667E+03, -8.149587596664846E+03, -4.475565267559236E+01}, "Saturn" },
                          {8.6810E25, {2.979208970334734E+12, 3.567811645222515E+11, -3.727175043882546E+10}, {-8.597282987579890E+02, 6.444261662580991E+03, 3.509724280159456E+01}, "Uranus" },
                          {1.0243E26, {3.962453395594141E+12, -2.104152802357125E+12, -4.798646790232673E+10}, {2.513631552931607E+03, 4.833306675848008E+03, -1.572465381335225E+02}, "Neptune" },
                          {1.3050E22, {7.404713835027376E+11, -4.770829352954782E+12, 2.963178894495413E+11}, {5.467423910048135E+03, -2.437018535334458E+02, -1.534466044015474E+03}, "Pluto" }
};

int main()
{
  std::cout.precision(12);
  int counter = 0;
  int year = 2012;
  while(1)
  {
    for (int i = 0; i < 10; i++)
    {
      acceleration delta = {0, 0, 0};
      for (int j = 0; j < 10; j++)
      {
        if (i == j)
        {
          continue;
        }
        double distance = dist (SolarSystem[i].position, SolarSystem[j].position);
        double normal_acc = G * SolarSystem[j].mass / (distance * distance);
        delta.x += normal_acc * (SolarSystem[j].position.x - SolarSystem[i].position.x) / distance;
        delta.y += normal_acc * (SolarSystem[j].position.y - SolarSystem[i].position.y) / distance;
        delta.z += normal_acc * (SolarSystem[j].position.z - SolarSystem[i].position.z) / distance;
      }
      SolarSystem[i].speed.x += delta.x * dt;
      SolarSystem[i].speed.y += delta.y * dt;
      SolarSystem[i].speed.z += delta.z * dt;

      SolarSystem[i].position.x += SolarSystem[i].speed.x * dt + delta.x * dt * dt;
      SolarSystem[i].position.y += SolarSystem[i].speed.y * dt + delta.y * dt * dt;
      SolarSystem[i].position.z += SolarSystem[i].speed.z * dt + delta.z * dt * dt;

      if (counter == 0)
      {

        std::cout << std::setw(12) << "Year " << year
                  << std::setw(10) << SolarSystem[i].name 
                  << std::setw(21) << std::scientific << SolarSystem[i].position.x 
                  << std::setw(21) << std::scientific << SolarSystem[i].position.y 
                  << std::setw(21) << std::scientific << SolarSystem[i].position.z 
                  << std::setw(21) << std::scientific << dist(SolarSystem[i].position, center) 
                  << std::endl; 
      }
    }
    year = (counter>=86400*365/dt) ? ++year : year;
    counter = (counter>=86400*365/dt) ? 0 : ++counter;
  }
  return 0;
}
