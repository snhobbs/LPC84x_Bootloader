/*
 * Copyright 2020 ElectroOptical Innovations, LLC
 * SystemConstants.h
 *
 */

#pragma once
#ifndef SYSTEMCONSTANTS_H_
#define SYSTEMCONSTANTS_H_

#include <Calculators/CalculatorBase.h>
#include <Chip.h>
#include <cstdint>

namespace Shell {
const constexpr size_t kMaximumReadWriteLength = 1024;
}
namespace SystemConstants {
const uint32_t kWatchDogClock = 10000;
const constexpr uint32_t kWatchDogTimeout = 2 * kWatchDogClock;
const constexpr uint32_t kSysTickFrequency = 1000;

const Chip::MainClockSpeeds kMainClockFrequency = Chip::MainClockSpeeds::k30M;

const constexpr uint8_t kISPCode = 0; //FIXME needs different firmare unlock method
}// namespace SystemConstants

#endif /* SYSTEMCONSTANTS_H_ */
