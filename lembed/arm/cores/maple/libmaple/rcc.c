/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2010 Perry Hung.
 * Copyright (c) 2011 LeafLabs, LLC.
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
 * @file libmaple/rcc.c
 * @brief Portable RCC routines.
 */

#include <libmaple/rcc.h>

#include "rcc_private.h"

#include <libmaple/libmaple.h>
#include <libmaple/bitband.h>


#define APB1                            RCC_APB1
#define APB2                            RCC_APB2
#define AHB                             RCC_AHB

/* Device descriptor table, maps rcc_clk_id onto bus and enable/reset
 * register bit numbers. */
const struct rcc_dev_info rcc_dev_table[] = {
    [RCC_GPIOA]  = { .clk_domain = APB2, .line_num = 2 },
    [RCC_GPIOB]  = { .clk_domain = APB2, .line_num = 3 },
    [RCC_GPIOC]  = { .clk_domain = APB2, .line_num = 4 },
    [RCC_GPIOD]  = { .clk_domain = APB2, .line_num = 5 },
    [RCC_AFIO]   = { .clk_domain = APB2, .line_num = 0 },
    [RCC_ADC1]   = { .clk_domain = APB2, .line_num = 9 },
    [RCC_ADC2]   = { .clk_domain = APB2, .line_num = 10 },
    [RCC_ADC3]   = { .clk_domain = APB2, .line_num = 15 },
    [RCC_USART1] = { .clk_domain = APB2, .line_num = 14 },
    [RCC_USART2] = { .clk_domain = APB1, .line_num = 17 },
    [RCC_USART3] = { .clk_domain = APB1, .line_num = 18 },
    [RCC_TIMER1] = { .clk_domain = APB2, .line_num = 11 },
    [RCC_TIMER2] = { .clk_domain = APB1, .line_num = 0 },
    [RCC_TIMER3] = { .clk_domain = APB1, .line_num = 1 },
    [RCC_TIMER4] = { .clk_domain = APB1, .line_num = 2 },
    [RCC_SPI1]   = { .clk_domain = APB2, .line_num = 12 },
    [RCC_SPI2]   = { .clk_domain = APB1, .line_num = 14 },
    [RCC_DMA1]   = { .clk_domain = AHB,  .line_num = 0 },
    [RCC_PWR]    = { .clk_domain = APB1, .line_num = 28},
    [RCC_BKP]    = { .clk_domain = APB1, .line_num = 27},
    [RCC_I2C1]   = { .clk_domain = APB1, .line_num = 21 },
    [RCC_I2C2]   = { .clk_domain = APB1, .line_num = 22 },
    [RCC_CRC]    = { .clk_domain = AHB,  .line_num = 6},
    [RCC_FLITF]  = { .clk_domain = AHB,  .line_num = 4},
    [RCC_SRAM]   = { .clk_domain = AHB,  .line_num = 2},
    [RCC_USB]    = { .clk_domain = APB1, .line_num = 23},
#if defined(STM32_HIGH_DENSITY) || defined(STM32_XL_DENSITY)
    [RCC_GPIOE]  = { .clk_domain = APB2, .line_num = 6 },
    [RCC_GPIOF]  = { .clk_domain = APB2, .line_num = 7 },
    [RCC_GPIOG]  = { .clk_domain = APB2, .line_num = 8 },
    [RCC_UART4]  = { .clk_domain = APB1, .line_num = 19 },
    [RCC_UART5]  = { .clk_domain = APB1, .line_num = 20 },
    [RCC_TIMER5] = { .clk_domain = APB1, .line_num = 3 },
    [RCC_TIMER6] = { .clk_domain = APB1, .line_num = 4 },
    [RCC_TIMER7] = { .clk_domain = APB1, .line_num = 5 },
    [RCC_TIMER8] = { .clk_domain = APB2, .line_num = 13 },
    [RCC_FSMC]   = { .clk_domain = AHB,  .line_num = 8 },
    [RCC_DAC]    = { .clk_domain = APB1, .line_num = 29 },
    [RCC_DMA2]   = { .clk_domain = AHB,  .line_num = 1 },
    [RCC_SDIO]   = { .clk_domain = AHB,  .line_num = 10 },
    [RCC_SPI3]   = { .clk_domain = APB1, .line_num = 15 },
#endif
#ifdef STM32_XL_DENSITY
    [RCC_TIMER9]  = { .clk_domain = APB2, .line_num = 19 },
    [RCC_TIMER10] = { .clk_domain = APB2, .line_num = 20 },
    [RCC_TIMER11] = { .clk_domain = APB2, .line_num = 21 },
    [RCC_TIMER12] = { .clk_domain = APB1, .line_num = 6 },
    [RCC_TIMER13] = { .clk_domain = APB1, .line_num = 7 },
    [RCC_TIMER14] = { .clk_domain = APB1, .line_num = 8 },
#endif
};

