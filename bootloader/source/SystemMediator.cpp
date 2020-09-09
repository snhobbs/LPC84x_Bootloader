/*
 * Copyright 2020 ElectroOptical Innovations, LLC
 * SystemController.cpp
 *
 *  Created on: Nov 1, 2019
 *      Author: simon
 */
#include "SystemMediator.h"
#include "Config.h"
#include <algorithm>

/*
 * Global to connect the shell send character function
 * */
static UartControllerType* shell_uart = nullptr;

void SetShellUart(UartControllerType* uart) {
  shell_uart = uart;
}

UartControllerType* GetShellUart(void) {
  return shell_uart;
}

int console_putc(char c) {
  shell_uart->write(static_cast<uint8_t>(c));
}

char console_getc(void) {
  uint8_t ch = shell_uart->read();
  return static_cast<uint8_t>(ch);
}

#if 0
void SystemMediator::RunUserCommand(const SerialCommand& command) {
    /* Once a command has been closed, run it */
  const uint8_t message_type = command.GetCmd();
  const ControllerCommands cmd = static_cast<ControllerCommands>(command.GetCmd());
  switch (cmd) {
    /* Modes */
    case ControllerCommands::GetVersion: {
      script_control_.StreamMessage(
          reinterpret_cast<const uint8_t*>(__HARDWARE_VERSION__),
          sizeof(__HARDWARE_VERSION__), message_type);
      break;
    }
    case ControllerCommands::GetFirmwareVersion: {
      script_control_.StreamMessage(
          reinterpret_cast<const uint8_t*>(__FIRMWARE_VERSION__),
          sizeof(__FIRMWARE_VERSION__), message_type);
      break;
    }
    case ControllerCommands::GetCompileDate: {
      script_control_.StreamMessage(reinterpret_cast<const uint8_t*>(__DATE__),
                                    sizeof(__DATE__), message_type);
      break;
    }
    case ControllerCommands::GetCompileTime: {
      script_control_.StreamMessage(reinterpret_cast<const uint8_t*>(__TIME__),
                                    sizeof(__TIME__), message_type);
      break;
    }
    case ControllerCommands::GetSerialNumber: {
      script_control_.StreamMessage(
          reinterpret_cast<const uint8_t*>(serial_number_.data()),
          serial_number_.size(), message_type);
      break;
    }
    case ControllerCommands::ISPMode: {
      SetISPMode();
      break;
    }

    default:
      assert(0);
      break;
  }
}
#endif
