/*
 * SerialController.h
 *
 *  Created on: Oct 7, 2020
 *      Author: simon
 */

#ifndef SERIALCONTROLLER_H_
#define SERIALCONTROLLER_H_

#include <array>
#include "HardwareLibrarian.h"

//  Holds state for receiving data
class SerialController {
 public:
  UartControllerType* uart_ = nullptr;

  SerialController(UartControllerType* uart) : uart_{uart} {}
  SerialController(void){}
};




#endif /* SERIALCONTROLLER_H_ */
