/**
 * @file    ltc2946.h
 * @author  Software development team
 * @brief   ADC driver for LTC2946 device
 * @version 1.0
 * @date    2024-10-21
 */

// clang-format off
/**
 * The LTC®2946 is a rail-to-rail system monitor that measures
 * current, voltage, power, charge and energy. It features an
 * operating range of 2.7V to 100V and includes a shunt regulator
 * for supplies above 100V. The current measurement common mode
 * range of 0V to 100V is independent of the input supply.
 * A 12-bit ADC measures load current, input voltage and an
 * auxiliary external voltage. Load current and internally
 * calculated power are integrated over an external clock or
 * crystal or internal oscillator time base for charge and energy.
 * An accurate time base allows the LTC2946 to provide measurement
 * accuracy of better than ±0.6% for charge and ±1% for power and
 * energy. Minimum and maximum values are stored and an overrange
 * alert with programmable thresholds minimizes the need for software
 * polling. Data is reported via a standard I2C interface.
 * Shutdown mode reduces power consumption to 15uA.
 *
 * I2C DATA FORMAT (MSB FIRST):
 *
 * Data Out:
 * Byte #1                                    Byte #2                     Byte #3
 *
 * START  SA6 SA5 SA4 SA3 SA2 SA1 SA0 W SACK  X  X C5 C4 C3 C2 C1 C0 SACK D7 D6 D5 D4 D3 D2 D1 D0 SACK  STOP
 *
 * Data In:
 * Byte #1                                    Byte #2                                    Byte #3
 *
 * START  SA6 SA5 SA4 SA3 SA2 SA1 SA0 W SACK  X  X  C5 C4 C3 C2 C1 C0 SACK  Repeat Start SA6 SA5 SA4 SA3 SA2 SA1 SA0 R SACK
 *
 * Byte #4                                   Byte #5
 * MSB                                       LSB
 * D15 D14  D13  D12  D11  D10  D9 D8 MACK   D7 D6 D5 D4 D3  D2  D1  D0  MNACK  STOP
 *
 * START       : I2C Start
 * Repeat Start: I2c Repeat Start
 * STOP        : I2C Stop
 * SAx         : I2C Address
 * SACK        : I2C Slave Generated Acknowledge (Active Low)
 * MACK        : I2C Master Generated Acknowledge (Active Low)
 * MNACK       : I2c Master Generated Not Acknowledge
 * W           : I2C Write (0)
 * R           : I2C Read  (1)
 * Cx          : Command Code
 * Dx          : Data Bits
 * X           : Don't care
 *
 *
 * Example Code:
 *
 * Read power, current, voltage, charge and energy.
 *
 *     // Set Control A register to default value in continuous mode
 *     CTRLA = LTC2946_CHANNEL_CONFIG_V_C_3|LTC2946_SENSE_PLUS|LTC2946_OFFSET_CAL_EVERY|LTC2946_ADIN_GND;
 *     // Sets the LTC2946 to continuous mode
 *     LTC2946_write(LTC2946_I2C_ADDRESS, LTC2946_CTRLA_REG, CTRLA);
 *
 *     resistor = .02; // Resistor Value On Demo Board
 *
 *     // Reads the ADC registers that contains V^2
 *     LTC2946_read_24_bits(LTC2946_I2C_ADDRESS, LTC2946_POWER_MSB2_REG, &power_code);
 *     // Calculates power from power code, resistor value and power lsb
 *     power = LTC2946_code_to_power(power_code, resistor, LTC2946_Power_lsb);
 *
 *     // Reads the voltage code across sense resistor
 *     LTC2946_read_12_bits(LTC2946_I2C_ADDRESS, LTC2946_DELTA_SENSE_MSB_REG, &current_code);
 *     // Calculates current from current code, resistor value and current lsb
 *     current = LTC2946_code_to_current(current_code, resistor, LTC2946_DELTA_SENSE_lsb);
 *
 *     // Reads VIN voltage code
 *     LTC2946_read_12_bits(LTC2946_I2C_ADDRESS, LTC2946_VIN_MSB_REG, &VIN_code);
 *     // Calculates VIN voltage from VIN code and lsb
 *     VIN = LTC2946_VIN_code_to_voltage(VIN_code, LTC2946_VIN_lsb);
 *
 *     // Reads energy code
 *     LTC2946_read_32_bits(LTC2946_I2C_ADDRESS, LTC2946_ENERGY_MSB3_REG, &energy_code);
 *     // Calculates Energy in Joules from energy_code, resistor, power lsb and time lsb
 *     energy = LTC2946_code_to_energy(energy_code,resistor,LTC2946_Power_lsb, LTC2946_INTERNAL_TIME_lsb);
 *
 *     // Reads charge code
 *     LTC2946_read_32_bits(LTC2946_I2C_ADDRESS, LTC2946_CHARGE_MSB3_REG, &charge_code);
 *     // Calculates charge in coulombs from charge_code, resistor, current lsb and time lsb
 *     charge = LTC2946_code_to_coulombs(charge_code,resistor,LTC2946_DELTA_SENSE_lsb, LTC2946_INTERNAL_TIME_lsb);
 *
 */
