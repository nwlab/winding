/**
 * @file    rdnx_gpio.c
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>  // for printf
#include <string.h> // for ffs

/* Library includes. */
#include <stm32f4xx_hal.h>

/* Core includes. */
#include <hal/rdnx_gpio.h>
#include <rdnx_macro.h>
#include <rdnx_sal.h>
#include <rdnx_slist.h>
#include <rdnx_types.h>

/* ============================================================================
 * Private Macro Definitions
 * ==========================================================================*/

#define GPIO(PORT, NUM)      (((PORT) - 'A') * 16 + (NUM))
#define GPIO2PORT(PIN)       ((PIN) / 16)
#define GPIO2PIN(PIN)        ((PIN) % 16)
#define GPIO2BIT(PIN)        (1 << ((PIN) % 16))
#define RDNX_GPIO(PORT, NUM) (((PORT) - RDNX_GPIO_PORT_A) * 16 + (NUM))

/* ============================================================================
 * Private Type Definitions
 * ==========================================================================*/

/**
 * @brief Struct containing the data for linking an application to a GPIO
 * instance.
 */
typedef struct rdnx_gpio_ctx
{
    /**
     * @brief Encoded GPIO identifier.
     *
     * The high 16 bits represent the GPIO port, and the low 16 bits
     * represent the GPIO pin number.
     */
    uint32_t pin;

    /**
     * @brief Callback function invoked on GPIO events.
     *
     * This function is called when a configured GPIO event occurs.
     */
    rdnx_gpio_callback cb;

    /**
     * @brief User-defined context pointer for the callback.
     *
     * This pointer is passed to the callback function and can be used
     * to store application-specific data associated with this GPIO.
     */
    void *user_ctx;
} rdnx_gpio_ctx_t;

/**
 * @brief List of the GPIO interrupts
 */
struct action_list_entry
{
    sys_snode_t next;
    rdnx_gpio_ctx_t *gpio_ctx;
};

/* ============================================================================
 * Public Variable Declarations (externs)
 * ==========================================================================*/

// clang-format off

GPIO_TypeDef *const digital_regs[] = {
    ['A' - 'A'] = GPIOA, GPIOB, GPIOC,
#ifdef GPIOD
    ['D' - 'A'] = GPIOD,
#endif
#ifdef GPIOE
    ['E' - 'A'] = GPIOE,
#endif
#ifdef GPIOF
    ['F' - 'A'] = GPIOF,
#endif
#ifdef GPIOG
    ['G' - 'A'] = GPIOG,
#endif
#ifdef GPIOH
    ['H' - 'A'] = GPIOH,
#endif
#ifdef GPIOI
    ['I' - 'A'] = GPIOI,
#endif
};

// clang-format on

/* ============================================================================
 * Private Variable Definitions
 * ==========================================================================*/

static sys_slist_t actions = RDNX_SLIST_STATIC_INIT();

/* ============================================================================
 * Private Function Declaration
 * ==========================================================================*/

static int rdnx_gpio_regs_to_pin(GPIO_TypeDef *regs, uint32_t bit) RDNX_UNUSED_FUNC;

static int rdnx_gpio_valid(uint32_t pin);

static IRQn_Type rdnx_gpio_exti_irq_id_from_pin(uint32_t pin);

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

