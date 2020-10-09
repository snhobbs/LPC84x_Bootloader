#include "isp_error_codes.h"
#include "shell/shell.h"

#include <limits>
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include "SerialController.h"
#include "Bootloader.h"
#include "SystemConstants.h"
#include "SystemManager.h"
#include "HardwareLibrarian.h"
#include "shell_commands.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#endif
Isp::Bootloader& GetBootloader(void);

SerialController& GetSerialController(void);
namespace Shell {

/*
 * Global to connect the shell send character function
 * */

UartControllerType* GetShellUart(void) { return GetSerialController().uart_; }
//  int console_putc(char c);
//  char console_getc(void);

static int console_putc(char c) {
  GetShellUart()->write(static_cast<uint8_t>(c));
  return 0;
}

void SetupShell(void) {
  assert(GetShellUart() != nullptr);
  sShellImpl shell_impl = {
    .send_char = console_putc,
  };
  shell_boot(&shell_impl);

  shell_put_line("LPC845 Bootloader");
}


inline void RaiseError(const char* str, const uint32_t code) {
  prv_echo_str("> FAIL,");
  prv_echo_str(str);
  char buffer[std::numeric_limits<uint32_t>::digits10 + 1]{"\0"};
  sprintf(buffer, ",%lu", code);
  shell_put_line(buffer);
}

inline void SendResponseCode(const uint32_t code) {
  //  response sent as new line at the end of all arguments
  char buffer[std::numeric_limits<uint32_t>::digits10 + 1]{"\0"};
  sprintf(buffer, "%lu", code);
  shell_put_line(buffer);
}

static inline bool CheckArgLength(const int argc, const int length_low, const int length_high) {
  if (argc <= length_low) {
    RaiseError("too few arguments", 1);
    return false;
  } else if (argc > length_high + 1) {
    RaiseError("too many arguments", 1);
    return false;
  }
  return true;
}

namespace cli {
#if 0
int cmd_syncronize(int argc, char *argv[]) {
  shell_put_line("syncronized");
  return 0;
}
#endif

int cmd_printerror(int argc, char *argv[]) {
  if (!CheckArgLength(argc, 1, 1)) {
    return -1;
  }
  const int code = std::atoi(argv[1]);
  if (static_cast<std::size_t>(code) < Isp::return_codes.size() && code > 0) {
    shell_put_line(Isp::return_codes[code]);
  } else {
    RaiseError("Unknown code", 2);
  }
  return 0;
}

const constexpr uint32_t unlock_code = 23130;
int cmd_unlock(int argc, char *argv[]) {
  //  Enables Flash Write, Erase, & Go
  if (!CheckArgLength(argc, 1, 1)) {
    return -1;
  }

  const uint32_t code = std::atol(argv[1]);
  if (code == unlock_code) {
    const uint32_t response_code = Isp::unlock();
    SendResponseCode(response_code);
  } else {
    shell_put_line("> FAIL,Unlock code incorrect,2");
  }
  return 0;
}

//  FIXME hardweare state change
int cmd_set_baudrate(int argc, char *argv[]) {
  //  Baud Depends of FAIM config, stopbit is 1 or 2
  if (!CheckArgLength(argc, 1, 2)) {
    return -1;
  }
  uint32_t stop_bits = 1;
  if (argc > 2) {
    stop_bits = std::atol(argv[2]);
  }
  if (stop_bits > 2) {
    RaiseError("Illegal number of stop bits", 2);
  }
  const uint32_t response_code = Isp::SetBaudRate(std::atol(argv[1]), stop_bits);
  SendResponseCode(response_code);
  return 0;
}

int cmd_echo(int argc, char *argv[]) {
  if (!CheckArgLength(argc, 1, 1)) {
    return -1;
  }
  shell_set_echo(std::atoi(argv[1]) != 0);
  const uint32_t response_code = Isp::CMD_SUCCESS;
  SendResponseCode(response_code);
  return 0;
}

//  FIXME needs a data slurp mode
int cmd_write_to_ram(int argc, char *argv[]) {
  if (!CheckArgLength(argc, 2, 2)) {
    return -1;
  }
  //  when transfer is complete send OK\c\r
  const uint32_t start = std::atol(argv[1]);
  const uint32_t length = std::atol(argv[2]);
#if 0
  if (strlen(argv[3]) < length) {
    SendResponseCode(Isp::COUNT_ERROR);
  } else {
#endif
  const uint32_t response_code = Isp::ValidateWriteToRam(start, length);
  SendResponseCode(response_code);
  if (response_code == Isp::CMD_SUCCESS) {
    GetBootloader().SetupWriteToRam(start, length);
  }
  return 0;
}

int cmd_read_memory(int argc, char *argv[]) {
  if (!CheckArgLength(argc, 2, 2)) {
    return -1;
  }

  const uint32_t start = std::atol(argv[1]);
  const uint32_t length = std::atol(argv[2]);
  std::array<uint8_t, Shell::kMaximumReadWriteLength> buffer{};
  const uint32_t response_code = Isp::ReadMemory(start, length, buffer.data());
  for (std::size_t i = 0; i < length && i < buffer.size(); i++) {
    prv_echo(static_cast<char>(buffer[i]));
  }
  shell_put_line("\n");
  SendResponseCode(response_code);
  return 0;
}

int cmd_prep_sectors(int argc, char *argv[]) {
  if (!CheckArgLength(argc, 2, 2)) {
    return -1;
  }
  const uint32_t start = std::atol(argv[1]);
  const uint32_t end = std::atol(argv[2]);

  const uint32_t response_code = Isp::PrepSectorsForWrite(start, end);
  SendResponseCode(response_code);
  return 0;
}

int cmd_copy_ram_to_flash(int argc, char *argv[]) {
  if (!CheckArgLength(argc, 3, 3)) {
    return -1;
  }
  const uint32_t flash_address = std::atol(argv[1]);
  const uint32_t ram_address = std::atol(argv[2]);
  const uint32_t length = std::atol(argv[3]);

  const uint32_t response_code = Isp::CopyRAMToFlash(flash_address, ram_address, length);
  SendResponseCode(response_code);
  return 0;
}

int cmd_go(int argc, char *argv[]) {
  if (!CheckArgLength(argc, 1, 2)) {
    return -1;
  }
  char mode = '\0'; 
  if (argc > 2) {
    mode = argv[2][0];
  }
  const uint32_t address = std::atol(argv[1]);
  const uint32_t response_code = Isp::Go(address, mode);
  SendResponseCode(response_code);
  return 0;
}

int cmd_erase_sector(int argc, char *argv[]) {
  if (!CheckArgLength(argc, 2, 2)) {
    return -1;
  }
  const uint32_t start = std::atol(argv[1]);
  const uint32_t end = std::atol(argv[2]);
  const uint32_t response_code = Isp::EraseSector(start, end);
  SendResponseCode(response_code);
  return 0;
}

int cmd_erase_pages(int argc, char *argv[]) {
  if (!CheckArgLength(argc, 2, 2)) {
    return -1;
  }
  const uint32_t start = std::atol(argv[1]);
  const uint32_t end = std::atol(argv[2]);
  const uint32_t response_code = Isp::ErasePages(start, end);
  SendResponseCode(response_code);
  return 0;
}

int cmd_check_sectors_blank(int argc, char *argv[]) {
  if (!CheckArgLength(argc, 2, 2)) {
    return -1;
  }
  const uint32_t start = std::atol(argv[1]);
  const uint32_t end = std::atol(argv[2]);
  const uint32_t response_code = Isp::BlankSectorCheck(start, end);
  SendResponseCode(response_code);
  return 0;
}

int cmd_read_part_id(int argc, char *argv[]) {
  if (!CheckArgLength(argc, 0, 0)) {
    return -1;
  }
  uint32_t part_id = 0;
  const uint32_t response_code = Isp::ReadPartID(&part_id);
  SendResponseCode(response_code);
  char buffer[(std::numeric_limits<uint32_t>::digits10 + 3)]{"\0"};
  //sprintf(buffer, "0x%x", part_id);
  sprintf(buffer, "%lu", part_id);
  shell_put_line(buffer);
  return 0;
}

int cmd_read_bootcode_version(int argc, char *argv[]) {
  if (!CheckArgLength(argc, 0, 0)) {
    return -1;
  }
  uint32_t major = 0;
  uint32_t minor = 0;
  const uint32_t response_code = Isp::ReadBootCodeVersion(&major, &minor);
  SendResponseCode(response_code);
  char buffer[(std::numeric_limits<uint32_t>::digits10 + 3)]{"\0"};
  sprintf(buffer, "%lu", major);
  shell_put_line(buffer);

  sprintf(buffer, "%lu", minor);
  shell_put_line(buffer);
  return 0;
}

int cmd_memory_locations_equal(int argc, char *argv[]) {
  if (!CheckArgLength(argc, 3, 3)) {
    return -1;
  }

  const uint32_t flash_address = std::atol(argv[1]);
  const uint32_t ram_start = std::atol(argv[2]);
  const uint32_t length = std::atol(argv[3]);
  const uint32_t response_code = Isp::MemoryLocationsEqual(flash_address, ram_start, length);
  SendResponseCode(response_code);
  return 0;
}

int cmd_read_uid(int argc, char *argv[]) {
  if (!CheckArgLength(argc, 0, 0)) {
    return -1;
  }
  std::array<uint32_t, 4> uuid;
  const uint32_t response_code = Isp::ReadUID(&uuid);
  SendResponseCode(response_code);

  for (uint32_t pt : uuid) {
    char buffer[(std::numeric_limits<uint32_t>::digits10 + 3)]{"\0"};
    sprintf(buffer, "0x%08lx", pt);
    shell_put_line(buffer);
  }
  return 0;
}

int cmd_read_CRC(int argc, char *argv[]) {
  if (!CheckArgLength(argc, 2, 2)) {
    return -1;
  }
  const uint32_t start = std::atol(argv[1]);
  const uint32_t length = std::atol(argv[2]);
  uint32_t crc = 0;
  const uint32_t response_code = Isp::ReadCRC(start, length, &crc);
  SendResponseCode(response_code);

  if (response_code == Isp::CMD_SUCCESS) {
    char buffer[(std::numeric_limits<uint32_t>::digits10 + 1)]{"\0"};
    sprintf(buffer, "%lu", crc);
    shell_put_line(buffer);
  }
  return 0;
}

int cmd_read_flash_signature(int argc, char *argv[]) {
  uint32_t wait_states = 2;
  uint32_t mode = 0;
  if (!CheckArgLength(argc, 2, 4)) {
    return -1;
  }
  if (argc > 3) {
    wait_states = std::atol(argv[3]);
  }
  if (argc > 4) {
    mode = std::atol(argv[4]);
  }

  const uint32_t start = std::atol(argv[1]);
  const uint32_t end = std::atol(argv[2]);
  uint32_t signature[4]{};
  const uint32_t response_code = Isp::ReadFlashSig(start, end, wait_states, mode, signature);
  SendResponseCode(response_code);

  if (response_code == Isp::CMD_SUCCESS) {
    char buffer[(std::numeric_limits<uint32_t>::digits10*sizeof(signature) + 3)]{"\0"};
    sprintf(buffer, "%lu\n%lu\n%lu\n%lu", signature[0], signature[1], signature[2], signature[3]);
    shell_put_line(buffer);
  }
  return 0;
}

int cmd_enter_isp(int argc, char *argv[]) {
  if (!CheckArgLength(argc, 1, 1)) {
    return -1;
  }
  if (strcmp(argv[0], "unlock")) {
    IapController::InvokeIsp();
    return 0;
  } else {
    SendResponseCode(Isp::INVALID_CODE);
  }
  return 0;
}

int cmd_check_valid(int argc, char *argv[]) {
  if (Isp::ImageIsValid()) {
    SendResponseCode(Isp::CMD_SUCCESS);
  } else {
    SendResponseCode(Isp::USER_CODE_CHECKSUM);
  }
  return 0;
}

#if 0
    def ReadWriteFAIM(self):
        response_code = self.WriteCommand("O")
        RaiseReturnCodeError(response_code, "Read Write FAIM")
#endif

}  //  namespace cli

static const sShellCommand s_shell_commands[] = {
  //{"?", cli::cmd_syncronize, "Required for compatability with NXP ISP loader. Arguments: None"},
  {"U", cli::cmd_unlock, "Unlock device, enables flash write, erase, & go. Arguments: code (uint32_t)"},
  {"B", cli::cmd_set_baudrate, "Set baudrate. Arguments: baudrate (uint32_t), stop bits (uint32_t, optional)"},
  {"A", cli::cmd_echo, "Enable/Disable echo. Arguments: enable (bool)"},
  {"W", cli::cmd_write_to_ram, "Write to ram. Arguments: start (uint32_t), data length in bytes (uint32_t)"},
  {"R", cli::cmd_read_memory, "Read from memory. Arguments: start (uint32_t), data length in bytes (uint32_t)"},
  {"P", cli::cmd_prep_sectors, "Prepare sectors for write. Arguments: start (uint32_t), end (uint32_t)"},
  {"C", cli::cmd_copy_ram_to_flash, "Copy RAM to flash. Arguments: flash address (uint32_t), RAM address (uint32_t), bytes (uint32_t)"},
  {"G", cli::cmd_go, "Begin executing code at the given location. Arguments: starting address (uint32_t), mode (char) T sets thumb mode"},
  {"E", cli::cmd_erase_sector, "Erase sector. Arguments: starting address (uint32_t), ending address (uint32_t)"},
  {"X", cli::cmd_erase_pages, "Erase pages. Arguments: starting address (uint32_t), ending address (uint32_t)"},
  {"I", cli::cmd_check_sectors_blank, "Check if sectors are blank. Arguments: starting address (uint32_t), ending address (uint32_t)"},
  {"J", cli::cmd_read_part_id, "Read part id. Arguments: None"},
  {"K", cli::cmd_read_bootcode_version, "Read bootloader version. Arguments: None"},
  {"M", cli::cmd_memory_locations_equal, "Checks to see if two sections in the memory map are equal. Arguments: flash address, ram start, bytes"},
  {"N", cli::cmd_read_uid, "Reads chip unique identifier. Arguments: None"},
  {"S", cli::cmd_read_CRC, "Calculate CRC of the given block of memory. Arguments: start (uint32_t), data length in bytes (uint32_t)"},
  {"Z", cli::cmd_read_flash_signature, "Read flash signature. Arguments: start (uint32_t), end (uint32_t), wait_states (uint32_t optional), mode (uint32_t, optional)"},
  {"CV", cli::cmd_check_valid, "Check image validity"},
  {"isp", cli::cmd_enter_isp, "Enter ISP mode. Argument: code (uint32_t, send \"unlock\")"},
  {"err", cli::cmd_printerror, "Print name of error code. Arguments: code (uint32_t)"},
  {"help", shell_help_handler, "Lists all commands"},
  {"?", shell_help_handler, "Same as help"},
};
}
const sShellCommand *const g_shell_commands = Shell::s_shell_commands;
const size_t g_num_shell_commands = ARRAY_SIZE(Shell::s_shell_commands);
