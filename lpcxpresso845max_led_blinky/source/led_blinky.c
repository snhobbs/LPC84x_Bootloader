/*
 * Copyright 2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "board.h"

#include "pin_mux.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define BOARD_LED_PORT 0/*BOARD_LED_RED_PORT*/
#define BOARD_LED_PIN  22 /*BOARD_LED_RED_PIN*/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
volatile uint32_t g_systickCounter;

/*******************************************************************************
 * Code
 ******************************************************************************/
void SysTick_Handler(void)
{
    if (g_systickCounter != 0U)
    {
        g_systickCounter--;
    }
}

void SysTick_DelayTicks(uint32_t n)
{
    g_systickCounter = n;
    while (g_systickCounter != 0U)
    {
    }
}

__attribute__((__section__(".m_validate"))) volatile const uint32_t image_signature __attribute__((aligned(4))) = 0xeaeaeaea;
/*!
 * @brief Main function
 */
int main(void)
{
    /* Init output LED GPIO. */
    GPIO_PortInit(GPIO, BOARD_LED_PORT);
    /* Board pin init */
    BOARD_InitPins();
    BOARD_InitBootClocks();
    //assert(image_signature == 0xeaeaeaea);

    /* Set systick reload value to generate 1ms interrupt */
    if (SysTick_Config(SystemCoreClock / 1000U))
    {
        while (1)
        {
          SysTick_DelayTicks(image_signature >> 30);
        }
    }

    while (1)
    {
        /* Delay 1000 ms */
        SysTick_DelayTicks(1000U);
        GPIO_PortToggle(GPIO, BOARD_LED_PORT, 1u << BOARD_LED_PIN);
        GPIO_PortToggle(GPIO, 0, 1u << 23);
    }
}