rdnx_err_t rdnx_gpio_init(const rdnx_gpio_params_t *param, rdnx_gpio_handle_t *handle)
{
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };

    if (param == NULL || handle == NULL)
    {
        return RDNX_ERR_BADARG;
    }

    rdnx_gpio_ctx_t *ctx = (rdnx_gpio_ctx_t *)rdnx_alloc(sizeof(rdnx_gpio_ctx_t));
    if (ctx == NULL)
    {
        return RDNX_ERR_NOMEM;
    }

    ctx->pin = RDNX_GPIO(param->port, param->pin);
    if (!rdnx_gpio_valid(ctx->pin))
    {
        rdnx_free(ctx);
        return RDNX_ERR_BADARG;
    }

    // Enable gpio port in RCC
    switch (GPIO2PORT(ctx->pin))
    {
    case 0:
        __HAL_RCC_GPIOA_CLK_ENABLE();
        break;
#ifdef GPIOB
    case 1:
        __HAL_RCC_GPIOB_CLK_ENABLE();
        break;
#endif
#ifdef GPIOC
    case 2:
        __HAL_RCC_GPIOC_CLK_ENABLE();
        break;
#endif
#ifdef GPIOD
    case 3:
        __HAL_RCC_GPIOD_CLK_ENABLE();
        break;
#endif
#ifdef GPIOE
    case 4:
        __HAL_RCC_GPIOE_CLK_ENABLE();
        break;
#endif
#ifdef GPIOF
    case 5:
        __HAL_RCC_GPIOF_CLK_ENABLE();
        break;
#endif
#ifdef GPIOG
    case 6:
        __HAL_RCC_GPIOG_CLK_ENABLE();
        break;
#endif
#ifdef GPIOH
    case 7:
        __HAL_RCC_GPIOH_CLK_ENABLE();
        break;
#endif
#ifdef GPIOI
    case 8:
        __HAL_RCC_GPIOI_CLK_ENABLE();
        break;
#endif
#ifdef GPIOJ
    case 9:
        __HAL_RCC_GPIOJ_CLK_ENABLE();
        break;
#endif
#ifdef GPIOK
    case 10:
        __HAL_RCC_GPIOK_CLK_ENABLE();
        break;
#endif
    default:
        break;
    }

    GPIO_TypeDef *regs = digital_regs[GPIO2PORT(ctx->pin)];

    GPIO_InitStruct.Pin = GPIO2BIT(ctx->pin);

    switch (param->pull)
    {
    case RDNX_GPIO_PULL_NONE:
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        break;
    case RDNX_GPIO_PULL_DOWN:
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
        break;
    case RDNX_GPIO_PULL_UP:
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        break;
    default:
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        break;
    }

    if (param->direction == RDNX_GPIO_DIRECTION_INPUT)
    {
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    }
    else if (param->direction == RDNX_GPIO_DIRECTION_INTERRUPT)
    {
        struct action_list_entry *action;

        switch (param->irq_trigger)
        {
        case RDNX_GPIO_IRQ_TRIGGER_LEVEL_HIGH:
        case RDNX_GPIO_IRQ_TRIGGER_EDGE_RISING:
            GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
            break;
        case RDNX_GPIO_IRQ_TRIGGER_LEVEL_LOW:
        case RDNX_GPIO_IRQ_TRIGGER_EDGE_FALLING:
            GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
            break;
        case RDNX_GPIO_IRQ_TRIGGER_DOUBLE:
            GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
            break;
        default:
            break;
        }

        /* Check port number */
        if (!IS_EXTI_GPIO_PORT(GPIO2PORT(ctx->pin)))
        {
            rdnx_free(ctx);
            return RDNX_ERR_BADARG;
        }

        /* Check pin number */
        if (!IS_EXTI_GPIO_PIN(GPIO2PIN(ctx->pin)))
        {
            rdnx_free(ctx);
            return RDNX_ERR_BADARG;
        }

        /* EXTI interrupt init*/
        IRQn_Type irq_id = rdnx_gpio_exti_irq_id_from_pin(ctx->pin);
        HAL_NVIC_SetPriority(irq_id, 5, 0);
        HAL_NVIC_EnableIRQ(irq_id);

        // Add to the list
        action = rdnx_alloc(sizeof(struct action_list_entry));
        if (action != NULL)
        {
            action->gpio_ctx = ctx;
            rdnx_slist_append(&actions, &action->next);
        }
    }
    else if (param->direction == RDNX_GPIO_DIRECTION_OUTPUT)
    {
        switch (param->pull)
        {
        case RDNX_GPIO_PULL_UP:
        case RDNX_GPIO_PULL_DOWN:
            GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
            break;
        case RDNX_GPIO_PULL_NONE:
        default:
            GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
            break;
        }
    }

    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(regs, &GPIO_InitStruct);

    ctx->cb = param->cb;
    ctx->user_ctx = param->user_ctx;

    *handle = ctx;

    return RDNX_ERR_OK;
}

rdnx_err_t rdnx_gpio_free(rdnx_gpio_handle_t handle)
{
    rdnx_gpio_ctx_t *ctx = handle;

    if (ctx == NULL)
    {
        return RDNX_ERR_BADARG;
    }

    // TODO Find and remove action
    struct action_list_entry *action = NULL;
    if (action != NULL)
    {
        rdnx_slist_remove(&actions, NULL, &action->next);
    }

    rdnx_free(ctx);

    return RDNX_ERR_OK;
}

rdnx_err_t rdnx_gpio_get(rdnx_gpio_handle_t handle, uint8_t *val)
{
    rdnx_gpio_ctx_t *ctx = handle;

    if (ctx == NULL || val == NULL)
    {
        return RDNX_ERR_BADARG;
    }
    GPIO_TypeDef *regs = digital_regs[GPIO2PORT(ctx->pin)];

    *val = (uint8_t)HAL_GPIO_ReadPin(regs, GPIO2BIT(ctx->pin));

    return RDNX_ERR_OK;
}

