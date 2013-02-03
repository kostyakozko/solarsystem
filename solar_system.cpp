#include <ctime>
#include <memory>

#include "model.h"

int main()
{
  time_t step = 86400*15;
  std::shared_ptr<Model> model = Model::getBestModel(step);

  while(1)
  {
    model->print();
    model->nextStep();
  }

  return 0;
}
