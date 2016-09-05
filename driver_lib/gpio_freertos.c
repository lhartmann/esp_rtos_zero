#include <driver/gpio.h>
#include <driver/gpio_freertos.h>
#include <string.h>
#include <esp8266/eagle_soc.h>
#include <esp8266/gpio_register.h>

static xSemaphoreHandle sem[16];

typedef void (*gpio_intr_ack_t)(uint32 ack_mask);
gpio_intr_ack_t gpio_intr_ack = (gpio_intr_ack_t)0x40004dcc;

static void gpio_rtos_interrupt_dispatcher() {
	// Read pending interrupts mask
	uint16_t st = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
	uint16_t pin_reg;
	int i;
	portBASE_TYPE woken = pdFALSE;
	
	for (i=0; i<16; ++i) {
		// Skip if no semaphores attached
		if (!sem[i]) continue;
		
		if (st & (1u<<i)) {
			// Interrupt active for pin i, trigger semaphore
			xSemaphoreGiveFromISR(sem[i], &woken);
			
			// Level-triggered interrupts must be disabled or would reenter the ISR.
			// User must re-enable after served.
			if (0) {
			pin_reg = GPIO_REG_READ(GPIO_PIN_ADDR(i));
			switch (GPIO_PIN_SOURCE_GET(pin_reg)) {
			case GPIO_PIN_INTR_LOLEVEL:
			case GPIO_PIN_INTR_HILEVEL:
				pin_reg &= ~GPIO_PIN_SOURCE_MASK;
				pin_reg |= GPIO_PIN_SOURCE_SET(GPIO_PIN_INTR_DISABLE);
				GPIO_REG_WRITE(GPIO_PIN_ADDR(i), pin_reg);
			}
			}
		}
	}
	
	// Clear all the flags just handled
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, 0xFFFFFFFF); //st);
	
	// Trigger scheduler context switch if required
	portEND_SWITCHING_ISR(woken);
}

void gpio_rtos_init() {
	memset(sem, 0, sizeof(sem));
	gpio_intr_handler_register(gpio_rtos_interrupt_dispatcher, 0);
	_xt_isr_unmask(1<<ETS_GPIO_INUM);
}

void gpio_rtos_register(int pin, xSemaphoreHandle semaphore) {
	if (pin < 0 || pin >= 16) return;
	
	vPortEnterCritical();
	sem[pin] = semaphore;
	vPortExitCritical();
}
