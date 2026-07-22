#include "ExternalFileSystem.h"

#include "Adafruit_SPIFlash.h"
#include "flash_devices.h"
#include <SPI.h>

// Defines the memory size used by lfs
#define EXTERNAL_FLASH_PAGE_SIZE 4096
#define EXTERNAL_FLASH_TOTAL_SIZE (1024 * EXTERNAL_FLASH_PAGE_SIZE) // 4MB FLASH

#define EXTERNAL_LFS_BLOCK_SIZE EXTERNAL_FLASH_PAGE_SIZE

#define EXTERNAL_LFS_FLASH__START_ADDR 0

// Onboard external flash P25Q32H
SPIFlash_Device_t const P25Q32H{
    .total_size       = (1UL << 22), // 4MiB
    .start_up_time_us = 5000,

    .manufacturer_id = 0x85,
    .memory_type     = 0x60,
    .capacity        = 0x16,

    .max_clock_speed_mhz         = 104,
    .quad_enable_bit_mask        = 0x02,
    .has_sector_protection       = 1,
    .supports_fast_read          = 1,
    .supports_qspi               = 1,
    .supports_qspi_writes        = 1,
    .write_status_register_split = 1,
    .single_status_byte          = 0,
    .is_fram                     = 0,
};
// Use this constructor to tune the QSPI pins.
// Adafruit_FlashTransport_QSPI flashTransport(PIN_QSPI_SCK, PIN_QSPI_CS, PIN_QSPI_IO0, PIN_QSPI_IO1, PIN_QSPI_IO2, PIN_QSPI_IO3);
Adafruit_FlashTransport_QSPI flashTransport;

Adafruit_SPIFlash flash(&flashTransport);

// LFS BUFFER
#define KEY_VALUE_LFS_CACHE_SIZE 256
#define KEY_VALUE_LFS_LOOKAHEAD_SIZE 128

static __attribute__((aligned(4))) uint8_t s_read_buffer[KEY_VALUE_LFS_CACHE_SIZE];
static __attribute__((aligned(4))) uint8_t s_prog_buffer[KEY_VALUE_LFS_CACHE_SIZE];
static __attribute__((aligned(4))) uint8_t s_lookahead_buffer[KEY_VALUE_LFS_LOOKAHEAD_SIZE];

//--------------------------------------------------------------------+
// LFS Disk IO
//--------------------------------------------------------------------+
static inline uint32_t lba2addr(uint32_t block) {
	return ((uint32_t)EXTERNAL_LFS_FLASH__START_ADDR) + block * EXTERNAL_LFS_BLOCK_SIZE;
}

static int _external_flash_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {
	(void)c;

	uint32_t addr = lba2addr(block) + off;
	flash.readBuffer(addr, (uint8_t *)buffer, size);
	return 0;
}

// Program a region in a block. The block must have previously
// been erased. Negative error codes are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int _external_flash_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {
	(void)c;

	uint32_t addr = lba2addr(block) + off;
	flash.writeBuffer(addr, (uint8_t *)buffer, size);
	return 0;
}

// Erase a block. A block must be erased before being programmed.
// The state of an erased block is undefined. Negative error codes
// are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int _external_flash_erase(const struct lfs_config *c, lfs_block_t block) {
	(void)c;

	uint32_t addr = lba2addr(block);

	// implement as write 0xff to whole block address
	flash.eraseSector(block);
	flash.waitUntilReady();
	return 0;
}

// Sync the state of the underlying block device. Negative error codes
// are propogated to the user.
static int _external_flash_sync(const struct lfs_config *c) {
	(void)c;

	return 0;
}

static struct lfs_config _ExternalFSConfig =
    {
        .context = NULL,

        .read  = _external_flash_read,
        .prog  = _external_flash_prog,
        .erase = _external_flash_erase,
        .sync  = _external_flash_sync,

        .read_size   = KEY_VALUE_LFS_CACHE_SIZE,
        .prog_size   = KEY_VALUE_LFS_CACHE_SIZE,
        .block_size  = EXTERNAL_LFS_BLOCK_SIZE,
        .block_count = EXTERNAL_FLASH_TOTAL_SIZE / EXTERNAL_LFS_BLOCK_SIZE,
        .lookahead   = 128,

        .read_buffer      = s_read_buffer,
        .prog_buffer      = s_prog_buffer,
        .lookahead_buffer = s_lookahead_buffer,
        .file_buffer      = NULL};

ExternalFileSystem ExternalFS;

//--------------------------------------------------------------------+
// Initializes and mounts the file system
//--------------------------------------------------------------------+

ExternalFileSystem::ExternalFileSystem(void)
    : Adafruit_LittleFS(&_ExternalFSConfig) {
}

bool ExternalFileSystem::begin(void) {
	flash.begin(&P25Q32H, 1);
	// failed to mount, erase all sector then format and mount again
	if (!Adafruit_LittleFS::begin()) {
		// Erase all sectors of external flash region for Filesystem.
		for (uint32_t block = EXTERNAL_LFS_FLASH__START_ADDR; block < EXTERNAL_FLASH_TOTAL_SIZE / EXTERNAL_LFS_BLOCK_SIZE; block += 1) {
			flash.eraseSector(block);
		}

		// lfs format
		this->format();

		// mount again if still failed, give up
		if (!Adafruit_LittleFS::begin())
			return false;
	}

	return true;
}

void ExternalFileSystem::flashDeepSleep(void) {
	// LFSをアンマウント
	Adafruit_LittleFS::end();

	// Deep Power-Down (0xB9)
	flashTransport.begin();
	flashTransport.runCommand(0xB9);
	delayMicroseconds(5); // tDP=3us (P25Q32H)
	flashTransport.end();
}

void ExternalFileSystem::flashWakeUp(void) {
	// Release from Deep Power-Down (0xAB)
	flashTransport.begin();
	flashTransport.runCommand(0xAB);
	delayMicroseconds(20); // tRES1 + tRES2 (8us + 8us)
	flashTransport.end();

	// 再マウント
	begin();
}