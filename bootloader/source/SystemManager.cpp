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
