#include "isp_error_codes.h"
#include <array>
namespace Isp {
const constexpr uint32_t kWriteSize = 1024;
uint32_t unlock(void);
uint32_t SetBaudRate(const uint32_t baudrate, const uint32_t stop_bits);
uint32_t WriteToRam(const uint32_t start, const uint32_t length);

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
}
