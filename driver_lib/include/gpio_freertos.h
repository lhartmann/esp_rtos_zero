/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <lhartmann@github.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return. Lucas V. Hartmann
 * ----------------------------------------------------------------------------
 * 
 * This is an GPIO interrupt dispatcher using FreeRTOS semaphores.
 * 
 * Usually just a single ISR function can be attached to all GPIOS, which makes
 * it hard to partition code. Using semaphores for each pin sounded more logical
 * at the time I was writing this. <LHartmann>
 */

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
	