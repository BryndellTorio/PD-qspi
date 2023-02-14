/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <drivers/flash.h>
#include <device.h>
#include <devicetree.h>
#include <stdio.h>
#include <string.h>

// ADDED: for Console controls
#include <string.h>
#include <zephyr.h>
#include <sys/printk.h>
#include <console/console.h>
#include <ctype.h>

#if (CONFIG_SPI_NOR - 0) ||				\
	DT_NODE_HAS_STATUS(DT_INST(0, jedec_spi_nor), okay)
#define FLASH_DEVICE DT_LABEL(DT_INST(0, jedec_spi_nor))
#define FLASH_NAME "JEDEC SPI-NOR"
#elif (CONFIG_NORDIC_QSPI_NOR - 0) || \
	DT_NODE_HAS_STATUS(DT_INST(0, nordic_qspi_nor), okay)
#define FLASH_DEVICE DT_LABEL(DT_INST(0, nordic_qspi_nor))
#define FLASH_NAME "JEDEC QSPI-NOR"
#elif DT_NODE_HAS_STATUS(DT_INST(0, st_stm32_qspi_nor), okay)
#define FLASH_DEVICE DT_LABEL(DT_INST(0, st_stm32_qspi_nor))
#define FLASH_NAME "JEDEC QSPI-NOR"
#else
#error Unsupported flash driver
#endif

#if defined(CONFIG_BOARD_ADAFRUIT_FEATHER_STM32F405)
#define FLASH_TEST_REGION_OFFSET 0xf000
#elif defined(CONFIG_BOARD_ARTY_A7_ARM_DESIGNSTART_M1) || \
	defined(CONFIG_BOARD_ARTY_A7_ARM_DESIGNSTART_M3)
/* The FPGA bitstream is stored in the lower 536 sectors of the flash. */
#define FLASH_TEST_REGION_OFFSET \
	DT_REG_SIZE(DT_NODE_BY_FIXED_PARTITION_LABEL(fpga_bitstream))
#else
#define FLASH_TEST_REGION_OFFSET 0xff000
#endif
#define FLASH_SECTOR_SIZE        4096


void main(void)
{
	const uint8_t expected[] = {0xb9};
	const size_t len = sizeof(expected);
	uint8_t buf[sizeof(expected)];
	const struct device *flash_dev;
	int rc;
	printk("\n" FLASH_NAME " SPI flash testing\n");
	printk("==========================\n");

	flash_dev = device_get_binding(FLASH_DEVICE);

	if (!flash_dev)
	{
		printk("SPI flash driver %s was not found!\n",
			   FLASH_DEVICE);
		return;
	}

	/* Write protection needs to be disabled before each write or
	 * erase, since the flash component turns on write protection
	 * automatically after completion of write and erase
	 * operations.
	 */
	printk("\nTest 1: Flash erase\n");

	rc = flash_erase(flash_dev, FLASH_TEST_REGION_OFFSET,
					 FLASH_SECTOR_SIZE);
	if (rc != 0)
	{
		printk("Flash erase failed! %d\n", rc);
	}
	else
	{
		printk("Flash erase succeeded!\n");
	}

	printk("\nTest 2: Flash write\n");

	printk("Attempting to write %zu bytes\n", len);
	rc = flash_write(flash_dev, FLASH_TEST_REGION_OFFSET, expected, len);
	if (rc != 0)
	{
		printk("Flash write failed! %d\n", rc);
		return;
	}

	memset(buf, 0, len);
	rc = flash_read(flash_dev, FLASH_TEST_REGION_OFFSET, buf, len);
	if (rc != 0)
	{
		printk("Flash read failed! %d\n", rc);
		return;
	}

	if (memcmp(expected, buf, len) == 0)
	{
		printk("Data read matches data written. Good!!\n");
	}
	else
	{
		const uint8_t *wp = expected;
		const uint8_t *rp = buf;
		const uint8_t *rpe = rp + len;

		printk("Data read does not match data written!!\n");
		while (rp < rpe)
		{
			printk("%08x wrote %02x read %02x %s\n",
				   (uint32_t)(FLASH_TEST_REGION_OFFSET + (rp - buf)),
				   *wp, *rp, (*rp == *wp) ? "match" : "MISMATCH");
			++rp;
			++wp;
		}
	}
	/*CUSTOM CODE.*/
	// initialize console for commands.
	console_getline_init();
	printk(">>> ");

	while (1)
	{
		char *s = console_getline();

		if (strcmp(s, "ON") == 0)
		{
			rc = flash_write(flash_dev, FLASH_TEST_REGION_OFFSET, expected, len);
			if (rc != 0)
			{
				printk("Failed to write to flash.");
			}
			printk("Deep power-down mode initiated.\n>>> ");
		}
		else if (strcmp(s, "OFF") == 0)
		{
			printk("Standby mode initiated.\n>>> ");
		}
		else if (strcmp(s, "READ BUFFER") == 0)
		{
			// Insert code to print the buffer content into the console.
		}
		else if (strcmp(s, "GET-BUFF DEC") == 0)
		{
			printk("Buffer: %d\n>>> ", expected[0]);
		}
		else if (strcmp(s, "GET-BUFF HEX") == 0)
		{
			printk("Buffer: %x\n>>> ", expected[0]);
		}
		else
		{
			printk("Command not found.\n>>> ");
		}
	}
	/*END OF CODE.*/
}