/**
 * @brief Get a peripheral's clock domain
 * @param id Clock ID of the peripheral whose clock domain to return
 * @return Clock source for the given clock ID
 */
rcc_clk_domain rcc_dev_clk(rcc_clk_id id)
{
    return rcc_dev_table[id].clk_domain;
}

/**
 * @brief Switch the clock used as the source of the system clock.
 *
 * After switching the source, this function blocks until the new
 * clock source is in use.
 *
 * @param sysclk_src New system clock source.
 * @see rcc_sysclk_src
 */
void rcc_switch_sysclk(rcc_sysclk_src sysclk_src)
{
    uint32 cfgr = RCC_BASE->CFGR;
    cfgr &= ~RCC_CFGR_SW;
    cfgr |= sysclk_src;

    /* Switch SYSCLK source. */
    RCC_BASE->CFGR = cfgr;

    /* Wait for new source to come into use. */
    while ((RCC_BASE->CFGR & RCC_CFGR_SWS) != (sysclk_src << 2))
        ;
}

/*
 * Turning clocks off and on, querying their status.
 */

/* IMPORTANT NOTE FOR IMPLEMENTORS:
 *
 * libmaple assumes that enum rcc_clk enumerators are two-byte
 * values, stored in a uint16, in the following way:
 *
 *  - The high-order byte is the byte offset (from RCC_BASE) of the register
 *    to touch when turning on or off the given clock.
 *
 *  - The low-order byte is the bit in that register that turns the
 *    clock on or off.
 *
 * Example for STM32F1: Turning on the high-speed external clock (HSE)
 * involves setting HSEON, bit 16, of RCC_CR. The high-order byte is
 * then offsetof(struct rcc_reg_map, CR) = 0, and the low-order byte
 * is 16.
 *
 * The corresponding value of RCC_CLK_HSE is thus (0 << 8) | 16 = 16.
 *
 * On all known STM32 series, this encoding has the property that
 * adding one to the low byte also gives the bit to check to determine
 * if the clock is ready. For example, on STM32F1, RCC_CR_HSERDY is
 * bit 17. If that's not the case on your series, rcc_is_clk_ready()
 * won't work for you. */

/* Returns the RCC register which controls the clock source. */
static inline __io uint32* rcc_clk_reg(rcc_clk clock)
{
    return (__io uint32*)((__io uint8*)RCC_BASE + (clock >> 8));
}

/* Returns a mask in rcc_clk_reg(clock) to be used for turning the
 * clock on and off */
static inline uint32 rcc_clk_on_mask(rcc_clk clock)
{
    return 1 << (clock & 0xFF);
}

/* Returns a mask in rcc_clk_reg(clock) to be used when checking the
 * readiness of the clock. */
static inline uint32 rcc_clk_ready_mask(rcc_clk clock)
{
    return rcc_clk_on_mask(clock) << 1;
}

/**
 * @brief Turn on a clock source.
 *
 * After this routine exits, callers should ensure that the clock
 * source is ready by waiting until rcc_is_clk_ready(clock) returns
 * true.
 *
 * @param clock Clock to turn on.
 * @see rcc_turn_off_clk()
 * @see rcc_is_clk_ready()
 */
void rcc_turn_on_clk(rcc_clk clock)
{
    *rcc_clk_reg(clock) |= rcc_clk_on_mask(clock);
}

/**
 * @brief Turn off a clock source.
 *
 * In certain configurations, certain clock sources cannot be safely
 * turned off. (For example, the main PLL on STM32F1 devices cannot be
 * turned off if it has been selected as the SYSCLK source). Consult
 * the reference material for your MCU to ensure it is safe to call
 * this function.
 *
 * @param clock Clock to turn off.
 * @see rcc_turn_on_clk()
 * @see rcc_is_clk_ready()
 */
