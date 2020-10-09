/*
 * shell_commands.h
 *
 *  Created on: Oct 7, 2020
 *      Author: simon
 */
#pragma once
#ifndef SHELL_COMMANDS_H_
#define SHELL_COMMANDS_H_

#include "HardwareLibrarian.h"
#include "shell/shell.h"

//  extern const uint32_t image_signature;
//
namespace Shell {
void SetShellUart(UartControllerType* uart);
UartControllerType* GetShellUart(void);
//  int console_putc(char c);
//  char console_getc(void);
void SetupShell(void);
}

#endif /* SHELL_COMMANDS_H_ */
