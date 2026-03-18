/**
 * @file    rdnx_gpio.h
 * @author  Software development team
 * @brief   API for GPIO peripheral devices
 * @version 1.0
 * @date    2024-10-18
 */

#ifndef RDNX_GPIO_H
#define RDNX_GPIO_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stddef.h> // for size_t
#include <stdint.h>

/* Core includes. */
#include <rdnx_types.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================================================================
 * Public Type Declarations
 * ==========================================================================*/

/**
 * @brief GPIO port identifier
 *
 * Not all ports are valid on all devices
 */
typedef enum rdnx_gpio_port
{
    RDNX_GPIO_PORT_NONE = 0,
    RDNX_GPIO_PORT_A,
    RDNX_GPIO_PORT_B,
    RDNX_GPIO_PORT_C,
    RDNX_GPIO_PORT_D,
    RDNX_GPIO_PORT_E,
    RDNX_GPIO_PORT_F,
    RDNX_GPIO_PORT_G,
    RDNX_GPIO_PORT_H
} rdnx_gpio_port_t;

/**
 * @brief GPIO pin identifier, relative to a single port
 *
 * The range of valid pin numbers will be different for different hardware
 * modules and even for different GPIO ports on a single module.
 */
typedef uint32_t rdnx_gpio_pin_t;

/**
 * @brief GPIO digital signal levels
 */
typedef enum rdnx_gpio_level
{
    RDNX_GPIO_LEVEL_LOW,
    RDNX_GPIO_LEVEL_HIGH
} rdnx_gpio_level_t;

/**
 * @brief GPIO direction
 *
 * - INPUT pins are for reading (digital or analog) levels
 * - OUTPUT pins are for setting (digital or analog) levels
 * - INTERRUPT pins are input pins configured to generate an interrupt
 *   (calling a registered callback function) when the level transitions as
 *   configured.  See also #RDNX_GPIO_irq_active_level_t and
 * #RDNX_GPIO_irq_trigger_t
 */
typedef enum rdnx_gpio_direction
{
    RDNX_GPIO_DIRECTION_INPUT,
    RDNX_GPIO_DIRECTION_OUTPUT,
    RDNX_GPIO_DIRECTION_INTERRUPT
} rdnx_gpio_direction_t;

/**
 * @brief GPIO signal pull configuration
 */
typedef enum rdnx_gpio_pull
{
    RDNX_GPIO_PULL_NONE,     /** @brief Neither pull up nor pull down resistors are
                                connected */
    RDNX_GPIO_PULL_DOWN,     /** @brief Platform default pull down resistor is
                                connected */
    RDNX_GPIO_PULL_UP,       /** @brief Platform default pull up resistor is
                                connected */
    RDNX_GPIO_PULL_SHUTDOWN, /** @brief Platform specific pin shutdown. It can
                                be used to conserve power. */
} rdnx_gpio_pull_t;

/**
 * @brief GPIO interrupt-triggering modes
 *
 * - LEVEL triggers whenever the signal level is at the active level (high or
 *   low)
 * - EDGE triggers only when the signal level transitions to the active level
 * - DOUBLE triggers on both the rising and falling edges.
 */
typedef enum rdnx_gpio_irq_trigger
{
    RDNX_GPIO_IRQ_TRIGGER_LEVEL_LOW,
    RDNX_GPIO_IRQ_TRIGGER_LEVEL_HIGH,
    RDNX_GPIO_IRQ_TRIGGER_EDGE_FALLING,
    RDNX_GPIO_IRQ_TRIGGER_EDGE_RISING,
    RDNX_GPIO_IRQ_TRIGGER_DOUBLE
} rdnx_gpio_irq_trigger_t;

/**
 * @brief GPIO callback handler function prototype
 *
 * When registered with a GPIO pin configured for interrupt mode, this function
 * is called (from interrupt context) when the interrupt triggers.
 *
 * @param[in] port Identifies the GPIO port that triggered the interrupt
 * @param[in] pin  Identifies the GPIO pin (within the port) that triggered
 *                 the interrupt
 * @param[in] user_ctx User context
 */
typedef void (*rdnx_gpio_callback)(rdnx_gpio_port_t port, rdnx_gpio_pin_t pin, void *user_ctx);

/**
 * @brief GPIO digital output device configuration parameters
 */
