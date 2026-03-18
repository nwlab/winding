/**
 * @file    ade7758.h
 * @author  Software development team
 * @brief   ADE7758 3-Phase Energy Metering IC driver (SPI)
 *
 * The ADE7758 is a high-accuracy, 3-phase energy measurement IC
 * supporting active, reactive, and apparent energy measurements.
 *
 * This driver provides:
 *  - Device initialization and reset
 *  - Register read/write access
 *  - Active energy and RMS measurements
 *
 * Datasheet reference: Analog Devices ADE7758
 */

#ifndef ADE7758_H
#define ADE7758_H

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Includes
 * ==========================================================================*/
#include <stdint.h>
#include <stdbool.h>

#include <rdnx_types.h>
#include <rdnx_macro.h>
#include <hal/rdnx_spi.h>
#include <hal/rdnx_gpio.h>

/* ============================================================================
 * Configuration
 * ==========================================================================*/

/** Maximum SPI frequency supported by ADE7758 */
#define ADE7758_SPI_MAX_FREQ_HZ     (10000000U)

/** SPI mode for ADE7758 */
#define ADE7758_SPI_MODE            (1U)

/* SPI command flags */
#define ADE7758_SPI_CMD_READ        (0x00U)
#define ADE7758_SPI_CMD_WRITE       (0x80U)

/* ============================================================================
 * ADE7758 Registers
 * ==========================================================================*/

/**
 * @brief ADE7758 register addresses
 *
 * Register widths must follow datasheet specifications.
 */
typedef enum
{
    /* Energy accumulation (16-bit) */
    ADE7758_REG_AWATTHR     = 0x01,
    ADE7758_REG_BWATTHR     = 0x02,
    ADE7758_REG_CWATTHR     = 0x03,
    ADE7758_REG_AVARHR      = 0x04,
    ADE7758_REG_BVARHR      = 0x05,
    ADE7758_REG_CVARHR      = 0x06,
    ADE7758_REG_AVAHR       = 0x07,
    ADE7758_REG_BVAHR       = 0x08,
    ADE7758_REG_CVAHR       = 0x09,

    /* RMS measurements (24-bit) */
    ADE7758_REG_AIRMS       = 0x0A,
    ADE7758_REG_BIRMS       = 0x0B,
    ADE7758_REG_CIRMS       = 0x0C,
    ADE7758_REG_AVRMS       = 0x0D,
    ADE7758_REG_BVRMS       = 0x0E,
    ADE7758_REG_CVRMS       = 0x0F,

    /* Frequency, temperature, waveform */
    ADE7758_REG_FREQ        = 0x10,
    ADE7758_REG_TEMP        = 0x11,
    ADE7758_REG_WFORM       = 0x12,

    /* Configuration */
    ADE7758_REG_OPMODE      = 0x13,
    ADE7758_REG_MMODE       = 0x14,
    ADE7758_REG_WAVMODE     = 0x15,
    ADE7758_REG_COMPMODE    = 0x16,
    ADE7758_REG_LCYCMODE    = 0x17,

    /* Interrupts */
    ADE7758_REG_MASK        = 0x18,
    ADE7758_REG_STATUS      = 0x19,
    ADE7758_REG_RSTATUS     = 0x1A,

    ADE7758_REG_ZXTOUT      = 0x1B,
    ADE7758_REG_LINECYC     = 0x1C,
    ADE7758_REG_SAGCYC      = 0x1D,
    ADE7758_REG_SAGLVL      = 0x1E,
    ADE7758_REG_VPINTLVL    = 0x1F,
    ADE7758_REG_IPINTLVL    = 0x20,
    ADE7758_REG_VPEAK       = 0x21,
    ADE7758_REG_IPEAK       = 0x22,

    /* Calibration and gains */
    ADE7758_REG_GAIN        = 0x23,

    ADE7758_REG_AVRMSGAIN   = 0x24,
    ADE7758_REG_BVRMSGAIN   = 0x25,
    ADE7758_REG_CVRMSGAIN   = 0x26,
    ADE7758_REG_AIGAIN      = 0x27,
    ADE7758_REG_BIGAIN      = 0x28,
    ADE7758_REG_CIGAIN      = 0x29,
    ADE7758_REG_AWG         = 0x2A,
    ADE7758_REG_BWG         = 0x2B,
    ADE7758_REG_CWG         = 0x2C,
    ADE7758_REG_AVARG       = 0x2D,
    ADE7758_REG_BVARG       = 0x2E,
    ADE7758_REG_CVARG       = 0x2F,
    ADE7758_REG_AVAG        = 0x30,
    ADE7758_REG_BVAG        = 0x31,
    ADE7758_REG_CVAG        = 0x32,
    ADE7758_REG_AVRMSOS     = 0x33,
    ADE7758_REG_BVRMSOS     = 0x34,
    ADE7758_REG_CVRMSOS     = 0x35,
    ADE7758_REG_AIRMSOS     = 0x36,
    ADE7758_REG_BIRMSOS     = 0x37,
    ADE7758_REG_CIRMSOS     = 0x38,
    ADE7758_REG_AWAITOS     = 0x39,
    ADE7758_REG_BWAITOS     = 0x3A,
    ADE7758_REG_CWAITOS     = 0x3B,
    ADE7758_REG_AVAROS      = 0x3C,
    ADE7758_REG_BVAROS      = 0x3D,
    ADE7758_REG_CVAROS      = 0x3E,
    ADE7758_REG_APHCAL      = 0x3F,
    ADE7758_REG_BPHCAL      = 0x40,
    ADE7758_REG_CPHCAL      = 0x41,
    ADE7758_REG_WDIV        = 0x42,
    ADE7758_REG_VADIV       = 0x44,
    ADE7758_REG_VARDIV      = 0x43,
    ADE7758_REG_APCFNUM     = 0x45,
    ADE7758_REG_APCFDEN     = 0x46,
    ADE7758_REG_VARCFNUM    = 0x47,
    ADE7758_REG_VARCFDEN    = 0x48,

    /* Device info */
    ADE7758_REG_CHKSUM      = 0x7E,
    ADE7758_REG_VERSION     = 0x7F

} ade7758_reg_t;

