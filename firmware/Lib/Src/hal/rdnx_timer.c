/**
 * @file rdnx_timer.c
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <string.h>

/* STM32 HAL include */
#include "stm32f4xx_hal.h"

/* Core includes. */
#include <rdnx_types.h>
#include <rdnx_macro.h>
#include "hal/rdnx_timer.h"

/* ============================================================================
 * Private Macro Definitions
 * ==========================================================================*/

 /** @brief Debug and Error log macros for the SPI driver. */
#ifdef DEBUG
    #define TIMER_DEB(fmt, ...) printf("[TMR](%s:%d):" fmt "\n\r", __func__, __LINE__, ##__VA_ARGS__)
    #define TIMER_ERR(fmt, ...) printf("[TMR](ERROR):" fmt "\n\r", ##__VA_ARGS__)
#else
    #define TIMER_DEB(fmt, ...)   ((void)0)
    #define TIMER_ERR(fmt, ...)   ((void)0)
#endif

/* ============================================================================
 * Private Type Definitions
 * ==========================================================================*/

typedef struct rdnx_timer_struct
{
    TIM_HandleTypeDef htim;
    rdnx_timer_params_t params;
    uint32_t timer_clk;
} rdnx_timer_struct_t;

/* ============================================================================
 * Static Storage
 * ==========================================================================*/

static rdnx_timer_struct_t g_timers[RDNX_TIMER_COUNT];

/* ============================================================================
 * Private Function Declaration
 * ==========================================================================*/

static TIM_TypeDef *rdnx_timer_get_instance(rdnx_timer_t t);
static IRQn_Type rdnx_timer_get_irqn(rdnx_timer_t t);
static void rdnx_timer_enable_clk(rdnx_timer_t t);
static uint32_t rdnx_timer_get_clk_freq(TIM_TypeDef *inst);

/* ============================================================================
 * Helpers
 * ==========================================================================*/

static TIM_TypeDef *rdnx_timer_get_instance(rdnx_timer_t t)
{
    static TIM_TypeDef *map[RDNX_TIMER_COUNT] = {
        TIM1,
        TIM2,
        TIM3,
        TIM4,
        TIM5,
        TIM6,
        TIM7,
        TIM8,
        TIM9,
        TIM10,
        TIM11,
        TIM12,
        TIM13,
        TIM14
    };
    return map[t];
}

static IRQn_Type rdnx_timer_get_irqn(rdnx_timer_t t)
{
    static IRQn_Type map[RDNX_TIMER_COUNT] = {
        TIM1_UP_TIM10_IRQn,
        TIM2_IRQn,
        TIM3_IRQn,
        TIM4_IRQn,
        TIM5_IRQn,
        TIM6_DAC_IRQn,
        TIM7_IRQn,
        TIM8_UP_TIM13_IRQn,
        TIM1_BRK_TIM9_IRQn,
        TIM1_UP_TIM10_IRQn,
        TIM1_TRG_COM_TIM11_IRQn,
        TIM8_BRK_TIM12_IRQn,
        TIM8_UP_TIM13_IRQn,
        TIM8_TRG_COM_TIM14_IRQn
    };
    return map[t];
}

static void rdnx_timer_enable_clk(rdnx_timer_t t)
{
    switch (t)
    {
    case RDNX_TIMER_1: __HAL_RCC_TIM1_CLK_ENABLE(); break;
    case RDNX_TIMER_2: __HAL_RCC_TIM2_CLK_ENABLE(); break;
    case RDNX_TIMER_3: __HAL_RCC_TIM3_CLK_ENABLE(); break;
    case RDNX_TIMER_4: __HAL_RCC_TIM4_CLK_ENABLE(); break;
    case RDNX_TIMER_5: __HAL_RCC_TIM5_CLK_ENABLE(); break;
    case RDNX_TIMER_6: __HAL_RCC_TIM6_CLK_ENABLE(); break;
    case RDNX_TIMER_7: __HAL_RCC_TIM7_CLK_ENABLE(); break;
    case RDNX_TIMER_8: __HAL_RCC_TIM8_CLK_ENABLE(); break;
    case RDNX_TIMER_9: __HAL_RCC_TIM9_CLK_ENABLE(); break;
    case RDNX_TIMER_10: __HAL_RCC_TIM10_CLK_ENABLE(); break;
    case RDNX_TIMER_11: __HAL_RCC_TIM11_CLK_ENABLE(); break;
    case RDNX_TIMER_12: __HAL_RCC_TIM12_CLK_ENABLE(); break;
    case RDNX_TIMER_13: __HAL_RCC_TIM13_CLK_ENABLE(); break;
    case RDNX_TIMER_14: __HAL_RCC_TIM14_CLK_ENABLE(); break;
    default: break;
    }
}

static uint32_t rdnx_timer_get_clk_freq(TIM_TypeDef *inst)
{
    /* Simplified: assumes APB1/APB2 timers doubling rule */
    if (inst == TIM1 || inst == TIM8 || inst == TIM9 ||
        inst == TIM10 || inst == TIM11)
    {
        return HAL_RCC_GetPCLK2Freq() * 2;
    }
    else
    {
        return HAL_RCC_GetPCLK1Freq() * 2;
    }
}

