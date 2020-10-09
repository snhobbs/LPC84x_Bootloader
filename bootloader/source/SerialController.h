/*
 * SerialController.h
 *
 *  Created on: Oct 7, 2020
 *      Author: simon
 */

#ifndef SERIALCONTROLLER_H_
#define SERIALCONTROLLER_H_

#include <array>
#include "shell_commands.h"
#include "HardwareLibrarian.h"
#if 0
template<typename T, std::size_t size>
class LiFo {
  std::array<T, size> buffer_{};
  std::size_t index_ = 1;

 public:
  void push(T* pbuff, std::size_t length) {
	for (std::size_t i = 0; i < size && index_ <= buffer_.size(); i++) {
	  buffer_[index_++] = pbuff[i];
	}
  }

  void pop(T* pbuff, std::size_t length) {
	for (std::size_t i = 0; i < size && index_ > 0; i++) {
	  buffer_[index_--] = pbuff[i];
	}
  }

  bool is
  void Reset(void) {
    index_ = 0;
  }
};
#endif

//  Holds state for receiving data
class SerialController {
 public:
  UartControllerType* uart_ = nullptr;
  enum class State {
	kShell,
	kBootup
  };
  State state = State::kBootup;

  void SetUart(UartControllerType* uart) {
    uart_ = uart;
  }
  void Setup(void) {
	Shell::SetupShell();
	state = State::kShell;
  }

  void Run(void) {
	switch(state) {
	case State::kBootup:
	  break;
	case State::kShell:
   	  while (!uart_->RxEmpty()) {
	    const uint8_t ch = uart_->read();
	    shell_receive_char(static_cast<char>(ch));
	  }
   	  break;
	}
  }

  SerialController(UartControllerType* uart) : uart_{uart} {}
  SerialController(void){}
};




#endif /* SERIALCONTROLLER_H_ */
