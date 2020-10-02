#include "Bootloader.h"
#include "shell/shell.h"
#include "nxpisp/isp.h"
#include <Crc.h>
#include <cstdint>
#include "HardwareLibrarian.h"
/*
 * Global to connect the shell send character function
 * */
static const constexpr uint32_t kSectorSize = 1024;
static const constexpr uint32_t kSectorCount = 64;
static const constexpr uint32_t kBootloaderSectors = 12;  // 0-11
static const constexpr uint32_t kImageSectorStart = kBootloaderSectors;  // 12-64
static const constexpr uint32_t kPageSize = 64;
static const constexpr uint32_t kPageCount = kSectorSize/kPageSize;
static const constexpr uint32_t kApplicationStackPointer = 0x10003FE0;
static_assert(kPageCount == 16);


__attribute__((__section__(".m_validate"))) volatile const uint32_t image_signature __attribute__((aligned(4))) = 0xdeadbeef;
static UartControllerType* shell_uart = nullptr;


void SetShellUart(UartControllerType* uart) {
  shell_uart = uart;
}

UartControllerType* GetShellUart(void) {
  return shell_uart;
}

static int console_putc(char c) {
  shell_uart->write(static_cast<uint8_t>(c));
  return 0;
}

#if 0
char console_getc(void) {
  uint8_t ch = shell_uart->read();
  return static_cast<uint8_t>(ch);
}
#endif

void SetupShell(void) {
  assert(GetShellUart() != nullptr);
  sShellImpl shell_impl = {
    .send_char = console_putc,
  };
  shell_boot(&shell_impl);
}

__attribute__((naked, noreturn)) static void BootJumpASM(uint32_t SP, uint32_t RH) {
  __asm("MSR      MSP,r0");
  __asm("BX       r1");
}

static void BootJump(const uint32_t image_start, const uint32_t stack_pointer) {
  //  https://www.keil.com/support/docs/3913.htm
  //  make sure we're in privilaged mode
  //  disable all interrupts
  
  //  disable peripherals to a known state
  //  Clear pending interrupts
  //  Disable systick and clear all state
  
  //  Activate the MSP, if the core is found to currently run with the PSP. As the compiler might still uses the stack, the PSP needs to be copied to the MSP before this
  if (CONTROL_SPSEL_Msk & __get_CONTROL()) {
    /* MSP is not active */
    __set_MSP(__get_PSP());
    __set_CONTROL(__get_CONTROL() & ~CONTROL_SPSEL_Msk);
  }

  //  Load the vector table address of the user application into SCB->VTOR register. Make sure the address meets the alignment requirements
  //  Check for a shadow pointer to the VTOR
  //
  BootJumpASM(image_start, stack_pointer);
}


void ExecuteImage(void) {
  const constexpr uint32_t start_address = kImageSectorStart * kSectorSize + sizeof(image_signature);
  BootJump(start_address, kApplicationStackPointer);
}

bool ImageIsValid(void) {
  const uint32_t crc = 0;
  Crc crc_controller;
  crc_controller.Setup();
  const constexpr uint32_t image_length = (kSectorCount - kBootloaderSectors) * kSectorSize;
  crc_controller.WriteData(reinterpret_cast<const uint8_t* const>(kImageSectorStart * kSectorSize + sizeof(image_signature)), image_length);
  return image_signature == crc_controller.Get32BitResult();
}