/* ============================================================================
 * Bit Definitions
 * ==========================================================================*/

/* ---------------- OPMODE (0x13) ---------------- */
/** === setOpMode / getOpMode ===
 OPERATIONAL MODE REGISTER (0x13)
The general configuration of the ADE7758 is defined by writing to the OPMODE register.
Table 18 summarizes the functionality of each bit in the OPMODE register.

Bit Location		Bit Mnemonic		Default Value		Description
0					DISHPF				0					The HPFs in all current channel inputs are disabled when this bit is set.
1					DISLPF				0					The LPFs after the watt and VAR multipliers are disabled when this bit is set.
2					DISCF				1					The frequency outputs APCF and VARCF are disabled when this bit is set.
3 to 5				DISMOD				0					By setting these bits, ADE7758’s ADCs can be turned off. In normal operation, these bits should be left at Logic 0.
															DISMOD[2:0]				Description
															0	0	0				Normal operation.
															1	0	0				Redirect the voltage inputs to the signal paths for the current channels and the current inputs to the signal paths for the voltage channels.
															0	0	1				Switch off only the current channel ADCs.
															1	0	1				Switch off current channel ADCs and redirect the current input signals to the voltage channel signal paths.
															0	1	0				Switch off only the voltage channel ADCs.
															1	1	0				Switch off voltage channel ADCs and redirect the voltage input signals to the current channel signal paths.
															0	1	1				Put the ADE7758 in sleep mode.
															1	1	1				Put the ADE7758 in power-down mode (reduces AIDD to 1 mA typ).
6					SWRST				0					Software Chip Reset. A data transfer to the ADE7758 should not take place for at least 18 μs after a software reset.
7					RESERVED			0					This should be left at 0.

*/
#define ADE7758_OPMODE_DISHPF       (1U << 0)
#define ADE7758_OPMODE_DISLPF       (1U << 1)
#define ADE7758_OPMODE_DISCF        (1U << 2)
#define ADE7758_OPMODE_SWRST        (1U << 6)

