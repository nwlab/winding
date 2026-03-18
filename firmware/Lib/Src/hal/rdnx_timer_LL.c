/**
 * @file rdnx_timer_LL.c
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <string.h>

/* Adjust per MCU family */
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_system.h"
#include "stm32f4xx_ll_tim.h"

/* Core includes. */
#include "hal/rdnx_timer.h"
#include <rdnx_macro.h>
#include <rdnx_types.h>

/* ============================================================================
 * Private Macro Definitions
 * ==========================================================================*/

/** @brief Debug and Error log macros for the SPI driver. */
#ifdef DEBUG
#define TIMER_DEB(fmt, ...) printf("[TMR](%s:%d):" fmt "\n\r", __func__, __LINE__, ##__VA_ARGS__)
#define TIMER_ERR(fmt, ...) printf("[TMR](ERROR):" fmt "\n\r", ##__VA_ARGS__)
#else
#define TIMER_DEB(fmt, ...) ((void)0)
#define TIMER_ERR(fmt, ...) ((void)0)
#endif

/* ============================================================================
 * Private Type Definitions
 * ==========================================================================*/

typedef struct rdnx_timer_struct
{
    TIM_TypeDef *inst;
    rdnx_timer_params_t params;
    uint32_t timer_clk;
} rdnx_timer_t_impl;

/* ============================================================================
 * Static Storage
 * ==========================================================================*/

static rdnx_timer_t_impl g_timers[RDNX_TIMER_COUNT];
static rdnx_timer_t_impl *g_timer_irq_map[RDNX_TIMER_COUNT];

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
    case RDNX_TIMER_1:
        LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM1);
        break;
    case RDNX_TIMER_2:
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2);
        break;
    case RDNX_TIMER_3:
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM3);
        break;
    case RDNX_TIMER_4:
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM4);
        break;
    case RDNX_TIMER_5:
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM5);
        break;
    case RDNX_TIMER_6:
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM6);
        break;
    case RDNX_TIMER_7:
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM7);
        break;
    case RDNX_TIMER_8:
        LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM8);
        break;
    case RDNX_TIMER_9:
        LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM9);
        break;
    case RDNX_TIMER_10:
        LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM10);
        break;
    case RDNX_TIMER_11:
        LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM11);
        break;
    case RDNX_TIMER_12:
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM12);
        break;
    case RDNX_TIMER_13:
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM13);
        break;
    case RDNX_TIMER_14:
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM14);
        break;
    default:
        break;
    }
}

extern uint32_t SystemCoreClock;

static uint32_t rdnx_timer_get_clk_freq(TIM_TypeDef *inst)
{
    uint32_t pclk;

    if (inst == TIM1 || inst == TIM8 || inst == TIM9 ||
        inst == TIM10 || inst == TIM11)
    {
        /* APB2 */
        uint32_t div = LL_RCC_GetAPB2Prescaler();
        if (div == LL_RCC_APB2_DIV_1) pclk = SystemCoreClock;
        else pclk = SystemCoreClock / (1 << ((div >> 4) - 1)) * 2; // doubling
    }
    else
    {
        /* APB1 */
        uint32_t div = LL_RCC_GetAPB1Prescaler();
        if (div == LL_RCC_APB1_DIV_1) pclk = SystemCoreClock;
        else pclk = SystemCoreClock / (1 << ((div >> 4) - 1)) * 2;
    }

    return pclk;
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

    rdnx_timer_t_impl *t = &g_timers[p->timer];
    memset(t, 0, sizeof(*t));

    t->inst = rdnx_timer_get_instance(p->timer);
    t->params = *p;

    rdnx_timer_enable_clk(p->timer);

    t->timer_clk = rdnx_timer_get_clk_freq(t->inst);

    LL_TIM_SetPrescaler(t->inst, p->divider - 1);
    LL_TIM_SetAutoReload(t->inst, p->period - 1);
    LL_TIM_SetCounterMode(t->inst, LL_TIM_COUNTERMODE_UP);

    LL_TIM_EnableARRPreload(t->inst);

    LL_TIM_ClearFlag_UPDATE(t->inst);
    LL_TIM_EnableIT_UPDATE(t->inst);

    IRQn_Type irqn = rdnx_timer_get_irqn(p->timer);

    NVIC_SetPriority(irqn, 5);
    NVIC_EnableIRQ(irqn);

    g_timer_irq_map[p->timer] = t;

    *out = (rdnx_timer_handle_t)t;

    return RDNX_ERR_OK;
}

