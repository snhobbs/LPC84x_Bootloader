/*
 * Copyright 2020 ElectroOptical Innovations, LLC
 * SystemController.h
 *
 *  Created on: September 4, 2020
 */

#pragma once
#ifndef SYSTEMMEDIATOR_H_
#define SYSTEMMEDIATOR_H_

#pragma GCC optimize("Os")
#pragma GCC push_options
#include "Board.h"
#include "Config.h"
#include "HardwareLibrarian.h"

#pragma GCC pop_options

#pragma GCC optimize("Og")
#pragma GCC push_options
#include <Utilities/TypeConversion.h>
#include <Utilities/CommonTypes.h>
#include <Watchdog.h>
#include <IAP.h>
#include <array>
#include <cstdint>
#pragma GCC pop_options

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

  //ScriptControl script_control_{hardware_.GetSerialControlUart()};
  std::array<char, (Chip::GetSerialNumberLength())> serial_number_;
  Utilities::StateController<State> state_{};

 public:
  bool BootupFinished(void) const { return state_.get() == State::Operating; }
  bool IsFault(void) const { return state_.get() == State::Fault; }

  void Reset(void) {
  }

  void Setup(void) {
#ifndef DEBUG
    watchdog_.Setup(SystemConstants::kWatchDogClock, SystemConstants::kWatchDogTimeout);
    if (watchdog_.ReadTimeoutFlag()) {
      assert(0);
    }
#endif
    hardware_.Setup();
    const bool pin_states_sane = hardware_.get_pins_sane_at_startup();
    if (!pin_states_sane) {
      assert(pin_states_sane);
    }
    Reset();
    hardware_.GetSerialNumber(serial_number_.data(), serial_number_.size());
    hardware_.EnableInterrupts();
  }

  void Run(uint32_t tick) {
    switch (state_.get()) {
    case (State::Operating):
      break;

    case (State::Initial):
      Setup();
      state_.set(State::Operating, tick);
      break;

    case (State::Fault):
      break;
    }
  }

  int32_t UpdateFaultState(void);

  // Tasks
  int32_t HouseKeeping(void) {
    // fixme update algorithms here?
#ifndef NDEBUG
    blinky(loops_++);
#endif
    return 0;
  }

  int32_t RunSerial(void) {
    //script_control_.ProcessSerialBuffer();
    return 0;
  }

  void USART0_IRQHandler(void) {
    ControllerUSARTHandler();
  }

  void ControllerUSARTHandler(void) {
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
