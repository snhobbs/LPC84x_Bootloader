#include "Bootloader.h"
#include "isp_error_codes.h"
#include <Crc.h>
#include <array>
#include <cstdint>
extern uint32_t SystemCoreClock;

//  FIXME disable interrupts with each of the read/writes to flash
namespace Isp {
static FlashController flash;

__attribute__((__section__(".m_validate"))) volatile const uint32_t image_signature __attribute__((aligned(4))) = 0xdeadbeef;

__attribute__((naked, noreturn)) static void BootJumpASM(uint32_t SP, uint32_t RH) {
  __asm("MSR      MSP,r0");
  __asm("BX       r1");
}

static uint32_t GetSystemCoreClock(void) {
  return SystemCoreClock;
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

constexpr inline bool FlashSectorLegal(uint32_t sector) {
  return sector >= Isp::kImageSectorStart && sector < Isp::kSectorCount;
}

constexpr inline bool FlashPageLegal(std::size_t page) {
  return page >= Isp::kImageSectorStart * Isp::kSectorSize/Isp::kPageSize && page < Isp::kSectorSize/Isp::kPageSize;
}

constexpr inline bool FlashAddressLegal(std::size_t address) {
  return address >= Isp::kImageSectorStart * Isp::kSectorSize && address < Isp::kSectorCount * Isp::kSectorSize;
}

constexpr inline bool FlashAddressRangeLegal(std::size_t start, std::size_t length) {
  return FlashAddressLegal(start) && FlashAddressLegal(start + length) && length <= Isp::kSectorSize;
}

constexpr inline bool RamAddressLegal(std::size_t address) {
  return address < flash.buffer_.size();
}

constexpr inline bool RamRangeLegal(std::size_t start, std::size_t length) {
  return RamAddressLegal(start) && RamAddressLegal(start + length) && length <= Isp::kSectorSize;
}

void ExecuteImage(void) {
  const constexpr uint32_t start_address = kImageSectorStart * kSectorSize + sizeof(image_signature);
  BootJump(start_address, kApplicationStackPointer);
}

bool ImageIsValid(void) {
  Crc crc_controller;
  crc_controller.Setup();
  const constexpr uint32_t image_length = (kSectorCount - kBootloaderSectors) * kSectorSize - sizeof(image_signature);
  uint8_t* p_image = reinterpret_cast<uint8_t* const>(kImageSectorStart * kSectorSize + sizeof(image_signature));
  crc_controller.WriteData(p_image, image_length);
  assert(image_signature == *reinterpret_cast<volatile uint32_t*>(kSectorCount*kSectorSize - sizeof(uint32_t)));
  return image_signature == crc_controller.Get32BitResult();
}

#if 0
  /* RAM address must be within the buffer, destination must not be in bootloader space */
  uint32_t CopyRAMToFlash(const uint32_t flash_address, const uint32_t ram_address, const uint32_t length) {
    if (AddressLegal(flash_address) && AddressLegal(flash_address+length) &&
      length <= buffer.size()) {
      const status_t status IAP_CopyRamToFlash(dstAddr, uint32_t *srcAddr, uint32_t numOfBytes, uint32_t SystemCoreClock);
      return TranslateStatus(status);
    }
    return Isp::INVALID_ADDR;
  }

  //  FIXME need to store data in buffer
  uint32_t WriteToRam(const uint32_t start, const uint32_t length) {
    if (AddressLegal(start) && AddressLegal(start+length)) {
      const status_t status IAP_CopyRamToFlash(start, end);
      return TranslateStatus(status);
    }
    return Isp::INVALID_ADDR;
  }
#endif


/* RAM address must be within the buffer, destination must not be in bootloader space */
uint32_t CopyRAMToFlash(const uint32_t flash_address, const uint32_t ram_address, const uint32_t length) {
  if (FlashAddressRangeLegal(flash_address, length) && RamRangeLegal(ram_address, length)) {
    const status_t status = IAP_CopyRamToFlash(flash_address, reinterpret_cast<uint32_t*>(&flash.buffer_[ram_address]), length, GetSystemCoreClock());
    return TranslateStatus(status);
  }
  return Isp::ADDR_ERROR;
}

uint32_t WriteToRam(const uint32_t start, const uint32_t length, uint8_t* arg) {
  if (RamAddressLegal(start) && RamAddressLegal(start+length)) {
	for (std::size_t i = 0; i < length; i++) {
	  flash.buffer_[start+i] = arg[i];
	}
    return Isp::CMD_SUCCESS;
  }
  return Isp::ADDR_ERROR;
}

uint32_t unlock(void) {
  flash.Unlock();
  return Isp::CMD_SUCCESS;
}

//  Disable changing of baud rate
uint32_t SetBaudRate(const uint32_t baudrate, const uint32_t stop_bits) {
  return Isp::INVALID_COMMAND;
}

uint32_t ReadMemory(const uint32_t start, const uint32_t length, uint8_t* buff) {
  if (RamAddressLegal(start) && RamAddressLegal(start + length) && length <= Isp::kSectorSize) {
    for (std::size_t i = 0; i < length; i++) {
      buff[i] = flash.buffer_[start+i];
    }
	return Isp::CMD_SUCCESS;
  }
  return Isp::ADDR_ERROR;
}

/* Only allow sectors that are not the bootloader */
uint32_t PrepSectorsForWrite(const uint32_t start, const uint32_t end) {
  if (FlashSectorLegal(start) && FlashSectorLegal(end)) {
    const status_t status = IAP_PrepareSectorForWrite(start, end);
    return TranslateStatus(status);
  }
  return Isp::INVALID_SECTOR_OR_INVALID_PAGE;
}

/* Only allow address to be start of bootloader */
uint32_t Go(const uint32_t address, const char mode) {
  if (address == kBootloaderStart) {
    NVIC_SystemReset();  //  never returns
    return Isp::CMD_SUCCESS;
  }
  return Isp::ADDR_ERROR;
}

/* Same rules as prepare for write */
uint32_t EraseSector(const uint32_t start, const uint32_t end) {
  if (FlashSectorLegal(start) && FlashSectorLegal(end)) {
    const status_t status = IAP_EraseSector(start, end, GetSystemCoreClock());
    return TranslateStatus(status);
  }
  return Isp::INVALID_SECTOR_OR_INVALID_PAGE;
}

/* Same rules as prepare for write */
uint32_t ErasePages(const uint32_t start, const uint32_t end) {
  if (FlashPageLegal(start) && FlashPageLegal(end)) {
    const status_t status = IAP_ErasePage(start, end, GetSystemCoreClock());
    return TranslateStatus(status);
  }
  return Isp::INVALID_SECTOR_OR_INVALID_PAGE;
}

uint32_t ReadPartID(uint32_t* part_id) {
  const status_t status = IAP_ReadPartID(part_id);
  return TranslateStatus(status);
}

uint32_t ReadBootCodeVersion(uint32_t* major, uint32_t* minor) {
  uint32_t version[2]{};
  status_t status = IAP_ReadBootCodeVersion(version);
  *major = version[0];
  *minor = version[1];
  return TranslateStatus(status);
}

uint32_t MemoryLocationsEqual(const uint32_t address1, const uint32_t address2, const uint32_t length) {

  uint32_t* pend = reinterpret_cast<uint32_t*>(address2);
  if (FlashAddressRangeLegal(address1, length) && FlashAddressRangeLegal(address2, length)) {
    const status_t status = IAP_Compare(address1, pend, length);
    return TranslateStatus(status);
  }
  return Isp::ADDR_ERROR;
}

uint32_t ReadUID(std::array<uint32_t, IapController::kSerialNumberWordCount>* uuid) {
  return IapController::ReadSerialNumber(uuid);
}

//  FIXME needs CRC driver
uint32_t ReadCRC(const uint32_t start, const uint32_t length, uint32_t* crc) {
  if (FlashAddressRangeLegal(start, length)) {
    *crc = 0xDEADBEEF;
    return Isp::CMD_SUCCESS;
  } else {
    return Isp::ADDR_ERROR;
  }
}

/* Use to validate image, check only the written image, need to save the signature somewhere. Reserve space at beginning of image for validation values */
uint32_t ReadFlashSig(const uint32_t start, const uint32_t end, const uint32_t wait_states, const uint32_t mode, uint32_t* signature) {
  if (FlashAddressLegal(start) && FlashAddressLegal(end)) {
    const status_t status = IAP_ExtendedFlashSignatureRead(start, end, wait_states, signature);
    return TranslateStatus(status);
  }
  return Isp::ADDR_ERROR;
}
}