rdnx_err_t rdnx_timer_enable(rdnx_timer_handle_t h)
{
    if (!h)
    {
        return RDNX_ERR_INVAL;
    }

    LL_TIM_EnableCounter(((rdnx_timer_t_impl *)h)->inst);
    return RDNX_ERR_OK;
}

rdnx_err_t rdnx_timer_disable(rdnx_timer_handle_t h)
{
    if (!h)
    {
        return RDNX_ERR_INVAL;
    }

    LL_TIM_DisableCounter(((rdnx_timer_t_impl *)h)->inst);
    return RDNX_ERR_OK;
}

rdnx_err_t rdnx_timer_set_period(rdnx_timer_handle_t h, uint32_t period)
{
    if (!h)
    {
        return RDNX_ERR_INVAL;
    }

    rdnx_timer_t_impl *t = h;

    LL_TIM_SetAutoReload(t->inst, period - 1);
    t->params.period = period;

    return RDNX_ERR_OK;
}

rdnx_err_t rdnx_timer_get_frequency(rdnx_timer_handle_t h, uint32_t *freq)
{
    if (!h || !freq)
    {
        return RDNX_ERR_INVAL;
    }

    rdnx_timer_t_impl *t = h;

    *freq = t->timer_clk / (t->params.divider * t->params.period);

    return RDNX_ERR_OK;
}

rdnx_err_t rdnx_timer_set_counter(rdnx_timer_handle_t h, uint32_t cnt)
{
    if (!h)
    {
        return RDNX_ERR_INVAL;
    }

    LL_TIM_SetCounter(((rdnx_timer_t_impl *)h)->inst, cnt);
    return RDNX_ERR_OK;
}

rdnx_err_t rdnx_timer_get_counter(rdnx_timer_handle_t h, uint32_t *cnt)
{
    if (!h || !cnt)
    {
        return RDNX_ERR_INVAL;
    }

    *cnt = LL_TIM_GetCounter(((rdnx_timer_t_impl *)h)->inst);
    return RDNX_ERR_OK;
}

rdnx_err_t rdnx_timer_register_callback(rdnx_timer_handle_t h, rdnx_timerdev_cb cb)
{
    if (!h)
    {
        return RDNX_ERR_INVAL;
    }

    ((rdnx_timer_t_impl *)h)->params.cb_isr = cb;
    return RDNX_ERR_OK;
}

rdnx_err_t rdnx_timer_unregister_callback(rdnx_timer_handle_t h)
{
    if (!h)
    {
        return RDNX_ERR_INVAL;
    }

    ((rdnx_timer_t_impl *)h)->params.cb_isr = NULL;
    return RDNX_ERR_OK;
}

/* ============================================================================
 * IRQ Dispatch (FAST, no loops)
 * ==========================================================================*/

static inline void rdnx_timer_irq_handler(rdnx_timer_t t)
{
    rdnx_timer_t_impl *impl = g_timer_irq_map[t];

    if (!impl)
    {
        return;
    }

    if (LL_TIM_IsActiveFlag_UPDATE(impl->inst))
    {
        LL_TIM_ClearFlag_UPDATE(impl->inst);

        if (impl->params.cb_isr)
            impl->params.cb_isr(impl->params.timer);
    }
}

/* ============================================================================
 * IRQ Handlers
 * ==========================================================================*/

void TIM2_IRQHandler(void)
{
    rdnx_timer_irq_handler(RDNX_TIMER_2);
}
void TIM3_IRQHandler(void)
{
    rdnx_timer_irq_handler(RDNX_TIMER_3);
}
void TIM4_IRQHandler(void)
{
    rdnx_timer_irq_handler(RDNX_TIMER_4);
}
void TIM5_IRQHandler(void)
{
    rdnx_timer_irq_handler(RDNX_TIMER_5);
}
RDNX_WEAK void TIM6_DAC_IRQHandler(void)
{
    rdnx_timer_irq_handler(RDNX_TIMER_6);
}
void TIM7_IRQHandler(void)
{
    rdnx_timer_irq_handler(RDNX_TIMER_7);
}

/* Shared IRQ examples */
void TIM1_UP_TIM10_IRQHandler(void)
{
    rdnx_timer_irq_handler(RDNX_TIMER_1);
    rdnx_timer_irq_handler(RDNX_TIMER_10);
}

void TIM8_UP_TIM13_IRQHandler(void)
{
    rdnx_timer_irq_handler(RDNX_TIMER_8);
    rdnx_timer_irq_handler(RDNX_TIMER_13);
}