void rcc_turn_off_clk(rcc_clk clock)
{
    *rcc_clk_reg(clock) &= ~rcc_clk_on_mask(clock);
}

/**
 * @brief Check if a clock is on.
 * @param clock Clock to check.
 * @return 1 if the clock is on, 0 if the clock is off.
 */
int rcc_is_clk_on(rcc_clk clock)
{
    return !!(*rcc_clk_reg(clock) & rcc_clk_on_mask(clock));
}

/**
 * @brief Check if a clock source is ready.
 *
 * In general, it is not safe to rely on a clock source unless this
 * function returns nonzero. Also note that this function may return
 * nonzero for a short period of time after a clock has been turned
 * off. Consult the reference material for your MCU for more details.
 *
 * @param clock Clock whose readiness to check for.
 * @return Nonzero if the clock is ready, zero otherwise.
 * @see rcc_turn_on_clk()
 * @see rcc_turn_off_clk()
 */
int rcc_is_clk_ready(rcc_clk clock)
{
    return (int)(*rcc_clk_reg(clock) & rcc_clk_ready_mask(clock));
}

__deprecated
void rcc_clk_init(rcc_sysclk_src sysclk_src,
                  rcc_pllsrc pll_src,
                  rcc_pll_multiplier pll_mul)
{
    /* Assume that we're going to clock the chip off the PLL, fed by
     * the HSE */
    ASSERT(sysclk_src == RCC_CLKSRC_PLL &&
           pll_src    == RCC_PLLSRC_HSE);

    RCC_BASE->CFGR = pll_src | pll_mul | (0x3 << 22);

    /* Turn on, and wait for, HSE. */
    rcc_turn_on_clk(RCC_CLK_HSE);
    while (!rcc_is_clk_ready(RCC_CLK_HSE))
        ;

    /* Do the same for the main PLL. */
    rcc_turn_on_clk(RCC_CLK_PLL);
    while (!rcc_is_clk_ready(RCC_CLK_PLL))
        ;

    /* Finally, switch over to the PLL. */
    rcc_switch_sysclk(RCC_CLKSRC_PLL);
}

/* pll_cfg->data must point to a valid struct stm32f1_rcc_pll_data. */
void rcc_configure_pll(rcc_pll_cfg *pll_cfg)
{
    stm32f1_rcc_pll_data *data = pll_cfg->data;
    rcc_pll_multiplier pll_mul = data->pll_mul;
    uint32 cfgr;
    /* Check that the PLL is disabled. */
    ASSERT_FAULT(!rcc_is_clk_on(RCC_CLK_PLL));

    cfgr = RCC_BASE->CFGR;
    cfgr &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLMUL);
    cfgr |= pll_cfg->pllsrc | pll_mul;

    RCC_BASE->CFGR = cfgr;
}

void rcc_clk_enable(rcc_clk_id id)
{
    static __io uint32* enable_regs[] = {
        [APB1] = &RCC_BASE->APB1ENR,
        [APB2] = &RCC_BASE->APB2ENR,
        [AHB] = &RCC_BASE->AHBENR,
    };
    rcc_do_clk_enable(enable_regs, id);
}

void rcc_reset_dev(rcc_clk_id id)
{
    static __io uint32* reset_regs[] = {
        [APB1] = &RCC_BASE->APB1RSTR,
        [APB2] = &RCC_BASE->APB2RSTR,
    };
    rcc_do_reset_dev(reset_regs, id);
}

void rcc_set_prescaler(rcc_prescaler prescaler, uint32 divider)
{
    static const uint32 masks[] = {
        [RCC_PRESCALER_AHB] = RCC_CFGR_HPRE,
        [RCC_PRESCALER_APB1] = RCC_CFGR_PPRE1,
        [RCC_PRESCALER_APB2] = RCC_CFGR_PPRE2,
        [RCC_PRESCALER_USB] = RCC_CFGR_USBPRE,
        [RCC_PRESCALER_ADC] = RCC_CFGR_ADCPRE,
    };
    rcc_do_set_prescaler(masks, prescaler, divider);
}

void rcc_clk_disable(rcc_clk_id id)
{
    static __io uint32* enable_regs[] = {
        [APB1] = &RCC_BASE->APB1ENR,
        [APB2] = &RCC_BASE->APB2ENR,
        [AHB] = &RCC_BASE->AHBENR,
    };
    rcc_do_clk_disable(enable_regs, id);
}