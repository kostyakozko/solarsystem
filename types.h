#ifndef __TYPES_H__
#define __TYPES_H__
struct coord
{
  long double x, y, z;
};

typedef struct coord velocity;

typedef struct coord acceleration;

struct planet
{
  long double mass;
  coord position;
  velocity speed;
  const char* name;
};

#endif //__TYPES_H__
