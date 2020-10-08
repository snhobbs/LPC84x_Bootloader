#pragma once
#include <cstdint>
#include <array>
#include <Chip.h>
#include "isp_error_codes.h"

template<typename T, std::size_t size>
class Queue {
  std::array<T, size> buffer_{};
  std::size_t index_ = 1;

 public:
  void push(T* pbuff, std::size_t length) {
	for (std::size_t i = 0; i < size && index_ <= buffer_.size(); i++) {
	  buffer_[index_++] = pbuff[i];
	}
  }

  void pop(T* pbuff, std::size_t length) {
	for (std::size_t i = 0; i < size && index_ > 0; i++) {
	  buffer_[index_++] = pbuff[i];
	}
  }
};


namespace Isp {
void ExecuteImage(void);
bool ImageIsValid(void);

static const constexpr uint32_t kSectorSize = 1024;
static const constexpr uint32_t kSectorCount = 64;
#ifdef DEBUG
static const constexpr uint32_t kBootloaderSectors = 36;  // 0-35
#else
static const constexpr uint32_t kBootloaderSectors = 12;  // 0-11
#endif
static const constexpr uint32_t kImageSectorStart = kBootloaderSectors;  // 12-64
static const constexpr uint32_t kPageSize = 64;
static const constexpr uint32_t kPageCount = kSectorSize/kPageSize;
static const constexpr uint32_t kApplicationStackPointer = 0x10003FE0;
static_assert(kPageCount == 16);

const constexpr uint32_t kWriteSize = 1024;
static const constexpr uint32_t kBootloaderStart = 0x00;

//  Bootloader receives data in a stream, stores it, and waits for further instructions
class FlashController {
  bool lock_ = true;

 public:
  std::array<uint8_t, Isp::kSectorSize> buffer_;
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
uint32_t WriteToRam(const uint32_t start, const uint32_t length, uint8_t* buff);

template<typename T>
uint32_t ReadMemory(const uint32_t start, const uint32_t length, T& buffer) { return Isp::CMD_SUCCESS; }
uint32_t PrepSectorsForWrite(const uint32_t start, const uint32_t end);
uint32_t CopyRAMToFlash(const uint32_t flash_address, const uint32_t ram_address, const uint32_t length);
uint32_t Go(const uint32_t address, const char mode);
uint32_t EraseSector(const uint32_t start, const uint32_t end);
uint32_t ErasePages(const uint32_t start, const uint32_t end);
uint32_t ReadPartID(uint32_t* part_id);
uint32_t ReadBootCodeVersion(uint32_t* major, uint32_t* minor);
uint32_t MemoryLocationsEqual(const uint32_t address1, const uint32_t address2, const uint32_t length);
uint32_t ReadUID(std::array<uint32_t, 4>* uuid);

uint32_t ReadCRC(const uint32_t start, const uint32_t length, uint32_t* crc);

uint32_t ReadFlashSig(const uint32_t start, const uint32_t end, const uint32_t wait_states, const uint32_t mode, uint32_t* signature);

inline uint32_t TranslateStatus(uint32_t status) {
  return status;
}

}