/* ---------------- MMODE (0x14) ---------------- */
/** === setMMode / getMMode ===
MEASUREMENT MODE REGISTER (0x14)
The configuration of the PERIOD and peak measurements made by the ADE7758 is defined by writing to the MMODE register.
Table 19 summarizes the functionality of each bit in the MMODE register.

Bit Location		Bit Mnemonic		Default Value		Description
0 to 1				FREQSEL				0					These bits are used to select the source of the measurement of the voltage line frequency.
															FREQSEL1		FREQSEL0		Source
															0				0				Phase A
															0				1				Phase B
															1				0				Phase C
															1				1				Reserved
2 to 4				PEAKSEL				7					These bits select the phases used for the voltage and current peak registers.
															Setting Bit 2 switches the IPEAK and VPEAK registers to hold the absolute values
															of the largest current and voltage waveform (over a fixed number of half-line cycles)
															from Phase A. The number of half-line cycles is determined by the content of the
															LINECYC register. At the end of the LINECYC number of half-line cycles, the content
															of the registers is replaced with the new peak values. Similarly, setting Bit 3 turns
															on the peak detection for Phase B, and Bit 4 for Phase C. Note that if more than one
															bit is set, the VPEAK and IPEAK registers can hold values from two different phases, that is,
															the voltage and current peak are independently processed (see the Peak Current Detection section).
5 to 7				PKIRQSEL			7					These bits select the phases used for the peak interrupt detection.
															Setting Bit 5 switches on the monitoring of the absolute current and voltage waveform to Phase A.
															Similarly, setting Bit 6 turns on the waveform detection for Phase B, and Bit 7 for Phase C.
															Note that more than one bit can be set for detection on multiple phases.
															If the absolute values of the voltage or current waveform samples in the selected phases exceeds
															the preset level specified in the VPINTLVL or IPINTLVL registers the corresponding bit(s) in the
															STATUS registers are set (see the Peak Current Detection section).

*/
#define ADE7758_MMODE_FREQSEL_A     (0U << 0)
#define ADE7758_MMODE_FREQSEL_B     (1U << 0)
#define ADE7758_MMODE_FREQSEL_C     (2U << 0)

/* ---------------- WAVMODE (0x15) ---------------- */
/** === setWavMode / getWavMode ===
WAVEFORM MODE REGISTER (0x15)
The waveform sampling mode of the ADE7758 is defined by writing to the WAVMODE register.
Table 20 summarizes the functionality of each bit in the WAVMODE register.

Bit Location		Bit Mnemonic		Default Value 		Description
0 to 1				PHSEL				0					These bits are used to select the phase of the waveform sample.
															PHSEL[1:0]				Source
															0	0					Phase A
															0	1					Phase B
															1	0					Phase C
															1	1					Reserved
2 to 4				WAVSEL				0					These bits are used to select the type of waveform.
															WAVSEL[2:0]				Source
															0	0	0				Current
															0	0	1				Voltage
															0	1	0				Active Power Multiplier Output
															0	1	1				Reactive Power Multiplier Output
															1	0	0				VA Multiplier Output
															-Others-				Reserved
5 to 6				DTRT				0					These bits are used to select the data rate.
															DTRT[1:0]				Update Rate
															0	0					26.04 kSPS (CLKIN/3/128)
															0	1					13.02 kSPS (CLKIN/3/256)
															1	0					6.51 kSPS (CLKIN/3/512)
															1	1					3.25 kSPS (CLKIN/3/1024)
7					VACF				0					Setting this bit to Logic 1 switches the VARCF output pin to an output
															frequency that is proportional to the total apparent power (VA).
															In the default state, Logic 0, the VARCF pin outputs a frequency proportional
															to the total reactive power (VAR).
*/

