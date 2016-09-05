/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <lhartmann@github.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return. Lucas V. Hartmann
 * ----------------------------------------------------------------------------
 * 
 * This is sharing engine for HSPI on ESP82xx with FreeRTOS.
 * 
 *  <LHartmann>
 */

#ifndef SPI_FREERTOS_HPP
#define SPI_FREERTOS_HPP

#include <driver/gpio.h>
#include <driver/spi.h>
#include <string.h>
#include <esp8266/eagle_soc.h>
#include <esp8266/gpio_register.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

/// @brief HSPI mutex. Yes, really!
extern xSemaphoreHandle HSPI_Mutex;

/**
 * @brief SPI-RTOS driver initialization
 */
extern "C" void spi_rtos_init(bool hw_cs);

/**
 * @brief SPI Sharing class with software CS. Only HSPI supported.
 * 
 * Multiple instances ARE supported, just set the CS pins/masks appropriatelly.
 * 
 * All transaction functions implicitly acquire/release the SPI and activate CS as
 * required. begin() and end() are optional, and may be used for grouping several
 * transfers as a single, indivisible SPI transaction.
 */
class HSPI_FreeRTOS {
	// GPIOs to change on take/give
	uint16_t sot, sog, cot, cog;

	// Recursive locking may cause overhead if shared across files.
	// Make sure to keep the instance local, and enable compiler optimization.
	int lockdepth;

	public:
	/** @brief Generic constructor.
	 * 
	 * @param set_on_take : GPIO bit mask of bits to be set when the SPI lock is acquired.
	 * @param clr_on_take : GPIO bit mask of bits to be cleared when the SPI lock is acquired.
	 * @param set_on_give : GPIO bit mask of bits to be set when the SPI lock is released.
	 * @param clr_on_give : GPIO bit mask of bits to be cleared when the SPI lock is released.
	 * 
	 * This function m ay be useful if multiple bits need changing for correct CS operation.
	 * This allows for a 74HC138 demux to route of a hardware CS.
	 */
	HSPI_FreeRTOS(
		uint16_t set_on_take,
		uint16_t clr_on_take,
		uint16_t set_on_give,
		uint16_t clr_on_give
	) : sot(set_on_take), cot(clr_on_take), sog(set_on_give), cog(clr_on_give), lockdepth(0) {}

	/**
	 * @brief Simplified constructor.
	 *
	 * @param cs_pin : Number of the GPIO pin (0-15) to be used as an active-low CS.
	 * 
	 * Simplified constructor for individually connected software CS pins.
	 */
	HSPI_FreeRTOS(uint16_t cs_pin) : sot(0), cot(1<<cs_pin), sog(1<<cs_pin), cog(0), lockdepth(0) {}

	/**
	 * @brief Allows de-asserting chip-select handling without releasing the Mutex.
	 * 
	 * @param assert : Set to true to activate the target, false to deactivate.
	 */
	void chip_select(bool assert) {
		if (assert) {
			// Activate chip select
			// Set before clear, since most CS are active low
			GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, sog);
			GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, cot);
		} else {
			// Deactivate chip select
			// Set before clear, since most CS are active low
			GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, sog);
			GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, cog);
		}
	}

	/**
	 * @brief Acquires the lock for the SPI port and activates the target.
	 * 
	 * Together with end(), may be used to group a set of reads and writes to a
	 * single, indivisible SPI operation. May be called recursively, CS is only
	 * activated on the first lock.
	 */
	void begin() {
		if (!lockdepth) {
			xSemaphoreTake(HSPI_Mutex, portMAX_DELAY);
			chip_select(true);
		}
		lockdepth++;
	}

	/**
	 * @brief Deactivates CS and releases the lock on the SPI port.
	 * 
	 * May be called recursively, CS is only deactivated on the last unlock.
	 */
	void end() {
		lockdepth--;
		if (!lockdepth) {
			wait(); // Wait until transmit buffer is sent.
			chip_select(false);
			xSemaphoreGive(HSPI_Mutex);
		}
	}

	/////// Transaction functions ///////

	/**
	 * @brief Full transaction.
	 */
	uint32_t operator() (
		uint8 cmd_bits, uint16 cmd_data,
		uint32 addr_bits, uint32 addr_data,
		uint32 dout_bits, uint32 dout_data,
		uint32 din_bits, uint32 dummy_bits
	) {
		uint32_t r;
		begin();
		r = spi_transaction(HSPI,
			cmd_bits, cmd_data,
			addr_bits, addr_data, 
			dout_bits, dout_data,
			din_bits, dummy_bits
		);
		end();
		return r;
	}

	/// @brief Send a byte.
	void tx8 (uint8_t  x) {
		begin();
		spi_tx8 (HSPI, x);
		end();
	}
	/// @brief Send half-word.
	void tx16(uint16_t x) {
		begin();
		spi_tx16(HSPI, x);
		end();
	}
	/// @brief Send a word.
	void tx32(uint32_t x) {
		begin();
		spi_tx32(HSPI, x);
		end();
	}

	/// @brief Read a byte.
	uint8_t rx8() {
		begin();
		uint8_t r = spi_rx8(HSPI);
		end();
		return r;
	}
	/// @brief Read half-word.
	uint16_t rx16() {
		begin();
		uint16_t r = spi_rx16(HSPI);
		end();
		return r;
	}
	/// @brief Read a word.
	uint32_t rx32() {
		begin();
		uint32_t r = spi_rx32(HSPI);
		end();
		return r;
	}

	/// @brief Waint until SPI operations have completed.
	void wait() {
		while (spi_busy(HSPI));
	}
};

#endif
