#include "isp.h"
#include "IAP.h"
#include "Crc.h"
#include <shell/shell.h>
#include "SystemConstants.h"
#include <array>
//  FIXME disable interrupts with each of the read/writes to flash
extern uint32_t SystemCoreClock;
namespace Isp {

static const constexpr uint32_t kBootloaderStart = 0x00;
class FlashController {
  static const constexpr uint32_t kRamBufferLength = 1024; // FIXME
  std::array<uint32_t, kRamBufferLength> buffer{};
  bool lock_ = true;
  
 public:
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
};

static FlashController flash;
/* RAM address must be within the buffer, destination must not be in bootloader space */
uint32_t CopyRAMToFlash(const uint32_t flash_address, const uint32_t ram_address, const uint32_t length) {
#if 0
  if (AddressLegal(flash_address) && AddressLegal(flash_address+length) &&
    length <= kRamBufferLength) {
    const status_t status IAP_CopyRamToFlash(dstAddr, uint32_t *srcAddr, uint32_t numOfBytes, uint32_t SystemCoreClock);
    return TranslateStatus(status);
  }
#endif
  return Isp::ADDR_ERROR;
}

//  FIXME need to store data in buffer
uint32_t WriteToRam(const uint32_t start, const uint32_t length) {
#if 0
  if (AddressLegal(start) && AddressLegal(start+length)) {
    const status_t status IAP_CopyRamToFlash(start, end);
    return TranslateStatus(status);
  }
#endif
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

//FIXME add to controller, needs to be trickled out
template<>
uint32_t ReadMemory<std::array<uint8_t, Shell::kMaximumReadWriteLength>>(const uint32_t start, const uint32_t length, std::array<uint8_t, Shell::kMaximumReadWriteLength>& buffer) {
  return Isp::CMD_SUCCESS;
}

/* Only allow sectors that are not the bootloader */
uint32_t PrepSectorsForWrite(const uint32_t start, const uint32_t end) {
  if (SectorLegal(start) && SectorLegal(end)) {
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
//  FIXME needs to read core clock
uint32_t EraseSector(const uint32_t start, const uint32_t end) {
  if (SectorLegal(start) && SectorLegal(end)) {
    const status_t status = IAP_EraseSector(start, end, SystemCoreClock);
    return TranslateStatus(status);
  }
  return Isp::INVALID_SECTOR_OR_INVALID_PAGE;
}

/* Same rules as prepare for write */
//  FIXME needs to read core clock
uint32_t ErasePages(const uint32_t start, const uint32_t end) {
  if (PageLegal(start) && PageLegal(end)) {
    const status_t status = IAP_ErasePage(start, end, SystemCoreClock);
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
  if (AddressLegal(address1) && AddressLegal(address1 + length) &&
      AddressLegal(address2) && AddressLegal(address2 + length)) {
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
  if (AddressLegal(start) && AddressLegal(start + length)) {
    *crc = 0xDEADBEEF; 
    return Isp::CMD_SUCCESS;
  } else {
    return Isp::ADDR_ERROR;
  }
}

/* Use to validate image, check only the written image, need to save the signature somewhere. Reserve space at beginning of image for validation values */
uint32_t ReadFlashSig(const uint32_t start, const uint32_t end, const uint32_t wait_states, const uint32_t mode, uint32_t* signature) {
  if (AddressLegal(start) && AddressLegal(end)) {
    const status_t status = IAP_ExtendedFlashSignatureRead(start, end, wait_states, signature);
    return TranslateStatus(status);
  }
  return Isp::ADDR_ERROR;
}
}
