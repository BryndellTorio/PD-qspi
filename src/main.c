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

// ADDED: for threading
#include <sys/__assert.h>

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

// ADDED: for threading
#define STACKSIZE 1024
#define PRIORITY 7

const uint8_t expected[] = {0xb9};
const size_t len = sizeof(expected);
uint8_t buf[sizeof(expected)];
const struct device *flash_dev;
int rc;

void qspi_loop(void)
{
	printk("\n" FLASH_NAME " SPI flash testing\n");
	printk("==========================\n");

	flash_dev = device_get_binding(FLASH_DEVICE);

	if (!flash_dev)
	{
		printk("SPI flash driver %s was not found!\n",
			   FLASH_DEVICE);
		return;
	}

	while (1)
	{
		memset(buf, 0, len);
		rc = flash_read(flash_dev, FLASH_TEST_REGION_OFFSET, buf, len);
		if (rc != 0)
		{
			printk("Flash read failed! %d\n", rc);
			return;
		}
		printk("buffer: %x\n", buf[0]);
		k_msleep(1000);
	}
}

// K_THREAD_DEFINE should be here for the ON/OFF command to see that qspi_loop_id is defined.
K_THREAD_DEFINE(qspi_loop_id, STACKSIZE, qspi_loop, NULL, NULL, NULL, PRIORITY, 0, 0);

void uart_console(void)
{
	/*CUSTOM CODE.*/
	// initialize console for commands.
	console_getline_init();
	printk(">>> ");

	while (1)
	{
		char *s = console_getline();

		if (strcmp(s, "FLASH PD") == 0)
		{
			rc = flash_write(flash_dev, FLASH_TEST_REGION_OFFSET, expected, len);
			if (rc != 0)
			{
				printk("Failed to write to flash.");
			}
			printk("Deep power-down mode initiated.\n>>> ");
		}
		else if (strcmp(s, "FLASH STANDBY") == 0)
		{
			const uint8_t dummy_load[] = {0x11};
			rc = flash_write(flash_dev, FLASH_TEST_REGION_OFFSET, dummy_load, len);
			if (rc != 0)
			{
				printk("Failed to write to flash.");
			}
			printk("Stanby mode activated.\n>>> ");
		}
		else if (strcmp(s, "FLASH ERASE") == 0)
		{
			rc = flash_erase(flash_dev, FLASH_TEST_REGION_OFFSET, FLASH_SECTOR_SIZE);
			if (rc != 0)
			{
				printk("Flash erase failed! %d\n", rc);
			}
			printk("Erase flash memory.\n>>> ");
		}
		else if (strcmp(s, "READ BUFFER") == 0)
		{
			memset(buf, 0, len);
			rc = flash_read(flash_dev, FLASH_TEST_REGION_OFFSET, buf, len);
			if (rc != 0)
			{
				printk("Flash read failed! %d\n", rc);
				return;
			}
			printk("buffer: %x\n>>> ", buf[0]);
		}
		else if (strcmp(s, "GET-BUFF DEC") == 0)
		{
			printk("Buffer: %d\n>>> ", expected[0]);
		}
		else if (strcmp(s, "GET-BUFF HEX") == 0)
		{
			printk("Buffer: %x\n>>> ", expected[0]);
		}
		else if (strcmp(s, "LOOP ON") == 0)
		{
			k_thread_resume(qspi_loop_id);
			printk("Loop activated.\n");
		}
		else if (strcmp(s, "LOOP OFF") == 0)
		{
			k_thread_suspend(qspi_loop_id);
			printk("Loop deactivated.\n>>> ");
		}
		else
		{
			printk("Command not found.\n>>> ");
		}
	}
}

K_THREAD_DEFINE(uart_console_id, STACKSIZE, uart_console, NULL, NULL, NULL, PRIORITY, 0, 0);