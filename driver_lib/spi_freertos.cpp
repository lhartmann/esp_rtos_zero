#include <driver/spi_freertos.hpp>

xSemaphoreHandle HSPI_Mutex;

void spi_rtos_init(bool hw_cs) {
	// Only HSPI is supported for multiplexed access on FreeRTOS.
	spi_init(HSPI, hw_cs);

	HSPI_Mutex = xSemaphoreCreateMutex();
}
