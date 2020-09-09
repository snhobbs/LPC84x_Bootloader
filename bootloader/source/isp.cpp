#include "nxpisp/isp.h"
#include "IAP.h"
#include "Crc.h"

//  FIXME disable interrupts with each of the read/writes to flash
namespace Isp {
uint32_t unlock(void) { return Isp::CMD_SUCCESS; }
uint32_t SetBaudRate(const uint32_t baudrate, const uint32_t stop_bits) { return Isp::CMD_SUCCESS; }
uint32_t WriteToRam(const uint32_t start, const uint32_t length) { return Isp::CMD_SUCCESS; }

template<>
uint32_t ReadMemory<std::array<uint8_t, Shell::kMaximumReadWriteLength>>(const uint32_t start, const uint32_t length, std::array<uint8_t, Shell::kMaximumReadWriteLength>& buffer) { return Isp::CMD_SUCCESS; }

/* Only allow sectors that are not the bootloader */
uint32_t PrepSectorsForWrite(const uint32_t start, const uint32_t end) {
  return Isp::CMD_SUCCESS;
}
/* RAM address must be within the buffer, destination must not be in bootloader space */
uint32_t CopyRAMToFlash(const uint32_t flash_address, const uint32_t ram_address, const uint32_t length) { return Isp::CMD_SUCCESS; }

/* Only allow address to be start of bootloader */
uint32_t Go(const uint32_t address, const char mode) {
  NVIC_SystemReset();  //  never returns
  return Isp::CMD_SUCCESS;
}

/* Same rules as prepare for write */
uint32_t EraseSector(const uint32_t start, const uint32_t end) { return Isp::CMD_SUCCESS; }

/* Same rules as prepare for write */
uint32_t ErasePages(const uint32_t start, const uint32_t end) { return Isp::CMD_SUCCESS; }

uint32_t ReadPartID(uint32_t* part_id) {
  return IAP_ReadPartID(part_id);
}

uint32_t ReadBootCodeVersion(uint32_t* major, uint32_t* minor) {
  uint32_t version[2]{};
  status_t ret = IAP_ReadBootCodeVersion(version);
  *major = version[0];
  *minor = version[1];
  return ret;
}

uint32_t MemoryLocationsEqual(const uint32_t address1, const uint32_t address2, const uint32_t length) {
  return Isp::CMD_SUCCESS;
}

uint32_t ReadUID(std::array<uint32_t, IspController::kSerialNumberWordCount>* uuid) {
  return IspController::ReadSerialNumber(uuid);
}

uint32_t ReadCRC(const uint32_t start, const uint32_t length, uint32_t* crc) {
  *crc = 0xDEADBEEF; 
  return Isp::CMD_SUCCESS;
}

/* Use to validate image, check only the written image, need to save the signature somewhere. Reserve space at beginning of image for validation values */
uint32_t ReadFlashSig(const uint32_t start, const uint32_t end, const uint32_t wait_states, const uint32_t mode, uint32_t* signature) {
  return IAP_ExtendedFlashSignatureRead(start, end, wait_states, signature);
}

}
