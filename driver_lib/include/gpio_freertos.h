#ifndef GPIO_FREERTOS_H
#define GPIO_FREERTOS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

/**  
  * @brief   Installs a FreeRTOS handler for GPIO interrupts.
  * 
  * @return  null
  */
void gpio_rtos_init();

/**  
  * @brief   Sets a semaphore to receive interrupt signals.
  * 
  * @param   pin : GPIO pin number for which the semaphore is to be used.
  * @param   bit_value : Semaphore that will be signaled on interrupt (0 to disable).
  *  
  * @return  null
  */
void gpio_rtos_register(int pin, xSemaphoreHandle semaphore);

#ifdef __cplusplus
}
#endif

#endif
	