/* ---------------- COMPMODE (0x16) ---------------- */
/** === setCompMode / getCompMode ===
COMPUTATIONAL MODE REGISTER (0x16)
The computational method of the ADE7758 is defined by writing to the COMPMODE register.

Bit Location	Bit Mnemonic	Default Value		Description
0 to 1			CONSEL			0					These bits are used to select the input to the energy accumulation registers.
													CONSEL[1:0] = 11 is reserved. IA, IB, and IC are IA, IB, and IC phase shifted by –90°, respectively.
													Registers		CONSEL[1, 0] = 00		CONSEL[1, 0] = 01			CONSEL[1, 0] = 10
													AWATTHR			VA × IA					VA × (IA – IB)				VA × (IA–IB)
													BWATTHR			VB × IB					0							0
													CWATTHR			VC × IC					VC × (IC – IB)				VC × IC

													AVARHR			VA × IA					VA × (IA – IB)				VA × (IA–IB)
													BVARHR			VB × IB					0							0
													CVARHR			VC × IC					VC × (IC – IB)				VC × IC

													AVAHR			VARMS × IARMS			VARMS × IARMS				VARMS × ARMS
													BVAHR			VBRMS × IBRMS			(VARMS + VCRMS)/2 × IBRMS	VARMS × IBRMS
													CVAHR			VCRMS × ICRMS			VCRMS × ICRMS				VCRMS × ICRMS

2 to 4			TERMSEL			7					These bits are used to select the phases to be included in the APCF and VARCF pulse outputs.
													Setting Bit 2 selects Phase A (the inputs to AWATTHR and AVARHR registers) to be included.
													Bit 3 and Bit 4 are for Phase B and Phase C, respectively.
													Setting all three bits enables the sum of all three phases to be included in the frequency outputs
													(see the Active Power Frequency Output and the Reactive Power Frequency Output sections).

5				ABS				0					Setting this bit places the APCF output pin in absolute only mode.
													Namely, the APCF output frequency is proportional to the sum of the absolute values of the watt-hour
													accumulation registers (AWATTHR, BWATTHR, and CWATTHR).
													Note that this bit only affects the APCF pin and has no effect on the content of the corresponding
													registers.

6				SAVAR			0					Setting this bit places the VARCF output pin in the signed adjusted mode.
													Namely, the VARCF output frequency is proportional to the sign-adjusted sum of the VAR-hour accumulation
													registers (AVARHR, BVARHR, and CVARHR).
													The sign of the VAR is determined from the sign of the watt calculation from the corresponding phase,
													that is, the sign of the VAR is flipped if the sign of the watt is negative, and if the watt is positive,
													there is no change to the sign of the VAR.
													Note that this bit only affects the VARCF pin and has no effect on the content of the corresponding
													registers.

7				NOLOAD			0					Setting this bit activates the no-load threshold in the ADE7758.
*/

/* ---------------- LCYCMODE (0x17) ---------------- */
/** === setLcycMode / getLcycMode ===

LINE CYCLE ACCUMULATION MODE REGISTER (0x17)
The functionalities involved the line-cycle accumulation mode in the ADE7758 are defined by writing to the LCYCMODE register.

Bit Location	Bit Mnemonic	Default Value		Description

0				LWATT			0					Setting this bit places the watt-hour accumulation registers
													(AWATTHR, BWATTHR, and CWATTHR registers) into line-cycle accumulation mode.
1				LVAR			0					Setting this bit places the VAR-hour accumulation registers (AVARHR, BVARHR, and CVARHR registers)
													into line-cycle accumulation mode.
2				LVA				0					Setting this bit places the VA-hour accumulation registers (AVAHR, BVAHR, and CVAHR registers)
													into line-cycle accumulation mode.
3 to 5			ZXSEL			7					These bits select the phases used for counting the number of zero crossings in the line-cycle
													accumulation mode. Bit 3, Bit 4, and Bit 5 select Phase A, Phase B, and Phase C, respectively.
													More than one phase can be selected for the zero-crossing detection,
													and the accumulation time is shortened accordingly.
6				RSTREAD			1					Setting this bit enables the read-with-reset for all the WATTHR, VARHR, and VAHR registers for all three
													phases, that is, a read to those registers resets the registers to 0 after the content of the registers
													have been read. This bit should be set to Logic 0 when the LWATT, LVAR, or LVA bits are set to Logic 1.
7				FREQSEL			0					Setting this bit causes the FREQ (0x10) register to display the period, instead of the frequency of the
													line input.
*/
#define ADE7758_LCYCMODE_LWATT      (1U << 0)
#define ADE7758_LCYCMODE_LVAR       (1U << 1)
#define ADE7758_LCYCMODE_LVA        (1U << 2)
#define ADE7758_LCYCMODE_ZXSEL_A    (1U << 3)
#define ADE7758_LCYCMODE_ZXSEL_B    (1U << 4)
#define ADE7758_LCYCMODE_ZXSEL_C    (1U << 5)
#define ADE7758_LCYCMODE_RSTREAD    (1U << 6)
#define ADE7758_LCYCMODE_FREQSEL    (1U << 7)

