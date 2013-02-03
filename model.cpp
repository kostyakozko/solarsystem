#include <iostream>
#include <cmath>
#include <cstdio>
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
  catch (std::string e)
  {
    std::cout << "Failed to initialize OpenCL model:\n" << e;
    return std::shared_ptr<Model>(new FallbackModel(_step));
  }
  catch (...)
  {
    std::cout << "Unrecoverable error has occured! Terminating...\n";
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
  // throw std::string("Not implemented yet!\n");

  cl_int error;

  cl_platform_id platform;
  cl_uint numPlatforms = 0;

  // TODO: add support for scanning for multiple installed OpenCL implementations
  error = clGetPlatformIDs(1, &platform, &numPlatforms);
  if ( (error != CL_SUCCESS) || (numPlatforms < 1) )
  {
    throw std::string("Could not find any installed OpenCL implementations!\n");
  }

  cl_device_type device_type;
  if (useAllDevices == true)
  {
    device_type = CL_DEVICE_TYPE_ALL;
  }
  else
  { // Default behavior is to use GPUs only
    device_type = CL_DEVICE_TYPE_GPU;
  }

  // TODO: add support for multiple devices
  const cl_uint maxDevices = 1;
  cl_uint numDevices = 0;
  cl_device_id devices[maxDevices];
  error = clGetDeviceIDs(platform, device_type, maxDevices, devices, &numDevices);
  if ( (error != CL_SUCCESS) || (numDevices < 1) )
  {
    throw std::string("Could not find any OpenCL devices!\n");
  }
  cl_device_fp_config fpConfig;
  for ( int i = 0; i < numDevices; ++i )
  {
    error = clGetDeviceInfo(devices[i], CL_DEVICE_DOUBLE_FP_CONFIG, sizeof(fpConfig), &fpConfig, NULL);
    if ( (error != CL_SUCCESS) || (fpConfig == 0) )
    {
      throw std::string("Detected OpenCL device does not support double-precision!\n");
    }
  }

  context = clCreateContext(NULL, numDevices, devices, NULL, NULL, &error);
  if ( error != CL_SUCCESS )
  {
    throw std::string("Could not create an OpenCL context!\n");
  }

  queue = clCreateCommandQueue(context, devices[0], 0, &error);
  if ( error != CL_SUCCESS )
  {
    clReleaseContext(context);
    throw std::string("Could not create device command queue!\n");
  }

  size_t size = 0;
  char* buffer = readProgram("model_kernel.cl", size);
  if ( buffer == NULL )
  {
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    throw std::string("Could not open OpenCL program file for reading!\n");
  }

  program = clCreateProgramWithSource(context, 1, (const char**)&buffer, &size, &error);
  free(buffer);
  if ( error != CL_SUCCESS )
  {
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    throw std::string("Could not create OpenCL program!\n");
  }

  error = clBuildProgram(program, numDevices, devices, NULL, NULL, NULL);
  if ( error != CL_SUCCESS )
  {
    const int logSize = 2000;
    char buildLog[logSize];
    clGetProgramBuildInfo(program, devices[0], CL_PROGRAM_BUILD_LOG, logSize, buildLog, NULL);
    std::string errorMsg(buildLog);
    errorMsg.append("OpenCL program build failed!\n");

    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    throw errorMsg;
  }

  kernel = clCreateKernel(program, "eulerModel", &error);
  if ( error != CL_SUCCESS )
  {
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    throw std::string("Could not create kernel to run on the device!\n");
  }

  deviceBuffer = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                                sizeof(planet)*count, SolarSystem, &error);
  if ( error != CL_SUCCESS )
  {
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    throw std::string("Could not transfer data buffer to the device!\n");
  }
  clSetKernelArg(kernel, 0, sizeof(cl_mem), &deviceBuffer);
  clSetKernelArg(kernel, 1, sizeof(count), &count);
  clSetKernelArg(kernel, 2, sizeof(origin), &origin);
  clSetKernelArg(kernel, 3, sizeof(step), &step);
  clSetKernelArg(kernel, 4, sizeof(dt), &dt);
}

OpenCLModel::~OpenCLModel()
{
  clReleaseMemObject(deviceBuffer);
  clReleaseKernel(kernel);
  clReleaseProgram(program);
  clReleaseCommandQueue(queue);
  clReleaseContext(context);
}

void OpenCLModel::nextStep()
{
  clSetKernelArg(kernel, 2, sizeof(origin), &origin);
  clEnqueueTask(queue, kernel, 0, NULL, &kernelRun);
  origin += step;
}

void OpenCLModel::print()
{
  clEnqueueReadBuffer(queue, deviceBuffer, CL_TRUE, 0, sizeof(planet)*count, SolarSystem, 1, &kernelRun, NULL);
  Model::print();
}

char* OpenCLModel::readProgram(const char* filename, size_t& size)
{
  FILE *file = fopen(filename, "r");
  if (file == NULL)
  {
    return NULL;
  }
  fseek(file, 0, SEEK_END);
  size = ftell(file);
  rewind(file);
  char* buffer = (char*)malloc(size + 1);
  buffer[size] = '\0';
  fread(buffer, sizeof(char), size, file);
  fclose(file);

  return buffer;
}
