/**
 * @file    yaa_gpio.h
 * @author  Software development team
 * @brief   API for GPIO peripheral devices
 * @version 1.0
 * @date    2024-10-18
 */

#ifndef YAA_GPIO_H
#define YAA_GPIO_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stddef.h> // for size_t
#include <stdint.h>

/* Core includes. */
#include <yaa_types.h>

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
typedef enum yaa_gpio_port
{
    YAA_GPIO_PORT_NONE = 0,
    YAA_GPIO_PORT_A,
    YAA_GPIO_PORT_B,
    YAA_GPIO_PORT_C,
    YAA_GPIO_PORT_D,
    YAA_GPIO_PORT_E,
    YAA_GPIO_PORT_F,
    YAA_GPIO_PORT_G,
    YAA_GPIO_PORT_H
} yaa_gpio_port_t;

/**
 * @brief GPIO pin identifier, relative to a single port
 *
 * The range of valid pin numbers will be different for different hardware
 * modules and even for different GPIO ports on a single module.
 */
typedef uint32_t yaa_gpio_pin_t;

/**
 * @brief GPIO digital signal levels
 */
typedef enum yaa_gpio_level
{
    YAA_GPIO_LEVEL_LOW,
    YAA_GPIO_LEVEL_HIGH
} yaa_gpio_level_t;

/**
 * @brief GPIO direction
 *
 * - INPUT pins are for reading (digital or analog) levels
 * - OUTPUT pins are for setting (digital or analog) levels
 * - INTERRUPT pins are input pins configured to generate an interrupt
 *   (calling a registered callback function) when the level transitions as
 *   configured.  See also #YAA_GPIO_irq_active_level_t and
 * #YAA_GPIO_irq_trigger_t
 */
typedef enum yaa_gpio_direction
{
    YAA_GPIO_DIRECTION_INPUT,
    YAA_GPIO_DIRECTION_OUTPUT,
    YAA_GPIO_DIRECTION_INTERRUPT
} yaa_gpio_direction_t;

/**
 * @brief GPIO signal pull configuration
 */
typedef enum yaa_gpio_pull
{
    YAA_GPIO_PULL_NONE,     /** @brief Neither pull up nor pull down resistors are
                                connected */
    YAA_GPIO_PULL_DOWN,     /** @brief Platform default pull down resistor is
                                connected */
    YAA_GPIO_PULL_UP,       /** @brief Platform default pull up resistor is
                                connected */
    YAA_GPIO_PULL_SHUTDOWN, /** @brief Platform specific pin shutdown. It can
                                be used to conserve power. */
} yaa_gpio_pull_t;

/**
 * @brief GPIO interrupt-triggering modes
 *
 * - LEVEL triggers whenever the signal level is at the active level (high or
 *   low)
 * - EDGE triggers only when the signal level transitions to the active level
 * - DOUBLE triggers on both the rising and falling edges.
 */
typedef enum yaa_gpio_irq_trigger
{
    YAA_GPIO_IRQ_TRIGGER_LEVEL_LOW,
    YAA_GPIO_IRQ_TRIGGER_LEVEL_HIGH,
    YAA_GPIO_IRQ_TRIGGER_EDGE_FALLING,
    YAA_GPIO_IRQ_TRIGGER_EDGE_RISING,
    YAA_GPIO_IRQ_TRIGGER_DOUBLE
} yaa_gpio_irq_trigger_t;

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
typedef void (*yaa_gpio_callback)(yaa_gpio_port_t port, yaa_gpio_pin_t pin, void *user_ctx);

/**
 * @brief GPIO digital output device configuration parameters
 */
typedef struct yaa_gpio_params
{
    /**
     * @brief The HAL GPIO handler for specific platform.
     *        See #GPIO_TypeDef.
     */
    yaa_gpio_port_t port;

    /**
     * @brief GPIO pin
     *
     * Used in combination with #port to specify a physical GPIO pin.
     */
    yaa_gpio_pin_t pin;

    /**
     * @brief Initial level
     *
     * When configuring an output pin, this is the initial level used by the
     * pin.
     */
    yaa_gpio_level_t level;

    /**
     * @brief I/O direction
     *
     * The GPIO pin's operating mode.  Input, output or interrupt.
     */
    yaa_gpio_direction_t direction;

    /**
     * @brief Internal pull resistor configuration.
     */
    yaa_gpio_pull_t pull;

    /**
     * @brief Interrupt triggering mode
     *
     * Used to determine when a GPIO pin configured for interrupt mode should
     * generate an interrupt.
     *
     * #YAA_GPIO_IRQ_TRIGGER_LEVEL. Level-triggered interrupt. The interrupt
     *                               triggers whenever the signal level is at
     *                               the activation level (high or low).
     * #YAA_GPIO_IRQ_TRIGGER_EDGE.  Edge-triggered interrupt.  The interrupt
     *                               triggers whenever the signal level
     *                               transitions to the activation level.
     * #YAA_GPIO_IRQ_TRIGGER_DOUBLE. Double-edge-triggered interrupt. The
     *                               interrupt triggers both on rising and
     *                               falling edges.
     */
    yaa_gpio_irq_trigger_t irq_trigger;

    /**
     * @brief GPIO callback function
     *
     * When registered with a GPIO pin configured for interrupt mode, this
     * function is called (from interrupt context) when the interrupt triggers.
     */
    yaa_gpio_callback cb;

    /**
     * @brief User context for GPIO callback
     */
    void *user_ctx;
} yaa_gpio_params_t;

/**
 * @brief Handle to an initialized GPIO device
 */
typedef struct yaa_gpio_ctx *yaa_gpio_handle_t;

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
 * @return #YAA_ERR_OK on success
 */
yaa_err_t yaa_gpio_init(const yaa_gpio_params_t *param, yaa_gpio_handle_t *handle);

/**
 * @brief Destroy a GPIO device, freeing its resources.
 *
 * @param[in] handle Handle to the GPIO device
 *
 * @return #YAA_ERR_OK on success
 */
yaa_err_t yaa_gpio_free(yaa_gpio_handle_t handle);

/**
 * @brief Get a GPIO device's signal level
 *
 * @param[in]  handle Handle to the GPIO device
 * @param[out] val    Pointer to memory which, on success, will contain the
 *                    device's signal level.  `0` indicates a low level
 *                    (usually ground).  Non-zero (usually `1`) indicates a
 *                    high level (usually the module's *VDD* voltage).
 *
 * @return #YAA_ERR_OK on success
 */
yaa_err_t yaa_gpio_get(yaa_gpio_handle_t handle, uint8_t *val);

/**
 * @brief Set a GPIO output device's signal level
 *
 * @param[in] handle Handle to the GPIO device
 * @param[in] val    The desired signal level.  `0` indicates a low level
 *                   (usually ground).  Non-zero indicates a high level
 *                   (usually the module's *VDD* voltage).
 *
 * @return #YAA_ERR_OK on success
 */
yaa_err_t yaa_gpio_set(yaa_gpio_handle_t handle, uint8_t val);

/**
 * @brief Destroy GPIO handle and release associated resources
 *
 * Deinitializes the GPIO instance associated with the given handle and
 * releases any internally allocated resources. After this call, the handle
 * becomes invalid and must not be used again.
 *
 * @param handle GPIO handle to destroy
 *
 * @return yaa_err_t
 * @retval YAA_OK                GPIO successfully destroyed
 * @retval YAA_ERR_INVALID_ARG   Handle is NULL or invalid
 */
yaa_err_t yaa_gpio_destroy(yaa_gpio_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif // YAA_GPIO_H
