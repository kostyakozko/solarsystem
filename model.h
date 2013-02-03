#ifndef __MODEL_H__
#define __MODEL_H__

#include <ctime>
#include <memory>

#ifndef MAC
#include <CL/cl.h>
#include <CL/cl_ext.h>
#else
#error We do not support inferior platforms.
#endif

#include "types.h"

class Model
{
public:
  Model(time_t _step);
  virtual ~Model() {}

  static std::shared_ptr<Model> getBestModel(time_t _step);

  virtual void nextStep() = 0;
  virtual void print();

protected:
  time_t step, origin, current;
};

class OpenCLModel : public Model
{
public:
  OpenCLModel(time_t _step, bool useAllDevices = false);
  ~OpenCLModel();

  void nextStep();
  void print();

private:
  char* readProgram(const char* filename, size_t &size);

  cl_context context;
  cl_command_queue queue;
  cl_program program;
  cl_kernel kernel;
  cl_mem deviceBuffer;
  cl_event kernelRun;
};

class FallbackModel : public Model
{
public:
  FallbackModel(time_t _step) : Model(_step) {}

  void nextStep();
};

#endif //__MODEL_H__
