/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2010 Perry Hung.
 * Copyright (c) 2011, 2012 LeafLabs, LLC.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *****************************************************************************/

/**
 * @file wirish/HardwareSerial.cpp
 * @brief Wirish serial port implementation.
 */

#include "HardwareSerial.h"

#include <libmaple/libmaple.h>
#include <libmaple/gpio.h>
#include <libmaple/timer.h>
#include <libmaple/usart.h>

HardwareSerial::HardwareSerial(usart_dev *usart_device,
                               uint8 tx_pin,
                               uint8 rx_pin)
{
	this->usart_device = usart_device;
	this->tx_pin = tx_pin;
	this->rx_pin = rx_pin;
}

/*
 * Set up/tear down
 */

/* F1 MCUs have no GPIO_AFR[HL], so turn off PWM if there's a conflict
 * on this GPIO bit. */
static void disable_timer_if_necessary(timer_dev *dev, uint8 ch)
{
	if (dev != NULL) {
		timer_set_mode(dev, ch, TIMER_DISABLED);
	}
}


void HardwareSerial::begin(uint32 baud)
{
	begin(baud, SERIAL_8N1);
}

void HardwareSerial::begin(uint32 baud, uint8_t config)
{
	// ASSERT(baud <= this->usart_device->max_baud);
	if (baud > this->usart_device->max_baud) {
		return;
	}

	const stm32_pin_info *txi = &PIN_MAP[this->tx_pin];
	const stm32_pin_info *rxi = &PIN_MAP[this->rx_pin];

	disable_timer_if_necessary(txi->timer_device, txi->timer_channel);

	usart_init(this->usart_device);
	usart_config_gpios_async(this->usart_device,
	                         rxi->gpio_device, rxi->gpio_bit,
	                         txi->gpio_device, txi->gpio_bit,
	                         config);
	usart_set_baud_rate(this->usart_device, USART_USE_PCLK, baud);
	usart_enable(this->usart_device);
}

void HardwareSerial::end(void)
{
	usart_disable(this->usart_device);
}

/*
 * I/O
 */
int HardwareSerial::read(void)
{
	// Block until a byte becomes available, to save user confusion.
	while (!this->available());
	return usart_getc(this->usart_device);
}

int HardwareSerial::available(void)
{
	return usart_data_available(this->usart_device);
}

int HardwareSerial::peek(void)
{
	return usart_peek(this->usart_device);
}

int HardwareSerial::availableForWrite(void)
{
	return 1;
}

size_t HardwareSerial::write(unsigned char ch)
{
	usart_putc(this->usart_device, ch);
	return 1;
}

void HardwareSerial::flush(void)
{
	usart_reset_rx(this->usart_device);
}
