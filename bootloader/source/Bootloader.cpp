#include "Bootloader.h"
#include "isp_error_codes.h"
#include <Crc.h>
#include <array>
#include <cstdint>
extern uint32_t SystemCoreClock;

//  FIXME disable interrupts with each of the read/writes to flash
extern uint32_t _vStackTop;
extern uint32_t _image_start;
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
    __disable_irq();

  //  disable peripherals to a known state
  //  Clear pending interrupts
  //  Disable systick and clear all state
  Chip::DefaultSetup();
  
  //  Activate the MSP, if the core is found to currently run with the PSP. As the compiler might still uses the stack, the PSP needs to be copied to the MSP before this
  if (CONTROL_SPSEL_Msk & __get_CONTROL()) {
    /* MSP is not active */
    __set_MSP(__get_PSP());
    __set_CONTROL(__get_CONTROL() & ~CONTROL_SPSEL_Msk);
  }

  //  Load the vector table address of the user application into SCB->VTOR register. Make sure the address meets the alignment requirements
  //  Check for a shadow pointer to the VTOR
  //
  using ApplicationIspResetFP = void (*)(void);
  auto jump_to_app = reinterpret_cast<ApplicationIspResetFP>(*reinterpret_cast<volatile uint32_t*>(image_start + kExecutionOffset));

  __DSB();                                                          /* Ensure all outstanding memory accesses included                                                                       buffered write are completed before reset */
  SCB->VTOR = image_start + kVectorTableOffset;
  __DSB();                                                          /* Ensure completion of memory access */
  //BootJumpASM(stack_pointer, image_start + kExecutionOffset);
  /* Change the main stack pointer. */
  __set_MSP(*reinterpret_cast<volatile uint32_t*>(image_start));
  jump_to_app();
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
  return FlashAddressLegal(start) && FlashAddressLegal(start + length - 1) && length <= Isp::kSectorSize;
}

constexpr inline bool RamAddressLegal(std::size_t address) {
  const uint32_t kIspRamStart = kRamStart + kRamOffset;
  return (address < kIspRamStart + flash.buffer_.size()) && (address >= kIspRamStart);
}

constexpr inline bool RamRangeLegal(std::size_t start, std::size_t length) {
  return RamAddressLegal(start) && RamAddressLegal(start + length - 1) && length <= Isp::kSectorSize && length%sizeof(uint32_t) == 0;
}

uint8_t* GetRamPtr(const uint32_t address) {
  assert(RamAddressLegal(address));
  return reinterpret_cast<uint8_t*>(&(flash.buffer_[address - kRamStart - kRamOffset]));
}

void ExecuteImage(void) {
  // const uint32_t start_address = _image_start;
  const uint32_t start_address = kImageStart;
  const uint32_t stack_pointer = *reinterpret_cast<uint32_t*>(start_address);
  //const uint32_t stack_pointer = _vStackTop;

  BootJump(start_address, stack_pointer);
}

bool ImageIsValid(void) {
  Crc crc_controller;
  crc_controller.SetupCrc32();
  const constexpr uint32_t image_length = (kSectorCount - kBootloaderSectors) * kSectorSize - sizeof(image_signature);
  flash.OpenFlashInteraction();
  uint8_t* p_image = reinterpret_cast<uint8_t* const>(kImageStart);
  crc_controller.WriteData(p_image, image_length);
  assert(image_signature == *reinterpret_cast<volatile uint32_t*>(kSectorCount*kSectorSize - sizeof(uint32_t)));
  flash.CloseFlashInteraction();
  return image_signature == crc_controller.Get32BitResult();
}

/* RAM address must be within the buffer, destination must not be in bootloader space */
uint32_t CopyRAMToFlash(const uint32_t flash_address, const uint32_t ram_address, const uint32_t length) {
  if (FlashAddressRangeLegal(flash_address, length) && RamRangeLegal(ram_address, length)) {
    uint32_t* ram_ptr = reinterpret_cast<uint32_t*>(GetRamPtr(ram_address));
    flash.OpenFlashInteraction();
    const status_t status = IAP_CopyRamToFlash(flash_address, ram_ptr, length, GetSystemCoreClock());
    flash.CloseFlashInteraction();
    return TranslateStatus(status);
  }
  return Isp::ADDR_ERROR;
}


uint32_t ValidateWriteToRam(const uint32_t start, const uint32_t length) {
  if (RamRangeLegal(start, length)) {
    return Isp::CMD_SUCCESS;
  }
  return Isp::ADDR_ERROR;
}