// clang-format on

#ifndef LTC2976_H
#define LTC2976_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdbool.h> // for bool
#include <stddef.h>  // for size_t
#include <stdint.h>

/* Core includes. */
#include <hal/yaa_i2c.h>
#include <yaa_types.h>
#include <yaa_macro.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================================================================
 * Public Macro Definitions
 * ==========================================================================*/

/** Use table to select address
| LTC2946 I2C Address Assignment    | Value |   AD1    |   AD2    |
|-----------------------------------|-------|----------|----------|
| LTC2946_I2C_ADDRESS               | 0x67  |   High   |   Low    |
| LTC2946_I2C_ADDRESS               | 0x68  |   Float  |   High   |
| LTC2946_I2C_ADDRESS               | 0x69  |   High   |   High   |
| LTC2946_I2C_ADDRESS               | 0x6A  | 	Float  |   Float  |
| LTC2946_I2C_ADDRESS               | 0x6B  | 	Float  |   Low    |
| LTC2946_I2C_ADDRESS               | 0x6C  | 	Low    |   High   |
| LTC2946_I2C_ADDRESS               | 0x6D  | 	High   |   Float  |
| LTC2946_I2C_ADDRESS               | 0x6E  | 	Low    |   Float  |
| LTC2946_I2C_ADDRESS               | 0x6F  | 	Low    |   Low    |
|                                   |       |          |          |
| LTC2946_I2C_MASS_WRITE            | 0xCC  |    X     |    X     |
| LTC2946_I2C_ALERT_RESPONSE        | 0x19  |    X     |    X     |
*/

//  Address assignment
// LTC2946 I2C Address              //  AD1       AD0
#define LTC2946_I2C_ADDRESS_0 0x67 //  High      Low
#define LTC2946_I2C_ADDRESS_1 0x68 //  Float     High
#define LTC2946_I2C_ADDRESS_2 0x69 //  High      High
#define LTC2946_I2C_ADDRESS_3 0x6A //  Float     Float
#define LTC2946_I2C_ADDRESS_4 0x6B //  Float     Low
#define LTC2946_I2C_ADDRESS_5 0x6C //  Low       High
#define LTC2946_I2C_ADDRESS_6 0x6D //  High      Float
#define LTC2946_I2C_ADDRESS_7 0x6E //  Low       Float
#define LTC2946_I2C_ADDRESS_8 0x6F //  Low       Low

/* Special 7-bit addresses */
#define LTC2946_I2C_MASS_WRITE_7BIT      0x66  /* 0xCC >> 1 */
#define LTC2946_I2C_ALERT_RESPONSE_7BIT  0x0C  /* 0x19 >> 1 */

// Registers

#define LTC2946_CTRLA_REG   0x00
#define LTC2946_CTRLB_REG   0x01
#define LTC2946_ALERT1_REG  0x02
#define LTC2946_STATUS1_REG 0x03
#define LTC2946_FAULT1_REG  0x04

