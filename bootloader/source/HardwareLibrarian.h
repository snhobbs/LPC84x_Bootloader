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
#include <uart.h>

class HardwareLibrarian {
  Board board_{SystemConstants::kMainClockFrequency};
  RingBuffer<uint8_t, 128> uart0_rx_;
  RingBuffer<uint8_t, 128> uart0_tx_;
  UARTController uart_{USARTChannel::Channel0, BaudRates::b9600, uart0_rx_,
                      uart0_tx_};
  bool pins_sane_at_startup_ = false;
  void SetupUsart(void) {
    Usart::ConfigurationTable table;
    table.output_enable_polarity = Usart::ConfigurationTable::LogicPolarity::kHigh;
    table.output_enable_turnaround = true;
    table.output_select_enable = true;
    uart_.Setup(table);
  }

  /*
   * Check the state of outputs that can be checked, this is
   * GPIOs, PWMs, DAQs, I2C pins
   * */
  bool PinStatesSane(void) const {
    return true;
  }

 public:
  void GetSerialNumber(char* data, uint32_t array_size) {
    return board_.GetSerialNumber(data, array_size);
  }

  void EnableInterrupts(void) {
    // enable uart interrupts
    uart_.EnableInterrupt(UsartDriver::InterruptFlag::kRxReady);
    assert(uart_.GetEnabledInterrupts() ==
           1 << static_cast<uint32_t>(UsartDriver::InterruptFlag::kRxReady));
    //  spi_.EnableInterrupt(_spi_interrupt_enable::kSPI_TxReadyInterruptEnable, true);
  }

  void Setup(void) {
    board_.Setup();
    pins_sane_at_startup_ = PinStatesSane();
    SetupUsart();
  }

  void Reset(void) {}

  UARTController& GetSerialControlUart(void) { return uart_; }

  bool get_pins_sane_at_startup(void) const {
    return pins_sane_at_startup_;
  }
  //GPIOOutputPin& GetTxen(void) { return IOPins::usart0_rts; }
  HardwareLibrarian(void) {}
};

#endif /* HARDWARELIBRARIAN_H_ */