/* ---------------- STATUS / MASK ---------------- */
/** INTERRUPT MASK REGISTER (0x18)
When an interrupt event occurs in the ADE7758, the IRQ logic output goes active low if the mask bit for this event is Logic 1 in the MASK register.
The IRQ logic output is reset to its default collector open state when the RSTATUS register is read.
describes the function of each bit in the interrupt mask register.
**/

// The next table summarizes the function of each bit for
// the Interrupt Enable Register

/*             Bit Mask // Bit Location / Description
#define AEHF      0x0001 // bit 0 - Enables an interrupt when there is a change in Bit 14 of any one of the three WATTHR registers, that is, the WATTHR register is half full.
#define REHF      0x0002 // bit 1 - Enables an interrupt when there is a change in Bit 14 of any one of the three VARHR registers, that is, the VARHR register is half full.
#define VAEHF     0x0004 // bit 2 - Enables an interrupt when there is a 0 to 1 transition in the MSB of any one of the three VAHR registers, that is, the VAHR register is half full.
#define SAGA      0x0008 // bit 3 - Enables an interrupt when there is a SAG on the line voltage of the Phase A.
#define SAGB      0x0010 // bit 4 - Enables an interrupt when there is a SAG on the line voltage of the Phase B.
#define SAGC	  0x0020 // bit 5 - Enables an interrupt when there is a SAG on the line voltage of the Phase C.
#define ZXTOA     0x0040 // bit 6 - Enables an interrupt when there is a zero-crossing timeout detection on Phase A.
#define ZXTOB     0x0080 // bit 7 - Enables an interrupt when there is a zero-crossing timeout detection on Phase B.
#define ZXTOC     0x0100 // bit 8 - Enables an interrupt when there is a zero-crossing timeout detection on Phase C.
#define ZXA       0x0200 // bit 9 - Enables an interrupt when there is a zero crossing in the voltage channel of Phase A
#define ZXB       0x0400 // bit 10 - Enables an interrupt when there is a zero crossing in the voltage channel of Phase B
#define ZXC       0x0800 // bit 11 - Enables an interrupt when there is a zero crossing in the voltage channel of Phase C
#define LENERGY   0x1000 // bit 12 - Enables an interrupt when the energy accumulations over LINECYC are finished.
//RESERVED        0x2000 // bit 13 - RESERVED
#define PKV       0x4000 // bit 14 - Enables an interrupt when the voltage input selected in the MMODE register is above the value in the VPINTLVL register.
#define PKI       0x8000 // bit 15 - Enables an interrupt when the current input selected in the MMODE register is above the value in the IPINTLVL register.
#define WFSM    0x010000 // bit 16 - Enables an interrupt when data is present in the WAVEMODE register.
#define REVPAP  0x020000 // bit 17 - Enables an interrupt when there is a sign change in the watt calculation among any one of the phases specified by the TERMSEL bits in the COMPMODE register.
#define REVPRP  0x040000 // bit 18 - Enables an interrupt when there is a sign change in the VAR calculation among any one of the phases specified by the TERMSEL bits in the COMPMODE register.
#define SEQERR  0x080000 // bit 19 - Enables an interrupt when the zero crossing from Phase A is followed not by the zero crossing of Phase C but with that of Phase B.
*/
/** INTERRUPT STATUS REGISTER (0x19)/RESET INTERRUPT STATUS REGISTER (0x1A)
The interrupt status register is used to determine the source of an interrupt event.
When an interrupt event occurs in the ADE7758, the corresponding flag in the interrupt status register is set.
The IRQ pin goes active low if the corresponding bit in the interrupt mask register is set.
When the MCU services the interrupt, it must first carry out a read from the interrupt status register to determine the source of the interrupt.
All the interrupts in the interrupt status register stay at their logic high state after an event occurs.
The state of the interrupt bit in the interrupt status register is reset to its default value once the reset interrupt status register is read.
**/