#define LTC2946_POWER_MSB2_REG               0x05
#define LTC2946_POWER_MSB1_REG               0x06
#define LTC2946_POWER_LSB_REG                0x07
#define LTC2946_MAX_POWER_MSB2_REG           0x08
#define LTC2946_MAX_POWER_MSB1_REG           0x09
#define LTC2946_MAX_POWER_LSB_REG            0x0A
#define LTC2946_MIN_POWER_MSB2_REG           0x0B
#define LTC2946_MIN_POWER_MSB1_REG           0x0C
#define LTC2946_MIN_POWER_LSB_REG            0x0D
#define LTC2946_MAX_POWER_THRESHOLD_MSB2_REG 0x0E
#define LTC2946_MAX_POWER_THRESHOLD_MSB1_REG 0x0F
#define LTC2946_MAX_POWER_THRESHOLD_LSB_REG  0x10
#define LTC2946_MIN_POWER_THRESHOLD_MSB2_REG 0x11
#define LTC2946_MIN_POWER_THRESHOLD_MSB1_REG 0x12
#define LTC2946_MIN_POWER_THRESHOLD_LSB_REG  0x13

#define LTC2946_DELTA_SENSE_MSB_REG               0x14
#define LTC2946_DELTA_SENSE_LSB_REG               0x15
#define LTC2946_MAX_DELTA_SENSE_MSB_REG           0x16
#define LTC2946_MAX_DELTA_SENSE_LSB_REG           0x17
#define LTC2946_MIN_DELTA_SENSE_MSB_REG           0x18
#define LTC2946_MIN_DELTA_SENSE_LSB_REG           0x19
#define LTC2946_MAX_DELTA_SENSE_THRESHOLD_MSB_REG 0x1A
#define LTC2946_MAX_DELTA_SENSE_THRESHOLD_LSB_REG 0x1B
#define LTC2946_MIN_DELTA_SENSE_THRESHOLD_MSB_REG 0x1C
#define LTC2946_MIN_DELTA_SENSE_THRESHOLD_LSB_REG 0x1D

#define LTC2946_VIN_MSB_REG               0x1E
#define LTC2946_VIN_LSB_REG               0x1F
#define LTC2946_MAX_VIN_MSB_REG           0x20
#define LTC2946_MAX_VIN_LSB_REG           0x21
#define LTC2946_MIN_VIN_MSB_REG           0x22
#define LTC2946_MIN_VIN_LSB_REG           0x23
#define LTC2946_MAX_VIN_THRESHOLD_MSB_REG 0x24
#define LTC2946_MAX_VIN_THRESHOLD_LSB_REG 0x25
#define LTC2946_MIN_VIN_THRESHOLD_MSB_REG 0x26
#define LTC2946_MIN_VIN_THRESHOLD_LSB_REG 0x27

#define LTC2946_ADIN_MSB_REG               0x28
#define LTC2946_ADIN_LSB_REG_REG           0x29
#define LTC2946_MAX_ADIN_MSB_REG           0x2A
#define LTC2946_MAX_ADIN_LSB_REG           0x2B
#define LTC2946_MIN_ADIN_MSB_REG           0x2C
#define LTC2946_MIN_ADIN_LSB_REG           0x2D
#define LTC2946_MAX_ADIN_THRESHOLD_MSB_REG 0x2E
#define LTC2946_MAX_ADIN_THRESHOLD_LSB_REG 0x2F
#define LTC2946_MIN_ADIN_THRESHOLD_MSB_REG 0x30
#define LTC2946_MIN_ADIN_THRESHOLD_LSB_REG 0x31

#define LTC2946_ALERT2_REG   0x32
#define LTC2946_GPIO_CFG_REG 0x33

#define LTC2946_TIME_COUNTER_MSB3_REG 0x34
#define LTC2946_TIME_COUNTER_MSB2_REG 0x35
#define LTC2946_TIME_COUNTER_MSB1_REG 0x36
#define LTC2946_TIME_COUNTER_LSB_REG  0x37

#define LTC2946_CHARGE_MSB3_REG 0x38
#define LTC2946_CHARGE_MSB2_REG 0x39
#define LTC2946_CHARGE_MSB1_REG 0x3A
#define LTC2946_CHARGE_LSB_REG  0x3B

