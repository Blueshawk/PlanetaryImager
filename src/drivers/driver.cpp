/*
 * Copyright (C) 2016  Marco Gulino <marco@gulinux.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include "driver.h"
#include "qhy/qhydriver.h"
#include "simulator/simulatordriver.h"
#include "available_drivers.h"

using namespace std;

class SupportedImagingDrivers::Private {
public:
  Private(SupportedImagingDrivers *q);  
private:
    SupportedImagingDrivers *q;
};

SupportedImagingDrivers::Private::Private(SupportedImagingDrivers* q) : q{q}
{

}

SupportedImagingDrivers::SupportedImagingDrivers() : dptr(this)
{
}

SupportedImagingDrivers::~SupportedImagingDrivers()
{
}


ImagingDriver::Cameras SupportedImagingDrivers::cameras() const
{
  Cameras cameras;
  qDebug() << "drivers: " << AvailableDrivers::drivers.size();
  for(auto driver: AvailableDrivers::drivers) {
    qDebug() << "driver cameras: " << driver->cameras().size();
    cameras.append(driver->cameras());
  }
  return cameras;
}