// The next table summarizes the function of each bit for
// the Interrupt Status Register, the Reset Interrupt Status Register.

//             Bit Mask // Bit Location / Description
#define ADE7758_INT_AEHF            (1U << 0) // bit 0 - Indicates that an interrupt was caused by a change in Bit 14 among any one of the three WATTHR registers, that is, the WATTHR register is half full.
#define ADE7758_INT_REHF            (1U << 1) // bit 1 - Indicates that an interrupt was caused by a change in Bit 14 among any one of the three VARHR registers, that is, the VARHR register is half full.
#define ADE7758_INT_VAEHF           (1U << 2) // bit 2 - Indicates that an interrupt was caused by a 0 to 1 transition in Bit 15 among any one of the three VAHR registers, that is, the VAHR register is half full.
#define ADE7758_INT_SAGA            (1U << 3) // bit 3 - Indicates that an interrupt was caused by a SAG on the line voltage of the Phase A.
#define ADE7758_INT_SAGB            (1U << 4) // bit 4 - Indicates that an interrupt was caused by a SAG on the line voltage of the Phase B.
#define ADE7758_INT_SAGC            (1U << 5) // bit 5 - Indicates that an interrupt was caused by a SAG on the line voltage of the Phase C.
#define ADE7758_INT_ZXTOA           (1U << 6) // bit 6 - Indicates that an interrupt was caused by a missing zero crossing on the line voltage of the Phase A.
#define ADE7758_INT_ZXTOB           (1U << 7) // bit 7 - Indicates that an interrupt was caused by a missing zero crossing on the line voltage of the Phase B.
#define ADE7758_INT_ZXTOC           (1U << 8) // bit 8 - Indicates that an interrupt was caused by a missing zero crossing on the line voltage of the Phase C
#define ADE7758_INT_ZXA             (1U << 9) // bit 9 - Indicates a detection of a rising edge zero crossing in the voltage channel of Phase A.
#define ADE7758_INT_ZXB             (1U << 10) // bit 9 - Indicates a detection of a rising edge zero crossing in the voltage channel of Phase A.
#define ADE7758_INT_ZXC             (1U << 11) // bit 10 - Indicates a detection of a rising edge zero crossing in the voltage channel of Phase B.
#define ADE7758_INT_LENERGY         (1U << 12) // bit 12 - In line energy accumulation, indicates the end of an integration over an integer number of half- line cycles (LINECYC). See the Calibration section.
#define ADE7758_INT_RESET           (1U << 13) // bit 13 - Indicates that the 5 V power supply is below 4 V. Enables a software reset of the ADE7758 and sets the registers back to their default values. This bit in the STATUS or RSTATUS register is logic high for only one clock cycle after a reset event.
#define ADE7758_INT_PKV             (1U << 14) // bit 14 - Indicates that an interrupt was caused when the selected voltage input is above the value in the VPINTLVL register.
#define ADE7758_INT_PKI             (1U << 15) // bit 15 - Indicates that an interrupt was caused when the selected current input is above the value in the IPINTLVL register.
#define ADE7758_INT_WFSM            (1U << 16) // bit 16 - Indicates that new data is present in the waveform register.
#define ADE7758_INT_REVPAP          (1U << 17) // bit 17 - Indicates that an interrupt was caused by a sign change in the watt calculation among any one of the phases specified by the TERMSEL bits in the COMPMODE register.
#define ADE7758_INT_REVPRP          (1U << 18) // bit 18 - Indicates that an interrupt was caused by a sign change in the VAR calculation among any one of the phases specified by the TERMSEL bits in the COMPMODE register.
#define ADE7758_INT_SEQERR          (1U << 19) // bit 19 - Indicates that an interrupt was caused by a zero crossing from Phase A followed not by the zero crossing of Phase C but by that of Phase B.