#define LTC2946_ENERGY_MSB3_REG 0x3C
#define LTC2946_ENERGY_MSB2_REG 0x3D
#define LTC2946_ENERGY_MSB1_REG 0x3E
#define LTC2946_ENERGY_LSB_REG  0x3F

#define LTC2946_STATUS2_REG    0x40
#define LTC2946_FAULT2_REG     0x41
#define LTC2946_GPIO3_CTRL_REG 0x42
#define LTC2946_CLK_DIV_REG    0x43

// Voltage Selection Command
#define LTC2946_DELTA_SENSE 0x00
#define LTC2946_VDD         0x08
#define LTC2946_ADIN        0x10
#define LTC2946_SENSE_PLUS  0x18

// Command Codes

#define LTC2946_ADIN_INTVCC 0x80
#define LTC2946_ADIN_GND    0x00

#define LTC2946_OFFSET_CAL_LAST  0x60
#define LTC2946_OFFSET_CAL_128   0x40
#define LTC2946_OFFSET_CAL_16    0x20
#define LTC2946_OFFSET_CAL_EVERY 0x00

#define LTC2946_CHANNEL_CONFIG_SNAPSHOT 0x07
#define LTC2946_CHANNEL_CONFIG_V_C      0x06
#define LTC2946_CHANNEL_CONFIG_A_V_C_1  0x05
#define LTC2946_CHANNEL_CONFIG_A_V_C_2  0x04
#define LTC2946_CHANNEL_CONFIG_A_V_C_3  0x03
#define LTC2946_CHANNEL_CONFIG_V_C_1    0x02
#define LTC2946_CHANNEL_CONFIG_V_C_2    0x01
#define LTC2946_CHANNEL_CONFIG_V_C_3    0x00

#define LTC2946_ENABLE_ALERT_CLEAR       0x80
#define LTC2946_ENABLE_SHUTDOWN          0x40
#define LTC2946_ENABLE_CLEARED_ON_READ   0x20
#define LTC2946_ENABLE_STUCK_BUS_RECOVER 0x10

#define LTC2946_DISABLE_ALERT_CLEAR       0x7F
#define LTC2946_DISABLE_SHUTDOWN          0xBF
#define LTC2946_DISABLE_CLEARED_ON_READ   0xDF
#define LTC2946_DISABLE_STUCK_BUS_RECOVER 0xEF

#define LTC2946_ACC_PIN_CONTROL 0x08
#define LTC2946_DISABLE_ACC     0x04
#define LTC2946_ENABLE_ACC      0x00

#define LTC2946_RESET_ALL          0x03
#define LTC2946_RESET_ACC          0x02
#define LTC2946_ENABLE_AUTO_RESET  0x01
#define LTC2946_DISABLE_AUTO_RESET 0x00

#define LTC2946_MAX_POWER_MSB2_RESET      0x00
#define LTC2946_MIN_POWER_MSB2_RESET      0xFF
#define LTC2946_MAX_DELTA_SENSE_MSB_RESET 0x00
#define LTC2946_MIN_DELTA_SENSE_MSB_RESET 0xFF
#define LTC2946_MAX_VIN_MSB_RESET         0x00
#define LTC2946_MIN_VIN_MSB_RESET         0xFF
#define LTC2946_MAX_ADIN_MSB_RESET        0x00
#define LTC2946_MIN_ADIN_MSB_RESET        0xFF

#define LTC2946_ENABLE_MAX_POWER_ALERT  0x80
#define LTC2946_ENABLE_MIN_POWER_ALERT  0x40
#define LTC2946_DISABLE_MAX_POWER_ALERT 0x7F
#define LTC2946_DISABLE_MIN_POWER_ALERT 0xBF

#define LTC2946_ENABLE_MAX_I_SENSE_ALERT  0x20
#define LTC2946_ENABLE_MIN_I_SENSE_ALERT  0x10
#define LTC2946_DISABLE_MAX_I_SENSE_ALERT 0xDF
#define LTC2946_DISABLE_MIN_I_SENSE_ALERT 0xEF

