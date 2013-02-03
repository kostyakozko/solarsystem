#ifndef __TYPES_H__
#define __TYPES_H__

/* We make this definition to ensure that the data array will be transferable
 * to OpenCL device. However, double-precision support is "optional" as of
 * OpenCL 1.2 standard, so we must query device capabilities at runtime.
 */
typedef double float_type;

struct coord
{
  float_type x, y, z;
};

typedef coord velocity;

typedef coord acceleration;

struct planet
{
  float_type mass;
  coord position;
  velocity speed;
  const char* name;
};

#endif //__TYPES_H__
