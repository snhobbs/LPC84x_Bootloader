/*
 * Copyright 2020 ElectroOptical Innovations, LLC
 * HardwareLibrarian.h
 *
 *      Author: simon
 */

#pragma once
#ifndef HARDWARELIBRARIAN_H_
#define HARDWARELIBRARIAN_H_

#include "Board.h"
#include "SystemConstants.h"
#include <RingBuffer/LightweightRingBuffer.h>
#include <uart.h>
#define UART_BUFFER_SIZE 2048
using UartControllerType = UartController<LightWeightRingBuffer<uint8_t, UART_BUFFER_SIZE>, LightWeightRingBuffer<uint8_t, UART_BUFFER_SIZE>>;
class HardwareLibrarian {
  Board board_{SystemConstants::kMainClockFrequency};
  LightWeightRingBuffer<uint8_t, UART_BUFFER_SIZE> uart0_rx_;
  LightWeightRingBuffer<uint8_t, UART_BUFFER_SIZE> uart0_tx_;
  UartControllerType uart_{USARTChannel::Channel0, BaudRates::b9600, uart0_rx_,
                      uart0_tx_};

  void SetupUsart(void) {
    Usart::ConfigurationTable table;
#ifdef RS485
	table.output_enable_polarity = Usart::ConfigurationTable::LogicPolarity::kHigh;
    table.output_enable_turnaround = true;
    table.output_select_enable = true;
#endif
    uart_.Setup(table);
  }

 public:
  void EnableInterrupts(void) {
    // enable uart interrupts
    uart_.EnableInterrupt(UsartDriver::InterruptFlag::kRxReady);
    assert(uart_.GetEnabledInterrupts() ==
           1 << static_cast<uint32_t>(UsartDriver::InterruptFlag::kRxReady));
  }

  void Setup(void) {
    board_.Setup();
    SetupUsart();
  }

  void Reset(void) {}
  UartControllerType& GetSerialControlUart(void) { return uart_; }
  HardwareLibrarian(void) {}
};

#endif /* HARDWARELIBRARIAN_H_ */
