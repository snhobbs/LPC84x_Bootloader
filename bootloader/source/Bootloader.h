#pragma once
#include <cstdint>
#include "HardwareLibrarian.h"
#include "shell/shell.h"
//  extern const uint32_t image_signature;
//
void SetShellUart(UartControllerType* uart);
UartControllerType* GetShellUart(void);
int console_putc(char c);
char console_getc(void);
void SetupShell(void);
void Go(void);
bool ImageIsValid(void);