uint32_t WriteToRam(const uint32_t start, const uint32_t length, uint8_t* arg) {
  if (ValidateWriteToRam(start, length) == Isp::CMD_SUCCESS) {
    for (std::size_t i = 0; i < length; i++) {
      *GetRamPtr(start+i) = arg[i];
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
  if (baudrate == 9600) {
    return Isp::CMD_SUCCESS;
  }
  return Isp::INVALID_BAUD_RATE;
}

uint32_t ReadMemory(const uint32_t start, const uint32_t length, uint8_t* buff) {
  if (RamRangeLegal(start, length)) {
    for (std::size_t i = 0; i < length; i++) {
      buff[i] = *GetRamPtr(start+i);
    }
    return Isp::CMD_SUCCESS;
  } else if (FlashAddressRangeLegal(start, length)) {
	for (std::size_t i = 0; i < length; i++) {
	  flash.OpenFlashInteraction();
	  buff[i] = *reinterpret_cast<uint8_t*>(start+i);
	  flash.CloseFlashInteraction();
	}
	return Isp::CMD_SUCCESS;
  }
  return Isp::ADDR_ERROR;
}

/* Only allow sectors that are not the bootloader */
uint32_t PrepSectorsForWrite(const uint32_t start, const uint32_t end) {
  if (FlashSectorLegal(start) && FlashSectorLegal(end)) {
	flash.OpenFlashInteraction();
    const status_t status = IAP_PrepareSectorForWrite(start, end);
    flash.CloseFlashInteraction();
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
    flash.OpenFlashInteraction();
    const status_t status = IAP_EraseSector(start, end, GetSystemCoreClock());
    flash.CloseFlashInteraction();
    return TranslateStatus(status);
  }
  return Isp::INVALID_SECTOR_OR_INVALID_PAGE;
}

/* Same rules as prepare for write */
uint32_t ErasePages(const uint32_t start, const uint32_t end) {
  if (FlashPageLegal(start) && FlashPageLegal(end)) {
    flash.OpenFlashInteraction();
    const status_t status = IAP_ErasePage(start, end, GetSystemCoreClock());
    flash.CloseFlashInteraction();
    return TranslateStatus(status);
  }
  return Isp::INVALID_SECTOR_OR_INVALID_PAGE;
}

uint32_t BlankSectorCheck(const uint32_t start, const uint32_t end) {
  if (FlashSectorLegal(start) && FlashSectorLegal(end)) {
    flash.OpenFlashInteraction();
    const status_t status = IAP_BlankCheckSector(start, end);
    flash.CloseFlashInteraction();
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

uint32_t MemoryLocationsEqual(const uint32_t flash_address, const uint32_t ram_start, const uint32_t length) {

  if (FlashAddressRangeLegal(flash_address, length) && RamRangeLegal(ram_start, length)) {
    flash.OpenFlashInteraction();
    const status_t status = IAP_Compare(flash_address, reinterpret_cast<uint32_t*>(GetRamPtr(ram_start)), length);
    flash.CloseFlashInteraction();
    return TranslateStatus(status);
  }
  return Isp::ADDR_ERROR;
}

uint32_t ReadUID(std::array<uint32_t, IapController::kSerialNumberWordCount>* uuid) {
  return IapController::ReadSerialNumber(uuid);
}

// FIXME this isnt the same as the ISP values for the same
uint32_t ReadCRC(const uint32_t start, const uint32_t length, uint32_t* crc) {
	Crc::SetupCrc32();

  if (FlashAddressRangeLegal(start, length)) {
    Crc::WriteData(reinterpret_cast<uint8_t*>(start), length);
    *crc = Crc::Get32BitResult();
    return Isp::CMD_SUCCESS;
  } else if (RamRangeLegal(start, length)) {
	Crc::WriteData(GetRamPtr(start), length);
	*crc = Crc::Get32BitResult();
	return Isp::CMD_SUCCESS;
  }
  return Isp::ADDR_ERROR;
}

/* Use to validate image, check only the written image, need to save the signature somewhere. Reserve space at beginning of image for validation values */
uint32_t ReadFlashSig(const uint32_t start, const uint32_t end, const uint32_t wait_states, const uint32_t mode, uint32_t* signature) {
  //if ((FlashAddressLegal(start) || FlashAddressInBootloader(start)) && FlashAddressLegal(end)) {
  const status_t status = IAP_ExtendedFlashSignatureRead(start, end, wait_states, signature);
  return TranslateStatus(status);
  //}
  //return Isp::ADDR_ERROR;
}
}  //  namespace Isp
