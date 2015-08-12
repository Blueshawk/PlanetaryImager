#include "driver.h"
#include "qhy/qhydriver.h"

using namespace std;

typedef QList<shared_ptr<Driver>> Drivers;

class SupportedDrivers::Private {
public:
  Private(const Drivers &drivers, SupportedDrivers *q);
  Drivers drivers;
  
private:
  SupportedDrivers *q;
};

SupportedDrivers::Private::Private(const Drivers& drivers, SupportedDrivers* q) : drivers{drivers}, q{q}
{

}

SupportedDrivers::SupportedDrivers() : dpointer({make_shared<QHYDriver>(), }, this)
{

}

SupportedDrivers::~SupportedDrivers()
{
}


Driver::Cameras SupportedDrivers::cameras() const
{
  Cameras cameras;
  for(auto driver: d->drivers) {
    for(auto camera: driver->cameras()) {
      cameras.push_back(camera);
    }
  }
  return cameras;
}
