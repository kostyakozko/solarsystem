/* This struct must perfectly match the host structure */
struct planet
{
  double mass;
  double3 position;
  double3 speed;
  intptr_t name;
};

__constant double G = 6.673848E-11;

double dist (double3 a, double3 b)
{
  double3 diff = a - b;
  return sqrt ( diff.x * diff.x + diff.y * diff.y + diff.z * diff.z );
}

static void attractTo(double3 this_position, double3 other_position, double other_mass, double3 *delta)
{
  double distance = dist (this_position, other_position);
  double normal_acc = G * other_mass / (distance * distance * distance); // division takes a lot of time so we fuck the physics laws to boost speed
  *delta += (other_position - this_position) * normal_acc;
}


__kernel void eulerModel( __global struct planet* SolarSystem,
                          int count,
                          unsigned long origin,
                          unsigned long step,
                          int dt )
{
  unsigned long current = origin;
  while ( (current - origin) < step)
  {
    for (int i = 0; i < count; ++i)
    {
      double3 delta = (double3)(0.0, 0.0, 0.0);
      double3 this_position = SolarSystem[i].position;
      // a simple Euler method
      for (int j = 0; j < i; ++j)
      {
        attractTo(this_position, SolarSystem[j].position, SolarSystem[j].mass, &delta);
      }
      for (int j = i + 1; j < count; ++j)
      {
        attractTo(this_position, SolarSystem[j].position, SolarSystem[j].mass, &delta);
      }
      SolarSystem[i].speed += delta * dt;
      SolarSystem[i].position += (SolarSystem[i].speed - delta * dt / 2) * dt;
    }
    current += dt;
  }
}
