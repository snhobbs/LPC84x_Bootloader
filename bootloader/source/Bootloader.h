#pragma once
#include <cstdint>
#include <array>
#include <Chip.h>
#include "isp_error_codes.h"
#include <RingBuffer/LightweightRingBuffer.h>
#include "HardwareLibrarian.h"
#include "shell/shell.h"

namespace Isp {
void ExecuteImage(void);
bool ImageIsValid(void);

const constexpr uint32_t kSectorSize = 1024;
const constexpr uint32_t kSectorCount = 64;
#ifdef DEBUG
const constexpr uint32_t kBootloaderSectors = 36;  // 0-35
#else
const constexpr uint32_t kBootloaderSectors = 12;  // 0-11
#endif
//const constexpr uint32_t kImageSectorStart = kBootloaderSectors;  // 12-64
const constexpr uint32_t kImageSectorStart = 0;  // 12-64
const constexpr uint32_t kVectorTableOffset = 0;//0x800;
const constexpr uint32_t kExecutionOffset = 0x1d00;//0x1cfc;//0x488;//0x800;
const constexpr uint32_t kImageStart = kImageSectorStart*kSectorSize;

const constexpr uint32_t kPageSize = 64;
const constexpr uint32_t kPageCount = kSectorSize/kPageSize;
//const constexpr uint32_t kApplicationStackPointer = 0x10003FE0;  //
const constexpr uint32_t kApplicationStackPointer = 0x10003fd0;
static_assert(kPageCount == 16);

const constexpr uint32_t kWriteSize = 1024;
const constexpr uint32_t kBootloaderStart = 0x00;
const constexpr uint32_t kRamStart = 0x10000000;
const constexpr uint32_t kRamOffset = 0x800;

//  Bootloader receives data in a stream, stores it, and waits for further instructions
class FlashController {
  bool lock_ = true;

 public:
  std::array<uint8_t, Isp::kSectorSize> buffer_ __attribute__((aligned(32)));
  void OpenFlashInteraction(void) {
    __disable_irq();
  }

  void CloseFlashInteraction(void) {
    __enable_irq();
  }

  void Unlock(void) {
    lock_ = false;
  }

  void Lock(void) {
    lock_ = true;
  }
};


uint32_t unlock(void);
uint32_t SetBaudRate(const uint32_t baudrate, const uint32_t stop_bits);
uint32_t ValidateWriteToRam(const uint32_t start, const uint32_t length);
uint32_t WriteToRam(const uint32_t start, const uint32_t length, uint8_t* buff);

uint32_t ReadMemory(const uint32_t start, const uint32_t length, uint8_t* buffer);
uint32_t PrepSectorsForWrite(const uint32_t start, const uint32_t end);
uint32_t CopyRAMToFlash(const uint32_t flash_address, const uint32_t ram_address, const uint32_t length);
uint32_t Go(const uint32_t address, const char mode);
uint32_t EraseSector(const uint32_t start, const uint32_t end);
uint32_t ErasePages(const uint32_t start, const uint32_t end);
uint32_t BlankSectorCheck(const uint32_t start, const uint32_t end);
uint32_t ReadPartID(uint32_t* part_id);
uint32_t ReadBootCodeVersion(uint32_t* major, uint32_t* minor);
uint32_t MemoryLocationsEqual(const uint32_t address1, const uint32_t address2, const uint32_t length);
uint32_t ReadUID(std::array<uint32_t, 4>* uuid);

uint32_t ReadCRC(const uint32_t start, const uint32_t length, uint32_t* crc);

uint32_t ReadFlashSig(const uint32_t start, const uint32_t end, const uint32_t wait_states, const uint32_t mode, uint32_t* signature);

inline uint32_t TranslateStatus(uint32_t status) {
  //ret = MAKE_STATUS((int32_t)kStatusGroup_IAP, (int32_t)status);
  if (status != Isp::CMD_SUCCESS) {
    return status - kStatusGroup_IAP*100;
  }
  return status;
}

#pragma GCC optimize("O0")
#pragma GCC push_options
class Bootloader {
 public:
  enum class State {
    kOperating,
	kWaitingForData
  };
  State state_ = State::kOperating;

  uint32_t loops_ = 0;
  const uint32_t kLoopsMax = 1<<20;
  std::size_t data_count_ = 0;
  std::array<uint8_t, 1024> buffer_{};
  std::size_t index_ = 0;
  std::size_t ram_start_ = 0;

  State get_state() const { return state_; }
  void set_state(State state) { state_ = state_; }
  void Setup(void) {}

  void Reset(void) {
    ram_start_ = 0;
    state_ = State::kOperating;
    loops_ = 0;
    data_count_ = 0;
    index_ = 0;
  }

  void SetupWriteToRam(const std::size_t start, const std::size_t length) {
	if (length <= buffer_.size()) {
	  ram_start_ = start;
	  state_ = State::kWaitingForData;
	  loops_ = 0;
	  data_count_ = length;
   	  index_ = 0;
	}
  }

  void ReadUart(UartControllerType* uart) {
    if(loops_++ > kLoopsMax) {
	  Reset();
    }
    while (!uart->RxEmpty() && index_ < buffer_.size() && data_count_ > 0) {
	  data_count_--;
	  const uint8_t ch = uart->read();
	  buffer_[index_++] = ch;
    }
    if (data_count_ == 0) {
      shell_put_line("OK");
      Isp::WriteToRam(ram_start_, index_, buffer_.data());
      Reset();
    }
  }
};
#pragma GCC pop_options
}
