#include <iostream>
#include <iomanip>
#include <cmath>
#include <cstdlib>

const double dt = 30.0;

const double G = 6.673848E-11;
struct coord
{
  double x, y, z;
};

inline double dist (coord a, coord b)
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

planet SolarSystem [] = { {1.9891E30, {-2.378294701483334E+08, -3.554940854778244E+08, -5.317780295720475E+06}, { 1.049384137411123E+01,     -3.208975269155431, -2.265299333673586E-01}, "Sun"              },
                          {3.3022E23, { 4.548763468074692E+10,  1.870856785359237E+10, -2.643117577390042E+09}, {-2.820685350474736E+04,  4.712154253557772E+04,  6.439182158884210E+03}, "Mercury"          },
                          {4.8680E24, {-9.300901927892263E+10,  5.363380347996668E+10,  6.088588870440500E+09}, {-1.774964829958531E+04, -3.044298930319684E+04,  6.076738453833201E+02}, "Venus"            },
                          {5.9746E24, { 9.528222122169644E+10,  1.127907384796359E+11, -8.677905781924685E+06}, {-2.324277008699390E+04,  1.911334254009245E+04,     -1.819867911761157}, "Earth"            },
                          {7.3477E22, { 9.494830658284894E+10,  1.126496389122832E+11, -2.696828838408615E+07}, {-2.277978315705406E+04,  1.813671748076087E+04,  8.455608469344170E+01}, "Moon"             }, 
                          {6.4185E23, { 7.335827465318605E+10, -1.992279783380762E+11, -5.979008528975585E+09}, { 2.365062032354612E+04,  1.048358210902060E+04, -3.609429262323488E+02}, "Mars"             },
                          {1.8986E27, { 2.672114642281701E+11,  7.058238524511377E+11, -8.922955139413936E+09}, {-1.237825897071175E+04,  5.250818572797908E+03,  2.551614040829192E+02}, "Jupiter"          },
                          {8.9319E22, { 2.674909942964225E+11,  7.055063945335917E+11, -8.930504798383487E+09}, { 6.240724750274147E+02,  1.662241581463450E+04,  8.606576118460202E+02}, "Io/JI"            },
                          {4.7998E22, { 2.668838426114181E+11,  7.052364232758741E+11, -8.951665408970252E+09}, {-3.509234130341419E+02, -1.304032744856808E+03,  9.713270923158084E+01}, "Europa/JII"       },
                          {1.4819E23, { 2.667199483411734E+11,  7.067757735666245E+11, -8.893483481040863E+09}, {-2.203337223020963E+04,  2.759948965816665E+02, -5.601755456611733E+01}, "Ganymede/JIII"    },
                          {1.0759E23, { 2.672127658971256E+11,  7.039418464483191E+11, -8.982867468030199E+09}, {-4.176170687280964E+03,  5.312513883900098E+03,  3.673643302101794E+02}, "Callisto/JIV"     },
                          {5.6846E26, {-1.229866547680459E+12, -7.904683052447389E+11,  6.268969749973371E+10}, { 4.699489217425667E+03, -8.149587596664846E+03, -4.475565267559236E+01}, "Saturn"           },
                          {8.6810E25, { 2.979208970334734E+12,  3.567811645222515E+11, -3.727175043882546E+10}, {-8.597282987579890E+02,  6.444261662580991E+03,  3.509724280159456E+01}, "Uranus"           },
                          {1.0243E26, { 3.962453395594141E+12, -2.104152802357125E+12, -4.798646790232673E+10}, { 2.513631552931607E+03,  4.833306675848008E+03, -1.572465381335225E+02}, "Neptune"          },
                          {1.3050E22, { 7.404713835027376E+11, -4.770829352954782E+12,  2.963178894495413E+11}, { 5.467423910048135E+03, -2.437018535334458E+02, -1.534466044015474E+03}, "Pluto"            }
};

inline void attractTo(coord& this_position, acceleration& delta, int j)
{
  coord& other_position = SolarSystem[j].position;
  double distance = dist (this_position, other_position);
  double normal_acc = G * SolarSystem[j].mass / (distance * distance);
  delta.x += normal_acc * (other_position.x - this_position.x) / distance;
  delta.y += normal_acc * (other_position.y - this_position.y) / distance;
  delta.z += normal_acc * (other_position.z - this_position.z) / distance;
}

int main()
{
  std::cout.precision(12);
  int year = 2012;
  int count = sizeof(SolarSystem)/sizeof(planet);
  double yearStep = 86400*365/dt;
  int counter = yearStep;
 while(1)
  {
    for (int i = 0; i < count; ++i)
    {
      acceleration delta = {0, 0, 0};
      coord& this_position = SolarSystem[i].position;
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
    if (counter>=yearStep)
    { 
      counter = 0;
      for (int i = 0; i < count; i++)
      {
        std::cout << std::setw(12) << "Year " << year
                  << std::setw(15) << SolarSystem[i].name
                  << std::setw(21) << std::scientific << SolarSystem[i].position.x
                  << std::setw(21) << std::scientific << SolarSystem[i].position.y
                  << std::setw(21) << std::scientific << SolarSystem[i].position.z
                  << std::setw(21) << std::scientific << dist(SolarSystem[i].position, center)
                  << std::endl;
      }      
      ++year;

    }
    else
    {
      ++counter;
    }
    if (year>2020)
    {
      exit(0);
    }
  }
  return 0;
}
