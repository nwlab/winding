/**
 * @file    one_wire.h
 * @author  Software development team
 * @brief   Driver for 1Wire device.
 * @version 1.0
 * @date    2024-10-29
 */

#ifndef ONE_WIRE_H
#define ONE_WIRE_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Core includes. */
#include <onewire/ds2482.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================================================================
 * Public Macro Definitions
 * ==========================================================================*/

#define OW_SEARCH_ROM_CMD   (0xF0)
#define OW_READ_ROM_CMD     (0x33)
#define OW_MATCH_ROM_CMD    (0x55)
#define OW_SKIP_ROM_CMD     (0xCC)
#define OW_ALARM_SEARCH_CMD (0xEC)

#define OW_CONVERT_T_FCMD        (0x44)
#define OW_WRITE_SCRATCHPAD_FCMD (0x4E)
#define OW_READ_SCRATCHPAD_FCMD  (0xBE)
#define OW_COPY_SCRATCHPAD_FCMD  (0x48)
#define OW_RECALL_EE_FCMD        (0xB8)
#define OW_READ_PSUP_FCMD        (0xB4)

/* ============================================================================
 * Global Function Declarations (prototypes)
 * ==========================================================================*/

/**
 * @brief  Resets OneWire bus
 * @note   Sends reset command for OneWire
 * @param[in]  handle Pointer to working onewire structure
 * @retval true  : device present
 *         false : no device present
 */
bool ow_reset(ds2482_handle_t handle);

/**
 * @brief  Writes byte to bus
 * @param[in]  handle Pointer to working onewire structure
 * @param[in]  byte: 8-bit value to write over OneWire protocol
 * @retval None
 */
void ow_write_byte(ds2482_handle_t handle, uint8_t byte_value);

/**
 * @brief  Reads byte from one wire bus
 * @param[in]  handle Pointer to working onewire structure
 * @retval Byte from read operation
 */
uint8_t ow_read_byte(ds2482_handle_t handle);

/**
 * @brief  Writes single bit to onewire bus
 * @param[in]  handle Pointer to working onewire structure
 * @param      bit_value: Bit value to send, 1 or 0
 * @retval None
 */
void ow_write_bit(ds2482_handle_t handle, uint8_t bit_value);

/**
 * @brief  Reads single bit from one wire bus
 * @param[in]  handle Pointer to working onewire structure
 * @retval Bit value:
 *            - 0: Bit is low (zero)
 *            - > 0: Bit is high (one)
 */
uint8_t ow_read_bit(ds2482_handle_t handle);

/**
 * @brief  Starts search, reset states first
 * @note   When you want to search for ALL devices on one onewire port,
 *         you should first use this function.
@verbatim
/...Initialization before
status = ow_first(ds2482_handle);
while (status) {
    //Check for new device
    status = ow_next(ds2482_handle);
}
@endverbatim
 * @param[in]  handle Pointer to working onewire structure
 * @retval  Device status:
 *            - false: No devices detected
 *            - true: Device detected
 */
bool ow_first(ds2482_handle_t handle);

/**
 * @brief  Reads next device
 * @note   Use @ref ow_first to start searching
 * @param[in]  handle Pointer to working onewire structure
 * @retval  Device status:
 *            - false: No devices detected any more
 *            - true: New device detected
 */
bool ow_next(ds2482_handle_t handle);

void ow_target_setup(ds2482_handle_t handle, unsigned char family_code);
void ow_family_skip_setup(ds2482_handle_t handle);

/**
 * @brief  Searches for OneWire devices on specific Onewire port
 * @note   Not meant for public use. Use @ref ow_first and @ref ow_next for
 * this.
 * @param[in]  handle Pointer to working onewire structure
 * @retval  Device status:
 *            - false: No devices detected
 *            - true: Device detected
 */
bool ow_search(ds2482_handle_t handle);

/*
uint8_t OW_touch_bit(enum I2C_CH ch, uint8_t sendbit, uint8_t addr);
uint8_t OW_touch_byte(enum I2C_CH ch, uint8_t sendbyte, uint8_t addr);
void    OW_block(enum I2C_CH ch, uint8_t *tran_buf, uint8_t tran_len,
uint8_t addr); uint8_t OW_verify(enum I2C_CH ch, uint8_t addr);
*/

#ifdef __cplusplus
}
#endif

#endif // ONE_WIRE_H