#define LTC2946_ENABLE_MAX_VIN_ALERT  0x08
#define LTC2946_ENABLE_MIN_VIN_ALERT  0x04
#define LTC2946_DISABLE_MAX_VIN_ALERT 0xF7
#define LTC2946_DISABLE_MIN_VIN_ALERT 0xFB

#define LTC2946_ENABLE_MAX_ADIN_ALERT  0x02
#define LTC2946_ENABLE_MIN_ADIN_ALERT  0x01
#define LTC2946_DISABLE_MAX_ADIN_ALERT 0xFD
#define LTC2946_DISABLE_MIN_ADIN_ALERT 0xFE

#define LTC2946_ENABLE_ADC_DONE_ALERT  0x80
#define LTC2946_DISABLE_ADC_DONE_ALERT 0x7F

#define LTC2946_ENABLE_GPIO_1_ALERT  0x40
#define LTC2946_DISABLE_GPIO_1_ALERT 0xBF

#define LTC2946_ENABLE_GPIO_2_ALERT  0x20
#define LTC2946_DISABLE_GPIO_2_ALERT 0xDF

#define LTC2946_ENABLE_STUCK_BUS_WAKE_ALERT  0x08
#define LTC2946_DISABLE_STUCK_BUS_WAKE_ALERT 0xF7

#define LTC2946_ENABLE_ENERGY_OVERFLOW_ALERT  0x04
#define LTC2946_DISABLE_ENERGY_OVERFLOW_ALERT 0xFB

#define LTC2946_ENABLE_CHARGE_OVERFLOW_ALERT  0x02
#define LTC2946_DISABLE_CHARGE_OVERFLOW_ALERT 0xFD

#define LTC2946_ENABLE_COUNTER_OVERFLOW_ALERT  0x01
#define LTC2946_DISABLE_COUNTER_OVERFLOW_ALERT 0xFE

#define LTC2946_GPIO1_IN_ACTIVE_HIGH 0xC0
#define LTC2946_GPIO1_IN_ACTIVE_LOW  0x80
#define LTC2946_GPIO1_OUT_HIGH_Z     0x40
#define LTC2946_GPIO1_OUT_LOW        0x00

#define LTC2946_GPIO2_IN_ACTIVE_HIGH 0x30
#define LTC2946_GPIO2_IN_ACTIVE_LOW  0x20
#define LTC2946_GPIO2_OUT_HIGH_Z     0x10
#define LTC2946_GPIO2_OUT_LOW        0x12
#define LTC2946_GPIO2_IN_ACC         0x00

#define LTC2946_GPIO3_IN_ACTIVE_HIGH 0x0C
#define LTC2946_GPIO3_IN_ACTIVE_LOW  0x08
#define LTC2946_GPIO3_OUT_REG_42     0x04
#define LTC2946_GPIO3_OUT_ALERT      0x00
#define LTC2946_GPIO3_OUT_LOW        0x40
#define LTC2946_GPIO3_OUT_HIGH_Z     0x00
#define LTC2946_GPIO_ALERT_CLEAR     0x00

// Register Mask Command
#define LTC2946_CTRLA_ADIN_MASK           0x7F
#define LTC2946_CTRLA_OFFSET_MASK         0x9F
#define LTC2946_CTRLA_VOLTAGE_SEL_MASK    0xE7
#define LTC2946_CTRLA_CHANNEL_CONFIG_MASK 0xF8
#define LTC2946_CTRLB_ACC_MASK            0xF3
#define LTC2946_CTRLB_RESET_MASK          0xFC
#define LTC2946_GPIOCFG_GPIO1_MASK        0x3F
#define LTC2946_GPIOCFG_GPIO2_MASK        0xCF
#define LTC2946_GPIOCFG_GPIO3_MASK        0xF3
#define LTC2946_GPIOCFG_GPIO2_OUT_MASK    0xFD
#define LTC2946_GPIO3_CTRL_GPIO3_MASK     0xBF

/* ============================================================================
 * Public Type Declarations
 * ==========================================================================*/

/**
 * @brief LTC2946 device configuration.
 */
