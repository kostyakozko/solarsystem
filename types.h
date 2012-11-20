#ifndef __TYPES_H__
#define __TYPES_H__
#include <valarray>
struct coord
{
  long double x, y, z;
};

typedef coord velocity;

typedef coord acceleration;

struct planet
{
  long double mass;
  coord position;
  velocity speed;
  const char* name;
};

#endif //__TYPES_H__
