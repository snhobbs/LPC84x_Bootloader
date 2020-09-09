/*
 * Copyright 2020 ElectroOptical Innovations, LLC
 * SystemManager.cpp
 *
 */

#include "SystemManager.h"
#include "SystemMediator.h"

SystemMediator& SystemManager::GetSystemMediator(void) {
  static SystemMediator instance;
  return instance;
}

void SystemManager::RunBootup(void) {
  if (system_mediator_.BootupFinished()) {
    state_.set(SystemManager::State::kOperating);
    assert(state_.get() == SystemManager::State::kOperating);
  } else {
    system_mediator_.Run();
  }
}

void SystemManager::HandleFault(void) {
#ifdef DEBUG
  while (true) {
  continue;
  }
#endif

  //  FIXME put into a failsafe mode?
  //  state_.set(State::kOperating);  //  FIXME no fault handling here
}

void SystemManager::RunOperating(void) {
  GetSystemMediator().RunSerial();
  Watchdog::Kick();
  GetSystemMediator().HouseKeeping();
}