typedef struct rdnx_gpio_params
{
    /**
     * @brief The HAL GPIO handler for specific platform.
     *        See #GPIO_TypeDef.
     */
    rdnx_gpio_port_t port;

    /**
     * @brief GPIO pin
     *
     * Used in combination with #port to specify a physical GPIO pin.
     */
    rdnx_gpio_pin_t pin;

    /**
     * @brief Initial level
     *
     * When configuring an output pin, this is the initial level used by the
     * pin.
     */
    rdnx_gpio_level_t level;

    /**
     * @brief I/O direction
     *
     * The GPIO pin's operating mode.  Input, output or interrupt.
     */
    rdnx_gpio_direction_t direction;

    /**
     * @brief Internal pull resistor configuration.
     */
    rdnx_gpio_pull_t pull;

    /**
     * @brief Interrupt triggering mode
     *
     * Used to determine when a GPIO pin configured for interrupt mode should
     * generate an interrupt.
     *
     * #RDNX_GPIO_IRQ_TRIGGER_LEVEL. Level-triggered interrupt. The interrupt
     *                               triggers whenever the signal level is at
     *                               the activation level (high or low).
     * #RDNX_GPIO_IRQ_TRIGGER_EDGE.  Edge-triggered interrupt.  The interrupt
     *                               triggers whenever the signal level
     *                               transitions to the activation level.
     * #RDNX_GPIO_IRQ_TRIGGER_DOUBLE. Double-edge-triggered interrupt. The
     *                               interrupt triggers both on rising and
     *                               falling edges.
     */
    rdnx_gpio_irq_trigger_t irq_trigger;

    /**
     * @brief GPIO callback function
     *
     * When registered with a GPIO pin configured for interrupt mode, this
     * function is called (from interrupt context) when the interrupt triggers.
     */
    rdnx_gpio_callback cb;

    /**
     * @brief User context for GPIO callback
     */
    void *user_ctx;
} rdnx_gpio_params_t;

/**
 * @brief Handle to an initialized GPIO device
 */
typedef struct rdnx_gpio_ctx *rdnx_gpio_handle_t;

/* ============================================================================
 * Global Function Declarations (prototypes)
 * ==========================================================================*/

/**
 * @brief Create a GPIO device
 *
 * @param[in]  params GPIO configuration parameters for a single pin
 * @param[out] handle Pointer to memory which, on success, will contain a
 *                    handle to the configured device.
 *
 * @return #RDNX_ERR_OK on success
 */
rdnx_err_t rdnx_gpio_init(const rdnx_gpio_params_t *param, rdnx_gpio_handle_t *handle);

/**
 * @brief Destroy a GPIO device, freeing its resources.
 *
 * @param[in] handle Handle to the GPIO device
 *
 * @return #RDNX_ERR_OK on success
 */
rdnx_err_t rdnx_gpio_free(rdnx_gpio_handle_t handle);

/**
 * @brief Get a GPIO device's signal level
 *
 * @param[in]  handle Handle to the GPIO device
 * @param[out] val    Pointer to memory which, on success, will contain the
 *                    device's signal level.  `0` indicates a low level
 *                    (usually ground).  Non-zero (usually `1`) indicates a
 *                    high level (usually the module's *VDD* voltage).
 *
 * @return #RDNX_ERR_OK on success
 */
rdnx_err_t rdnx_gpio_get(rdnx_gpio_handle_t handle, uint8_t *val);

/**
 * @brief Set a GPIO output device's signal level
 *
 * @param[in] handle Handle to the GPIO device
 * @param[in] val    The desired signal level.  `0` indicates a low level
 *                   (usually ground).  Non-zero indicates a high level
 *                   (usually the module's *VDD* voltage).
 *
 * @return #RDNX_ERR_OK on success
 */
rdnx_err_t rdnx_gpio_set(rdnx_gpio_handle_t handle, uint8_t val);

/**
 * @brief Destroy GPIO handle and release associated resources
 *
 * Deinitializes the GPIO instance associated with the given handle and
 * releases any internally allocated resources. After this call, the handle
 * becomes invalid and must not be used again.
 *
 * @param handle GPIO handle to destroy
 *
 * @return rdnx_err_t
 * @retval RDNX_OK                GPIO successfully destroyed
 * @retval RDNX_ERR_INVALID_ARG   Handle is NULL or invalid
 */
rdnx_err_t rdnx_gpio_destroy(rdnx_gpio_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif // RDNX_GPIO_H
