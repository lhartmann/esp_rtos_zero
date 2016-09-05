#ifndef SPI_FREERTOS_HPP
#define SPI_FREERTOS_HPP

#include <driver/gpio.h>
#include <driver/spi.h>
#include <string.h>
#include <esp8266/eagle_soc.h>
#include <esp8266/gpio_register.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

extern xSemaphoreHandle HSPI_Mutex;

// Global driver initialization
extern "C" void spi_rtos_init(bool hw_cs);

// SPI Sharing class with software CS. Only HSPI supported.
class HSPI_FreeRTOS {
	// GPIOs to change on take/give
	uint16_t sot, sog, cot, cog;

	// Recursive locking may cause overhead if shared across files.
	// Make sure to keep the instance local, and enable compiler optimization.
	int lockdepth;

	public:
	// Generic constructor, good for 74HC138 routing of a hardware/software CS.
	HSPI_FreeRTOS(
		uint16_t set_on_take,
		uint16_t clr_on_take,
		uint16_t set_on_give,
		uint16_t clr_on_give
	) : sot(set_on_take), cot(clr_on_take), sog(set_on_give), cog(clr_on_give), lockdepth(0) {}

	// Simplified constructor, good for independent CS pins for each slave.
	HSPI_FreeRTOS(uint16_t cs_pin) : sot(0), cot(1<<cs_pin), sog(1<<cs_pin), cog(0), lockdepth(0) {}

	// Allows de-asserting chip-select handling without releasing the Mutex.
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

	void begin() {
		if (!lockdepth) {
			xSemaphoreTake(HSPI_Mutex, portMAX_DELAY);
			chip_select(true);
		}
		lockdepth++;
	}
	void end() {
		lockdepth--;
		if (!lockdepth) {
			wait(); // Wait until transmit buffer is sent.
			chip_select(false);
			xSemaphoreGive(HSPI_Mutex);
		}
	}

	/////// Transaction functions ///////

	// Full transaction, use class as a function-object
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

	// TX operation.
	void tx8 (uint8_t  x) {
		begin();
		spi_tx8 (HSPI, x);
		end();
	}
	void tx16(uint16_t x) {
		begin();
		spi_tx16(HSPI, x);
		end();
	}
	void tx32(uint32_t x) {
		begin();
		spi_tx32(HSPI, x);
		end();
	}

	// RX operations.
	uint8_t rx8() {
		begin();
		uint8_t r = spi_rx8(HSPI);
		end();
		return r;
	}
	uint16_t rx16() {
		begin();
		uint16_t r = spi_rx16(HSPI);
		end();
		return r;
	}
	uint32_t rx32() {
		begin();
		uint32_t r = spi_rx32(HSPI);
		end();
		return r;
	}

	// Waint until SPI operations have completed.
	void wait() {
		while (spi_busy(HSPI));
	}
};

#endif