/* ============================================================================
 * Data Types
 * ==========================================================================*/

/**
 * @brief ADE7758 initialization parameters
 */
typedef struct
{
    rdnx_spi_handle_t spi;        /**< SPI peripheral handle */
    rdnx_gpio_port_t cs_port;     /**< GPIO port for chip select */
    rdnx_gpio_pin_t  cs_pin;      /**< GPIO pin for chip select */
    rdnx_gpio_port_t irq_port;     /**< GPIO port for IRQ */
    rdnx_gpio_pin_t  irq_pin;      /**< GPIO pin for IRQ */

    /* Function pointers for user-defined lock/unlock mechanisms */
    rdnx_err_t (*lock)(void);     /**< User-defined lock function */
    rdnx_err_t (*unlock)(void);   /**< User-defined unlock function */
} ade7758_params_t;

/**
 * @brief ADE7758 opaque driver handle
 */
typedef struct ade7758_ctx *ade7758_handle_t;

/* ============================================================================
 * Measurement Data Structures
 * ==========================================================================*/

/**
 * @brief ADE7758 measurement snapshot
 *
 * Contains raw register values read from the ADE7758.
 * All fields store unscaled register codes as provided by the device.
 * Scaling to physical units (V, A, W, Wh, Hz, °C) must be applied by
 * higher-level application code.
 */
typedef struct ade7758_values
{
    /* ---------------- Active Energy (16-bit signed) ---------------- */
    int32_t awatt_code;        /**< Phase A active energy */
    int32_t bwatt_code;        /**< Phase B active energy */
    int32_t cwatt_code;        /**< Phase C active energy */

    /* ---------------- Reactive Energy (16-bit signed) ---------------- */
    int32_t avar_code;         /**< Phase A reactive energy */
    int32_t bvar_code;         /**< Phase B reactive energy */
    int32_t cvar_code;         /**< Phase C reactive energy */

    /* ---------------- Apparent Energy (16-bit signed) ---------------- */
    int32_t ava_code;          /**< Phase A apparent energy */
    int32_t bva_code;          /**< Phase B apparent energy */
    int32_t cva_code;          /**< Phase C apparent energy */

    /* ---------------- RMS Current (24-bit unsigned) ---------------- */
    uint32_t airms_code;       /**< Phase A RMS current */
    uint32_t birms_code;       /**< Phase B RMS current */
    uint32_t cirms_code;       /**< Phase C RMS current */

    /* ---------------- RMS Voltage (24-bit unsigned) ---------------- */
    uint32_t avrms_code;       /**< Phase A RMS voltage */
    uint32_t bvrms_code;       /**< Phase B RMS voltage */
    uint32_t cvrms_code;       /**< Phase C RMS voltage */

    /* ---------------- System Measurements ---------------- */
    uint16_t freq_code;        /**< Line frequency code */
    int16_t  temp_code;        /**< Internal temperature sensor code */

} ade7758_values_t;

static_assert(sizeof(ade7758_values_t) == 64, "Bad size of ade7758_values_t");

/* ============================================================================
 * Public Variable Declarations
 * ==========================================================================*/

// LSB Weights
static const float ADE7758_vref_mv = 3300.0f;     // Reference voltage in mV
static const float ADE7758_divider = 1.0f;        // Voltage divider ratio (if applicable)
static const float ADE7758_shunt_ohm = 0.01f;     // Shunt resistance in Ohms
static const float ADE7758_energy_lsb = 0.0001f;  // Energy LSB scaling factor (Wh per code)

/* ============================================================================
 * Public API
 * ==========================================================================*/

/**
 * @brief Initialize ADE7758 device
 *
 * Allocates internal context, initializes SPI and optional CS GPIO,
 * performs optional software reset.
 *
 * @param[in]  param   Initialization parameters
 * @param[out] handle  Pointer to receive device handle
 *
 * @return rdnx_err_t
 * @retval RDNX_ERR_OK       Initialization successful
 * @retval RDNX_ERR_BADARG   Invalid arguments
 * @retval RDNX_ERR_NOMEM    Memory allocation failed
 */
