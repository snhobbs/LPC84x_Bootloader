/*
 * Copyright 2020 ElectroOptical Innovations, LLC
 * SystemController.h
 *
 *  Created on: September 4, 2020
 */

#pragma once
#ifndef SYSTEMMEDIATOR_H_
#define SYSTEMMEDIATOR_H_

#include "shell/shell.h"
#include "Board.h"
#include "Config.h"
#include "HardwareLibrarian.h"

#include <Utilities/TypeConversion.h>
#include <Utilities/CommonTypes.h>
#include <Watchdog.h>
#include <IAP.h>
#include <array>
#include <cstdint>

void SetShellUart(UartControllerType* uart);
UartControllerType* GetShellUart(void);
int console_putc(char c);
char console_getc(void);

class SystemMediator {
  /*
   * SystemMediator attempts to follow the mediator pattern for
   * interfacing objects that need to interact
   * */

  enum class State {
    Initial,
    Operating,
    Fault,
  };

  std::size_t loops_ = 0;
  Watchdog watchdog_;
  HardwareLibrarian hardware_{};

  UartControllerType& uart_{hardware_.GetSerialControlUart()};
  Utilities::StateController<State> state_{};

 public:
  void Reset(void) {}

  void Setup(void) {
#ifndef DEBUG
    watchdog_.Setup(SystemConstants::kWatchDogClock, SystemConstants::kWatchDogTimeout);
    if (watchdog_.ReadTimeoutFlag()) {
      assert(0);
    }
#endif
    hardware_.Setup();
    Reset();
    hardware_.EnableInterrupts();
    SetupShell();
  }

  void SetupShell(void) {
    SetShellUart(&uart_);
    sShellImpl shell_impl = {
      .send_char = console_putc,
    };
    shell_boot(&shell_impl);
  }
  void Run(void) {
    watchdog_.Kick();
    switch (state_.get()) {
    case (State::Operating):
      RunSerial();
      break;

    case (State::Initial):
      Setup();
      state_.set(State::Operating, 0);
      break;

    case (State::Fault):
      break;
    }
  }

  int32_t HouseKeeping(void) {
    return 0;
  }

  int32_t RunSerial(void) {
    while (!uart_.TxEmpty()) {
      char c = console_getc();
      shell_receive_char(c);
    }  
    return 0;
  }

  void USART0_IRQHandler(void) {
    hardware_.GetSerialControlUart().RxTxInterruptDriverIO();
  }

 private:
  // Script Control Functions
  void SetISPMode(void) {
    // fixme set safe mode
    // __disable_irq();
    HouseKeeping();
    IspController::InvokeIsp();
  }

 public:
  SystemMediator(void) {}
};

#endif /* SYSTEMMEDIATOR_H_ */
