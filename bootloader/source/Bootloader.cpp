#include "Bootloader.h"
#include "shell/shell.h"
#include "nxpisp/isp.h"
#include <cstdint>
#include <HardwareLibrarian.h>
/*
 * Global to connect the shell send character function
 * */
__attribute__((__section__(".m_validate"))) volatile const uint32_t image_signature __attribute__((aligned(4))) = 0xdeadbeef;
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

#if 0
char console_getc(void) {
  uint8_t ch = shell_uart->read();
  return static_cast<uint8_t>(ch);
}
#endif

void SetupShell(void) {
  SetShellUart(shell_uart);
  sShellImpl shell_impl = {
    .send_char = console_putc,
  };
  shell_boot(&shell_impl);
}

void Go(void) {
  Isp::Go(0x00, '\0');
}

bool ImageIsValid(void) {
  return image_signature == 0xdeadbeef;
}