/* ============================================================================
 * API Implementation
 * ==========================================================================*/

rdnx_err_t rdnx_timer_init(const rdnx_timer_params_t *p, rdnx_timer_handle_t *out)
{
    if (!p || !out)
    {
        return RDNX_ERR_INVAL;
    }

    rdnx_timer_struct_t *t = &g_timers[p->timer];
    memset(t, 0, sizeof(*t));

    t->params = *p;

    rdnx_timer_enable_clk(p->timer);

    t->htim.Instance = rdnx_timer_get_instance(p->timer);

    t->timer_clk = rdnx_timer_get_clk_freq(t->htim.Instance);

    t->htim.Init.Prescaler = p->divider - 1;
    t->htim.Init.CounterMode = TIM_COUNTERMODE_UP;
    t->htim.Init.Period = p->period - 1;
    t->htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    t->htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(&t->htim) != HAL_OK)
    {
        return RDNX_ERR_FAIL;
    }

    HAL_NVIC_SetPriority(rdnx_timer_get_irqn(p->timer), 5, 0);
    HAL_NVIC_EnableIRQ(rdnx_timer_get_irqn(p->timer));

    *out = (rdnx_timer_handle_t)t;

    return RDNX_ERR_OK;
}

rdnx_err_t rdnx_timer_enable(rdnx_timer_handle_t h)
{
    if (!h)
    {
        return RDNX_ERR_INVAL;
    }
    return (HAL_TIM_Base_Start_IT(&((rdnx_timer_struct_t *)h)->htim) == HAL_OK) ? RDNX_ERR_OK : RDNX_ERR_FAIL;
}

rdnx_err_t rdnx_timer_disable(rdnx_timer_handle_t h)
{
    if (!h)
    {
        return RDNX_ERR_INVAL;
    }
    return (HAL_TIM_Base_Stop_IT(&((rdnx_timer_struct_t *)h)->htim) == HAL_OK) ? RDNX_ERR_OK : RDNX_ERR_FAIL;
}

rdnx_err_t rdnx_timer_set_period(rdnx_timer_handle_t h, uint32_t period)
{
    if (!h)
    {
        return RDNX_ERR_INVAL;
    }

    rdnx_timer_struct_t *t = h;

    __HAL_TIM_SET_AUTORELOAD(&t->htim, period - 1);
    t->params.period = period;

    return RDNX_ERR_OK;
}

rdnx_err_t rdnx_timer_get_frequency(rdnx_timer_handle_t h, uint32_t *freq)
{
    if (!h || !freq)
    {
        return RDNX_ERR_INVAL;
    }

    rdnx_timer_struct_t *t = h;

    *freq = t->timer_clk / (t->params.divider * t->params.period);

    return RDNX_ERR_OK;
}

rdnx_err_t rdnx_timer_set_counter(rdnx_timer_handle_t h, uint32_t cnt)
{
    if (!h)
    {
        return RDNX_ERR_INVAL;
    }
    __HAL_TIM_SET_COUNTER(&((rdnx_timer_struct_t *)h)->htim, cnt);
    return RDNX_ERR_OK;
}

rdnx_err_t rdnx_timer_get_counter(rdnx_timer_handle_t h, uint32_t *cnt)
{
    if (!h || !cnt)
    {
        return RDNX_ERR_INVAL;
    }
    *cnt = __HAL_TIM_GET_COUNTER(&((rdnx_timer_struct_t *)h)->htim);
    return RDNX_ERR_OK;
}

rdnx_err_t rdnx_timer_register_callback(rdnx_timer_handle_t h, rdnx_timerdev_cb cb)
{
    if (!h)
    {
        return RDNX_ERR_INVAL;
    }
    ((rdnx_timer_struct_t *)h)->params.cb_isr = cb;
    return RDNX_ERR_OK;
}

rdnx_err_t rdnx_timer_unregister_callback(rdnx_timer_handle_t h)
{
    if (!h)
    {
        return RDNX_ERR_INVAL;
    }
    ((rdnx_timer_struct_t *)h)->params.cb_isr = NULL;
    return RDNX_ERR_OK;
}

/* ============================================================================
 * IRQ + Callback Bridge
 * ==========================================================================*/

RDNX_WEAK void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    for (int i = 0; i < RDNX_TIMER_COUNT; i++)
    {
        rdnx_timer_struct_t *t = &g_timers[i];

        if (t->htim.Instance == htim->Instance)
        {
            if (t->params.cb_isr)
                t->params.cb_isr(t->params.timer);
        }
    }
}

/* ============================================================================
 * IRQ Handlers (weak override expected in startup)
 * ==========================================================================*/

#define HANDLE_IRQ(timer) \
void timer##_IRQHandler(void) \
{ \
    HAL_TIM_IRQHandler(&g_timers[RDNX_TIMER_##timer##_INDEX].htim); \
}

/* You may prefer manual mapping instead of macro for clarity */
void TIM2_IRQHandler(void) { HAL_TIM_IRQHandler(&g_timers[RDNX_TIMER_2].htim); }
void TIM3_IRQHandler(void) { HAL_TIM_IRQHandler(&g_timers[RDNX_TIMER_3].htim); }
/* Add others as needed */