typedef struct ltc2946_params
{
    /**
     * @brief I2C handle
     */
    yaa_i2c_handle_t i2c;

    /**
     * @brief I2C address of the device
     */
    uint8_t address;

} ltc2946_params_t;

/**
 * @brief Handle to an initialized LTC2946 device
 */
typedef struct ltc2946_ctx *ltc2946_handle_t;

/**
 * @brief
 */
typedef struct ltc2946_values
{
    uint32_t power_code, max_power_code, min_power_code;
    uint16_t current_code, max_current_code, min_current_code;
    uint16_t VIN_code, max_VIN_code, min_VIN_code;
    uint16_t ADIN_code, max_ADIN_code, min_ADIN_code;
    uint32_t energy_code;
    uint32_t charge_code;
    uint32_t time_code;
} ltc2946_values_t;

YAA_STATIC_ASSERT(sizeof(ltc2946_values_t) == 44);

/* ============================================================================
 * Public Variable Declarations
 * ==========================================================================*/

// LSB Weights
static const float LTC2946_ADIN_lsb = 5.001221E-04;                      //!< Typical ADIN lsb weight in volts (2.048f / 4096.0f)
static const float LTC2946_DELTA_SENSE_lsb = 2.5006105E-05;              //!< Typical Delta lsb weight in volts
static const float LTC2946_VIN_lsb = 2.5006105E-02;                      //!< Typical VIN lsb weight in volts (102.4f / 4096.0f)
static const float LTC2946_Power_lsb = 6.25305E-07;                      //!< Typical POWER lsb weight in W VIN_lsb * DELTA_SENSE_lsb
static const float LTC2946_TIME_lsb = 16.39543E-3;                       //!< Static variable which is based off of the default clk frequency of 250KHz.

/* ============================================================================
 * Global Function Declarations (prototypes)
 * ==========================================================================*/

/**
 * @brief Create an LTC2946 device
 *
 * @param[in]  params LTC2946 configuration parameters
 * @param[out] handle Pointer to memory which, on success, will contain a
 *                    handle to the configured device.
 *
 * @return #YAA_ERR_OK on success
 */
yaa_err_t ltc2946_init(const ltc2946_params_t *param, ltc2946_handle_t *handle);

/**
 * @brief Free an LTC2946 device
 *
 * After this call completes, the LTC2946 device will be invalid.
 *
 * Attempting to free an LTC2946 device while it is in use (by a read or write
 * operation) will result in undefined behavior.
 *
 * @param[in] handle Handle to the LTC2946 device
 *
 * @return #YAA_ERR_OK on success
 */
yaa_err_t ltc2946_destroy(ltc2946_handle_t handle);

/**
 * @brief Read data from an LTC2946 device
 *
 * This is a synchronous operation that will block until the operation
 * completes.
 *
 * @param[in]  handle         Handle to the LTC2946 device
 * @param[out] values         Pointer to memory which, upon successful
 *                            completion, will contain the received data.
 *
 * @return #YAA_ERR_OK on success
 */
yaa_err_t ltc2946_read(ltc2946_handle_t handle, ltc2946_values_t *values);

/**
 * @brief Print LTC2946 measurement values.
 *
 * This function prints the contents of an ::ltc2946_values_t structure
 * in a human-readable format. It is intended for debugging, logging,
 * or diagnostic purposes.
 *
 * The following measurement codes are printed:
 *  - Power (current, maximum, minimum)
 *  - Current (current, maximum, minimum)
 *  - VIN voltage (current, maximum, minimum)
 *  - ADIN voltage (current, maximum, minimum)
 *  - Accumulated energy
 *  - Accumulated charge
 *  - Accumulated time
 *
 * @param[in] v Pointer to the LTC2946 values structure to be printed.
 *              If NULL, the function prints an error message and returns.
 *
 * @note This function uses printf(). Ensure that a suitable output
 *       backend (UART, semihosting, RTT, etc.) is available on the target.
 *
 * @warning This function is intended for debug use and may increase
 *          code size and execution time on embedded systems.
 */
void ltc2946_print(const ltc2946_values_t *v);

#ifdef __cplusplus
}
#endif

#endif // LTC2976_H
