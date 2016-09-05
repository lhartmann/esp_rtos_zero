/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <lhartmann@github.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return. Lucas V. Hartmann
 * ----------------------------------------------------------------------------
 */

#include <driver/spi_freertos.hpp>

xSemaphoreHandle HSPI_Mutex;

void spi_rtos_init(bool hw_cs) {
	// Only HSPI is supported for multiplexed access on FreeRTOS.
	spi_init(HSPI, hw_cs);

	HSPI_Mutex = xSemaphoreCreateMutex();
}