rdnx_err_t rdnx_gpio_set(rdnx_gpio_handle_t handle, uint8_t val)
{
    rdnx_gpio_ctx_t *ctx = handle;

    if (ctx == NULL)
    {
        return RDNX_ERR_BADARG;
    }
    GPIO_TypeDef *regs = digital_regs[GPIO2PORT(ctx->pin)];

    HAL_GPIO_WritePin(regs, GPIO2BIT(ctx->pin), val == 0 ? GPIO_PIN_RESET : GPIO_PIN_SET);

    return RDNX_ERR_OK;
}

rdnx_err_t rdnx_gpio_destroy(rdnx_gpio_handle_t handle)
{
    rdnx_gpio_ctx_t *ctx = RDNX_CAST(rdnx_gpio_ctx_t *, handle);
    if (ctx == NULL)
    {
        return RDNX_ERR_NORESOURCE;
    }

    rdnx_free(ctx);

    return RDNX_ERR_OK;
}

/* ============================================================================
 * Private Function Definitions
 * ==========================================================================*/

// Convert a register and bit location back to an integer pin identifier
static int rdnx_gpio_regs_to_pin(GPIO_TypeDef *regs, uint32_t bit)
{
    uint8_t i;
    for (i = 0; i < ARRAY_SIZE(digital_regs); i++)
    {
        if (digital_regs[i] == regs)
        {
            return GPIO('A' + i, ffs(bit) - 1);
        }
    }
    return 0;
}

// Verify that a gpio is a valid pin
static int rdnx_gpio_valid(uint32_t pin)
{
    uint32_t port = GPIO2PORT(pin);
    return port < ARRAY_SIZE(digital_regs) && digital_regs[port];
}

static IRQn_Type rdnx_gpio_exti_irq_id_from_pin(uint32_t pin)
{
    switch (GPIO2PIN(pin))
    {
    case 0:
        return EXTI0_IRQn;
    case 1:
        return EXTI1_IRQn;
    case 2:
        return EXTI2_IRQn;
    case 3:
        return EXTI3_IRQn;
    case 4:
        return EXTI4_IRQn;
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
        return EXTI9_5_IRQn;
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
        return EXTI15_10_IRQn;
    default:
        return EXTI0_IRQn;
    }
}

/**
 * @brief Generic Interrupt handler callback
 * @param pin pin number on which the interrupt occurred (GPIO_PIN_pin)
 */
static inline void rdnx_gpio_handle_callback(uint16_t pin)
{
    struct action_list_entry *it;
    // Check exits in the list
    RDNX_SLIST_FOR_EACH_CONTAINER(&actions, struct action_list_entry, it, next)
    {
        if (it->gpio_ctx != NULL && GPIO2BIT(it->gpio_ctx->pin) == pin)
        {
            if (it->gpio_ctx->cb)
            {
                it->gpio_ctx->cb(GPIO2PORT(it->gpio_ctx->pin), GPIO2PIN(it->gpio_ctx->pin), it->gpio_ctx->user_ctx);
            }
        }
    }
}

/**
 * @brief EXTI GPIO Interrupt handler callback
 * @param pin pin number on which the interrupt occurred (GPIO_PIN_pin)
 */
void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    rdnx_gpio_handle_callback(pin);
}

/**
 * @brief EXTI GPIO Interrupt handler callback for rising edge detection
 * @param pin pin number on which the interrupt occurred (GPIO_PIN_pin)
 */
void HAL_GPIO_EXTI_Rising_Callback(uint16_t pin)
{
    rdnx_gpio_handle_callback(pin);
}

/**
 * @brief EXTI GPIO Interrupt handler callback for falling edge detection
 * @param pin pin number on which the interrupt occurred (GPIO_PIN_pin)
 */
void HAL_GPIO_EXTI_Falling_Callback(uint16_t pin)
{
    rdnx_gpio_handle_callback(pin);
}

void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

void EXTI1_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
}

void EXTI2_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);
}

void EXTI3_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);
}

void EXTI4_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
}

void EXTI9_5_IRQHandler(void)
{
    for (uint16_t pin = GPIO_PIN_5;
         pin <= GPIO_PIN_9;
         pin <<= 1)
    {
        HAL_GPIO_EXTI_IRQHandler(pin);
    }
}

void EXTI15_10_IRQHandler(void)
{
    for (uint32_t pin = GPIO_PIN_10;
         pin <= GPIO_PIN_15;
         pin <<= 1)
    {
        if (__HAL_GPIO_EXTI_GET_IT(pin) != RESET)
        {
            HAL_GPIO_EXTI_IRQHandler(pin);
        }
    }
}