rdnx_err_t ade7758_init(const ade7758_params_t *param,
                        ade7758_handle_t *handle);

/**
 * @brief Deinitialize ADE7758 device
 *
 * Frees internal resources. After this call, the handle is invalid.
 *
 * @param handle Device handle
 *
 * @return rdnx_err_t
 */
rdnx_err_t ade7758_destroy(ade7758_handle_t handle);

/**
 * @brief Read ADE7758 register(s)
 *
 * @param handle Device handle
 * @param reg Register address
 * @param data Buffer to store read data
 * @param length Number of bytes to read
 *
 * @return rdnx_err_t
 */
rdnx_err_t ade7758_read_reg(ade7758_handle_t handle,
                            ade7758_reg_t reg,
                            uint8_t *data,
                            uint16_t length);

/**
 * @brief Write ADE7758 register(s)
 *
 * @param handle Device handle
 * @param reg Register address
 * @param data Buffer containing data to write
 * @param length Number of bytes to write
 *
 * @return rdnx_err_t
 */
rdnx_err_t ade7758_write_reg(ade7758_handle_t handle,
                             ade7758_reg_t reg,
                             const uint8_t *data,
                             uint16_t length);

/**
 * @brief Read active energy from AWATTHR register
 *
 * @param handle Device handle
 * @param value Pointer to store 16-bit signed energy value
 *
 * @return rdnx_err_t
 */
rdnx_err_t ade7758_read_active_energy(ade7758_handle_t handle,
                                      int32_t *value);

/**
 * @brief Read silicon revision/version of ADE7758
 *
 * @param handle Device handle
 * @param version Pointer to store revision value
 *
 * @return rdnx_err_t
 */
rdnx_err_t ade7758_get_version(ade7758_handle_t handle,
                               uint8_t *version);

/**
 * @brief Read all primary ADE7758 measurements
 *
 * Reads energy accumulation, RMS voltage/current, frequency
 * and temperature registers into a single structure.
 *
 * All values are returned as raw register codes.
 *
 * @param handle  ADE7758 device handle
 * @param values  Pointer to measurement structure
 *
 * @return rdnx_err_t
 */
rdnx_err_t ade7758_read_all(ade7758_handle_t handle,
                            ade7758_values_t *values);

/**
 * @brief Read
 *
 * @param handle Device handle
 * @param temperature
 *
 * @return rdnx_err_t
 */
rdnx_err_t ade7758_get_temperature(ade7758_handle_t handle,
                                   uint16_t *temperature);

/**
 * @brief Convert RMS voltage code to volts
 *
 * @param code        RMS voltage register value
 * @param vref_mv     ADC reference voltage in millivolts
 * @param divider    External voltage divider ratio
 *
 * @return Voltage in volts
 */
float ade7758_code_to_voltage(uint32_t code,
                              float vref_mv,
                              float divider);

/**
 * @brief Convert RMS current code to amperes
 *
 * @param code        RMS current register value
 * @param vref_mv     ADC reference voltage in millivolts
 * @param shunt_ohm   Current shunt resistance in ohms
 *
 * @return Current in amperes
 */
float ade7758_code_to_current(uint32_t code,
                              float vref_mv,
                              float shunt_ohm);

/**
 * @brief Convert active energy code to watt-hours
 *
 * @param code        Energy register value
 * @param energy_lsb  Energy LSB scaling factor (Wh / code)
 *
 * @return Energy in watt-hours
 */
float ade7758_code_to_energy(int32_t code,
                             float energy_lsb);

/**
 * @brief Convert frequency code to hertz
 *
 * @param code Frequency register value
 *
 * @return Frequency in Hz
 */
float ade7758_code_to_frequency(uint16_t code);

/**
 * @brief Convert temperature code to degrees Celsius
 *
 * @param code Temperature register value
 *
 * @return Temperature in °C
 */
float ade7758_code_to_temperature(int16_t code);

rdnx_err_t ade7758_stop_device(ade7758_handle_t handle);

void ade7758_print(const ade7758_values_t *v, float vref_mv, float divider, float shunt_ohm, float energy_lsb);

#ifdef __cplusplus
}
#endif

#endif /* ADE7758_H */
