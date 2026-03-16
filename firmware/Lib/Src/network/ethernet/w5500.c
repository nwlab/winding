/**
 * @file    w5500.c
 * @author  Software development team
 * @brief   W5500 Ethernet controller driver implementation.
 *
 * This file provides the implementation of the W5500 Ethernet driver,
 * including device initialization, SPI communication handling,
 * register access, socket management, and basic network operations.
 *
 * The driver is designed to operate on STM32 platforms using the HAL
 * SPI interface. Platform-specific dependencies (SPI, GPIO, delays)
 * should be abstracted to allow unit testing on host environments.
 *
 * @note In unit-test builds, hardware-specific accesses (e.g., RCC,
 *       GPIO, SPI registers) must be mocked or redirected to
 *       RAM-backed structures to avoid invalid memory access.
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <string.h> // for memcpy
#include <inttypes.h>

/* Core includes. */
#include <hal/yaa_gpio.h>
#include <hal/yaa_spi.h>
#include <yaa_macro.h>
#include <yaa_sal.h>
#include <yaa_types.h>

#include <network/ethernet/w5500.h>
/* Temporary use LL API */
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_spi.h"
#include <stm32f4xx_hal.h>

#include <network/ethernet/w5500.h>

#include "w5500_internal.h"
#include "../yaa_socket_backend.h"

/* ============================================================================
 * Private Macro Definitions
 * ==========================================================================*/

// #define YAA_W5500_UNDERLINE

#ifndef YAA_DEFAULT_CONNECTION_BUFFER_SIZE
    #define YAA_DEFAULT_CONNECTION_BUFFER_SIZE 2048
#endif

#define W5500_RESET_TIMEOUT_CNT   100U

/* ============================================================================
 * Private Type Definitions
 * ==========================================================================*/

/* ============================================================================
 * Private Function Declaration
 * ==========================================================================*/

#ifdef YAA_W5500_UNDERLINE
#include <ether_w5500.h>
static void spi_ll_off();
static void spi_ll_on();
static uint8_t spi_ll_spio(uint8_t val);
static void wz_rst_on();
static void wz_rst_off();
#endif /* YAA_W5500_UNDERLINE */

static void w5500_irq_callback(yaa_gpio_port_t port, yaa_gpio_pin_t pin, void *ctx);

/* ============================================================================
 * Private Variable Definitions
 * ==========================================================================*/

#ifdef YAA_W5500_UNDERLINE
static LL_Ether spi_api = {
    .cs_on = spi_ll_on,
    .cs_off = spi_ll_off,
    .spio = spi_ll_spio,
    .rst_on = wz_rst_on,
    .rst_off = wz_rst_off
};

static wiz_NetInfo net_info = { .mac = { 0xEA, 0x11, 0x22, 0x33, 0x44, 0xEA },
                                .ip = { 10, 5, 0, 34 },
                                .sn = { 255, 255, 0, 0 },
                                .dhcp = NETINFO_STATIC };

static eth_w5500_ctx_t *eth_ctx = NULL;

#endif /* YAA_W5500_UNDERLINE */


/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

yaa_err_t eth_w5500_init(const eth_w5500_params_t *param, eth_w5500_handle_t *handle)
{
    yaa_err_t status = YAA_ERR_OK;

    if (param == NULL || handle == NULL)
    {
        return YAA_ERR_BADARG;
    }

    eth_w5500_ctx_t *ctx = (eth_w5500_ctx_t *)yaa_alloc(sizeof(eth_w5500_ctx_t));
    if (ctx == NULL)
    {
        W5500_ERR("No memory");
        return YAA_ERR_NOMEM;
    }

    ctx->gpio_cs = NULL;
    ctx->gpio_reset = NULL;
    ctx->gpio_int = NULL;

    if (param->cs_port != YAA_GPIO_PORT_NONE)
    {
        yaa_gpio_params_t gpio_cs_param = {
            .port = param->cs_port,
            .pin = param->cs_pin,
            .pull = YAA_GPIO_PULL_UP,
            .direction = YAA_GPIO_DIRECTION_OUTPUT,
            .cb = NULL,
        };
        status = yaa_gpio_init(&gpio_cs_param, &ctx->gpio_cs);
        if (status != YAA_ERR_OK)
        {
            yaa_free(ctx);
            return status;
        }
        (void)yaa_gpio_set(ctx->gpio_cs, 1);
    }

    if (param->rst_port != YAA_GPIO_PORT_NONE)
    {
        yaa_gpio_params_t gpio_rst_param = {
            .port = param->rst_port,
            .pin = param->rst_pin,
            .pull = YAA_GPIO_PULL_UP,
            .direction = YAA_GPIO_DIRECTION_OUTPUT,
            .cb = NULL,
        };
        status = yaa_gpio_init(&gpio_rst_param, &ctx->gpio_reset);
        if (status != YAA_ERR_OK)
        {
            goto free_ctx;
        }
        (void)yaa_gpio_set(ctx->gpio_reset, 1);
    }

    if (param->irq_port != YAA_GPIO_PORT_NONE)
    {
        yaa_gpio_params_t gpio_irq_param = {
            .port = param->irq_port,
            .pin = param->irq_pin,
            .pull = YAA_GPIO_PULL_UP,
            .direction = YAA_GPIO_DIRECTION_INTERRUPT,
            .irq_trigger = YAA_GPIO_IRQ_TRIGGER_EDGE_FALLING,
            .cb = w5500_irq_callback,
            .user_ctx = ctx
        };
        status = yaa_gpio_init(&gpio_irq_param, &ctx->gpio_int);
        if (status != YAA_ERR_OK)
        {
            goto free_ctx;
        }
    }

    ctx->spi = param->spi;
    memcpy(ctx->mac_addr, param->mac, YAA_NET_ETH_ADDR_LEN);
	memcpy(&ctx->retry_time, &param->retry_time, 1);
	memcpy(&ctx->retry_count, &param->retry_count, 1);

    uint32_t buff_size;
    if (param->max_buff_size != 0)
    {
        buff_size = param->max_buff_size;
    }
    else
    {
        buff_size = YAA_DEFAULT_CONNECTION_BUFFER_SIZE;
    }

    if (buff_size <= 1024)
        ctx->buff_kb = 1;
    else if (buff_size <= 2048)
        ctx->buff_kb = 2;
    else if (buff_size <= 4096)
        ctx->buff_kb = 4;
    else if (buff_size <= 8192)
        ctx->buff_kb = 8;
    else
        ctx->buff_kb = 16;

    ctx->sock_any_port = SOCK_ANY_PORT_NUM;

    w5500_sockets_init(ctx);

    status = w5500_setup(ctx);
    if (status != YAA_ERR_OK)
    {
        W5500_ERR("Fail setup W5500");
        goto free_ctx;
    }

    status = w5500_set_ip(ctx, param->ip);
    if (status != YAA_ERR_OK)
    {
        goto free_ctx;
    }

    status = w5500_set_subnet(ctx, param->subnet);
    if (status != YAA_ERR_OK)
    {
        goto free_ctx;
    }

    status = w5500_set_gateway(ctx, param->gw);
    if (status != YAA_ERR_OK)
    {
        goto free_ctx;
    }

    status = w5500_check_link_status(ctx);
    W5500_ERR("Link is %s", status == YAA_ERR_OK ? "UP" : "DOWN");

    status = w5500_socket_irq_config(ctx, 0xFF);

#if defined(DEBUG)
    w5500_dump_reg(ctx);
#endif

#ifdef YAA_W5500_UNDERLINE
    // Need for callbacks
    eth_ctx = ctx;

    if (ether_w5500_init(&spi_api, &net_info) != 0)
    {
        W5500_ERR("Fail init W5500");
        yaa_free(ctx);
        return YAA_ERR_FAIL;
    }

    w5500_dump_reg(ctx);
#endif /* YAA_W5500_UNDERLINE */

    *handle = (eth_w5500_handle_t)ctx;

    W5500_DEB("Init done");

    return YAA_ERR_OK;

free_ctx:
	yaa_free(ctx);

	return status;
}

yaa_err_t eth_w5500_update_link_status(eth_w5500_handle_t handle, yaa_eth_link_state_t *up)
{
    eth_w5500_ctx_t *ctx = YAA_CAST(eth_w5500_ctx_t *, handle);
    if (ctx == NULL)
    {
        return YAA_ERR_NORESOURCE;
    }
#ifdef YAA_W5500_UNDERLINE
    int8_t phylink = wizphy_getphylink();

    if (phylink < 0)
    {
        W5500_ERR("Fail get phy link W5500");
        return YAA_ERR_IO;
    }

    W5500_DEB("PHY Link is %d", phylink);

    if (up)
    {
        *up = (phylink != 0 ? YAA_ETH_LINK_UP : YAA_ETH_LINK_DOWN);
    }
#endif

    yaa_err_t status = w5500_check_link_status(ctx);
    W5500_DEB("Link is %s", status == YAA_ERR_OK ? "UP" : "DOWN");

    if (up)
    {
        *up = (status == YAA_ERR_OK ? YAA_ETH_LINK_UP : YAA_ETH_LINK_DOWN);
    }

    return YAA_ERR_OK;
}

yaa_err_t eth_w5500_destroy(eth_w5500_handle_t handle)
{
    eth_w5500_ctx_t *ctx = YAA_CAST(eth_w5500_ctx_t *, handle);
    if (ctx == NULL)
    {
        return YAA_ERR_NORESOURCE;
    }

    if (ctx->gpio_cs != NULL)
    {
        yaa_gpio_destroy(ctx->gpio_cs);
    }

    if (ctx->gpio_reset != NULL)
    {
        yaa_gpio_destroy(ctx->gpio_reset);
    }

    if (ctx->gpio_int != NULL)
    {
        yaa_gpio_destroy(ctx->gpio_int);
    }
#ifdef YAA_W5500_UNDERLINE
    eth_ctx = NULL;
#endif
    yaa_free(ctx);

    return YAA_ERR_OK;
}

yaa_err_t eth_w5500_set_mac(eth_w5500_handle_t handle, const uint8_t mac[6])
{
    eth_w5500_ctx_t *ctx = YAA_CAST(eth_w5500_ctx_t *, handle);
    if (ctx == NULL)
    {
        return YAA_ERR_NORESOURCE;
    }
    return w5500_set_mac(ctx, mac);
}

yaa_err_t eth_w5500_set_ip(eth_w5500_handle_t handle, const uint8_t ip[4])
{
    eth_w5500_ctx_t *ctx = YAA_CAST(eth_w5500_ctx_t *, handle);
    if (ctx == NULL)
    {
        return YAA_ERR_NORESOURCE;
    }
    return w5500_set_ip(ctx, ip);
}

yaa_err_t eth_w5500_set_subnet(eth_w5500_handle_t handle, const uint8_t subnet[4])
{
    eth_w5500_ctx_t *ctx = YAA_CAST(eth_w5500_ctx_t *, handle);
    if (ctx == NULL)
    {
        return YAA_ERR_NORESOURCE;
    }
    return w5500_set_subnet(ctx, subnet);
}

yaa_err_t eth_w5500_set_gateway(eth_w5500_handle_t handle, const uint8_t gateway[4])
{
    eth_w5500_ctx_t *ctx = YAA_CAST(eth_w5500_ctx_t *, handle);
    if (ctx == NULL)
    {
        return YAA_ERR_NORESOURCE;
    }
    return w5500_set_gateway(ctx, gateway);
}

yaa_err_t eth_w5500_get_mac(eth_w5500_handle_t handle, uint8_t mac[6])
{
    eth_w5500_ctx_t *ctx = YAA_CAST(eth_w5500_ctx_t *, handle);
    if (ctx == NULL)
    {
        return YAA_ERR_NORESOURCE;
    }
    return w5500_get_mac(ctx, mac);
}

yaa_err_t eth_w5500_get_ip(eth_w5500_handle_t handle, uint8_t ip[4])
{
    eth_w5500_ctx_t *ctx = YAA_CAST(eth_w5500_ctx_t *, handle);
    if (ctx == NULL)
    {
        return YAA_ERR_NORESOURCE;
    }
    return w5500_get_ip(ctx, ip);
}

yaa_err_t eth_w5500_get_subnet(eth_w5500_handle_t handle, uint8_t subnet[4])
{
    eth_w5500_ctx_t *ctx = YAA_CAST(eth_w5500_ctx_t *, handle);
    if (ctx == NULL)
    {
        return YAA_ERR_NORESOURCE;
    }
    return w5500_get_subnet(ctx, subnet);
}

yaa_err_t eth_w5500_get_gateway(eth_w5500_handle_t handle, uint8_t gateway[4])
{
    eth_w5500_ctx_t *ctx = YAA_CAST(eth_w5500_ctx_t *, handle);
    if (ctx == NULL)
    {
        return YAA_ERR_NORESOURCE;
    }
    return w5500_get_gateway(ctx, gateway);
}

/* ============================================================================
 * Private Function Definitions
 * ==========================================================================*/

/**
 * @brief Pull CS line low
 *
 * @param ctx W5500 context
 *
 * @return yaa_err_t
 */
static inline yaa_err_t w5500_cs_low(const eth_w5500_ctx_t *ctx)
{
    return yaa_gpio_set(ctx->gpio_cs, 0);
}

/**
 * @brief Pull CS line high
 *
 * @param ctx W5500 context
 *
 * @return yaa_err_t
 */
static inline yaa_err_t w5500_cs_high(const eth_w5500_ctx_t *ctx)
{
    return yaa_gpio_set(ctx->gpio_cs, 1);
}

/******************************************************************************
 * @brief Write data to a W5500 register
 *
 * @param ctx   - The device descriptor
 * @param block - The block select bits (common registers, socket registers, etc.)
 * @param addr  - The register address
 * @param data  - The data buffer to write
 * @param len   - The data length
 *
 * @return 0 in case of success, negative error code otherwise
*******************************************************************************/
yaa_err_t w5500_reg_write(eth_w5500_ctx_t *ctx,
                           uint8_t block,
                           uint16_t addr,
                           const uint8_t *data,
                           uint16_t len)
{
    uint8_t spi_tx[3 + len];
    yaa_err_t ret;

    if (ctx == NULL)
    {
        return YAA_ERR_BADARG;
    }

    spi_tx[0] = W5500_BYTE_HIGH(addr);
    spi_tx[1] = W5500_BYTE_LOW(addr);
    spi_tx[2] = W5500_BSB(block) | W5500_RWB_WRITE | W5500_OM_VDM;

    memcpy(&spi_tx[3], data, len);

    (void)w5500_cs_low(ctx);

    ret = yaa_spi_transmitreceive(ctx->spi, spi_tx, NULL, 3 + len, 0);

    (void)w5500_cs_high(ctx);

    return ret;
}

/******************************************************************************
 * @brief Read data from a W5500 register
 *
 * @param ctx   - The device descriptor
 * @param block - The block select bits (common registers, socket registers, etc.)
 * @param addr  - The register address
 * @param data  - The buffer to store read data
 * @param len   - The data length to read
 *
 * @return 0 in case of success, negative error code otherwise
*******************************************************************************/
yaa_err_t w5500_reg_read(eth_w5500_ctx_t *ctx,
                          uint8_t block,
                          uint16_t addr,
                          uint8_t *data,
                          uint16_t len)
{
    uint8_t spi_buffer[3 + len];
    yaa_err_t ret;

    if (ctx == NULL)
    {
        return YAA_ERR_BADARG;
    }

    spi_buffer[0] = W5500_BYTE_HIGH(addr);
    spi_buffer[1] = W5500_BYTE_LOW(addr);
    spi_buffer[2] = W5500_BSB(block) | W5500_RWB_READ | W5500_OM_VDM;

    (void)w5500_cs_low(ctx);

    ret = yaa_spi_transmitreceive(ctx->spi, spi_buffer, spi_buffer, 3 + len, 3 + len);

    (void)w5500_cs_high(ctx);

    if (ret == YAA_ERR_OK)
    {
        memcpy(data, &spi_buffer[3], len);
#ifdef DEBUG
        if (len == 1) {
            W5500_DEB("Reg : 0x%04X [%02X]", addr, data[0]);
        } else if (len == 2) {
            W5500_DEB("Reg : 0x%04X [%02X, %02X]", addr, data[0], data[1]);
        } else if (len == 3) {
            W5500_DEB("Reg : 0x%04X [%02X, %02X, %02X]", addr, data[0], data[1], data[2]);
        }
#endif
    }

    return ret;
}

/******************************************************************************
 * @brief Read a 16-bit register reliably by reading until consecutive values match
 *
 * @param dev   - The device descriptor
 * @param block - The block select bits (common registers, socket registers, etc.)
 * @param addr  - The register address
 * @param value - Pointer to store the 16-bit value
 *
 * @return 0 in case of success, negative error code otherwise
 *******************************************************************************/
yaa_err_t w5500_read_16bit_reg(eth_w5500_ctx_t *dev, uint8_t block, uint16_t addr, uint16_t *value)
{
    yaa_err_t ret;
    uint8_t data[2];
    uint16_t val1, val2;
    int attempts = 0;
    const int max_attempts = 5;

    do
    {
        ret = w5500_reg_read(dev, block, addr, data, 2);
        if (ret)
        {
            return ret;
        }
        val1 = (data[0] << 8) | data[1];

        ret = w5500_reg_read(dev, block, addr, data, 2);
        if (ret)
        {
            return ret;
        }
        val2 = (data[0] << 8) | data[1];

        attempts++;
    } while (val1 != val2 && attempts < max_attempts);

    if (attempts >= max_attempts)
    {
        return YAA_ERR_IO;
    }

    *value = val1;
    return YAA_ERR_OK;
}

 /******************************************************************************
 * @brief Initialize socket data structures
 *
 * @param dev - The device descriptor
 *
 * @note Internal function, no return value
*******************************************************************************/
void w5500_sockets_init(eth_w5500_ctx_t *ctx)
{
	for (int i = 0; i <= W5500_MAX_SOCK_NUMBER; i++)
    {
        ctx->sockets[i].in_use = 0;
		ctx->sockets[i].protocol = W5500_Sn_MR_CLOSE;
		ctx->sockets[i].mss = 1460;
		ctx->sockets[i].tx_buf_size = 2;
		ctx->sockets[i].rx_buf_size = 2;
	}
}

/******************************************************************************
 * @brief Find an available socket in the socket mapping table
 *
 * @param dev         - The W5500 network device descriptor
 * @param sock_id     - Pointer to store the found physical socket ID
 *
 * @return 0 in case of success, -ENOENT if no free socket found
 *******************************************************************************/
yaa_err_t w5500_net_find_free_socket(eth_w5500_ctx_t *dev, uint8_t *sock_id)
{
    uint8_t i;

    for (i = 0; i <= W5500_MAX_SOCK_NUMBER; i++)
    {
        if (!dev->sockets[i].in_use)
        {
            *sock_id = i;
            return YAA_ERR_OK;
        }
    }
    return YAA_ERR_NORESOURCE;
}

/******************************************************************************
 * @brief Reset the W5500 chip using hardware or software method
 *
 * @param dev - The device descriptor
 *
 * @return 0 in case of success, negative error code otherwise
*******************************************************************************/
yaa_err_t w5500_reset(eth_w5500_ctx_t *ctx)
{
    yaa_err_t ret;
    uint8_t mr;
    uint8_t ver = 0x00;

    if (ctx->gpio_reset)
    {
        ret = yaa_gpio_set(ctx->gpio_reset, 0);
        if (ret != YAA_ERR_OK)
        {
            return ret;
        }

        yaa_mdelay(1); // 500 us min per datasheet

        ret = yaa_gpio_set(ctx->gpio_reset, 1);
        if (ret != YAA_ERR_OK)
        {
            return ret;
        }

        /* Wait for reset to complete */
        yaa_mdelay(2); // wait for internal reset

        W5500_DEB("GPIO reset done");
    }

    mr = W5500_MR_RST;
    ret = w5500_reg_write(ctx, W5500_COMMON_REG, W5500_MR, &mr, 1);
    if (ret != YAA_ERR_OK)
    {
        return ret;
    }

    yaa_mdelay(2); // small delay to let W5500 reset

    // Wait for reset completion
    uint32_t timeout = W5500_RESET_TIMEOUT_CNT;

    do
    {
        // Read mode register
        ret = w5500_reg_read(ctx, W5500_COMMON_REG, W5500_MR, &mr, 1);
        if (ret != YAA_ERR_OK)
        {
            return ret;
        }

        if ((mr & W5500_MR_RST) == 0)
        {
            break;  // Reset finished
        }

        yaa_mdelay(10);
    } while (--timeout > 0);

    if (timeout == 0)
    {
        W5500_ERR("Reset timeout");
        return YAA_ERR_TIMEOUT;
    }

    ret = w5500_reg_read(ctx, W5500_COMMON_REG, W5500_VERSIONR, &ver, 1);
    if (ret != YAA_ERR_OK)
    {
        return ret;
    }

    if (ver != W5500_CHIP_VERSION)
    {
        W5500_ERR("Chip version incorrect 0x%02X", ver);
        return YAA_ERR_NORESOURCE;
    }

    W5500_DEB("Reset done");
    return YAA_ERR_OK;
}

/******************************************************************************
 * @brief Setup the W5500 chip with basic configuration
 *
 * @param dev - The device descriptor
 *
 * @return 0 in case of success, negative error code otherwise
*******************************************************************************/
yaa_err_t w5500_setup(eth_w5500_ctx_t *ctx)
{
	yaa_err_t ret;

	ret = w5500_reset(ctx);
	if (ret != YAA_ERR_OK)
    {
		return ret;
    }

	ret = w5500_set_mac(ctx, ctx->mac_addr);
	if (ret != YAA_ERR_OK)
    {
		return ret;
    }

	ret = w5500_tcp_config(ctx, ctx->retry_time, ctx->retry_count);
	if (ret != YAA_ERR_OK)
    {
		return ret;
    }

    W5500_DEB("Setup done");
    return ret;
}

/******************************************************************************
 * @brief Check the Ethernet link status
 *
 * @param dev - The device descriptor
 *
 * @return 0 if link is up, YAA_ERR_NORESOURCE if link is down, other negative codes on error
*******************************************************************************/
yaa_err_t w5500_check_link_status(eth_w5500_ctx_t *dev)
{
    yaa_err_t ret;
    uint8_t phycfgr = 0;

    ret = w5500_reg_read(dev, W5500_COMMON_REG, W5500_PHYCFGR, &phycfgr, 1);
    if (ret != YAA_ERR_OK)
    {
        return ret;
    }

    W5500_DEB("PHY Config REG 0x%02X", phycfgr);

    if (!yaa_field_get(W5500_PHYCFGR_LNK, phycfgr))
    {
        return YAA_ERR_NORESOURCE;
    }

    return YAA_ERR_OK;
}

yaa_err_t w5500_global_irq_config(eth_w5500_ctx_t *dev, uint8_t imr)
{
    return w5500_reg_write(dev, W5500_COMMON_REG, W5500_IMR, &imr, 1);
}

yaa_err_t w5500_global_irq_clear(eth_w5500_ctx_t *dev, uint8_t imr)
{
    return w5500_reg_write(dev, W5500_COMMON_REG, W5500_IR, &imr, 1);
}

uint8_t w5500_global_irq_get(eth_w5500_ctx_t *dev)
{
    uint8_t ir = 0x00;
    (void)w5500_reg_read(dev, W5500_COMMON_REG, W5500_IR, &ir, 1);
    return ir;
}

yaa_err_t w5500_socket_irq_config(eth_w5500_ctx_t *dev, uint8_t mask)
{
    return w5500_reg_write(dev, W5500_COMMON_REG, W5500_SIMR, &mask, 1);
}

yaa_err_t w5500_socket_irq_clear(eth_w5500_ctx_t *dev, uint8_t mask)
{
    return w5500_reg_write(dev, W5500_COMMON_REG, W5500_SIR, &mask, 1);
}

uint8_t w5500_socket_irq_get(eth_w5500_ctx_t *dev)
{
    uint8_t sir = 0x00;
    (void)w5500_reg_read(dev, W5500_COMMON_REG, W5500_SIR, &sir, 1);
    return sir;
}

void w5500_irq_handler(eth_w5500_ctx_t *dev)
{
    uint8_t sir = w5500_socket_irq_get(dev);

    for (uint8_t s = 0; s < 8; s++)
    {
        if (sir & (1 << s))
        {
            uint8_t sn_ir;

            w5500_socket_reg_read(dev, s, W5500_Sn_IR, &sn_ir, 1);

            // process events here

            // clear socket interrupt cause
            w5500_socket_reg_write(dev, s, W5500_Sn_IR, &sn_ir, 1);
        }
    }
}

/******************************************************************************
 * @brief Configure W5500 TCP parameters (RTR and RCR)
 *
 * @param dev         - The device descriptor
 * @param retry_time  - Retry timeout value in 100μs units (default: 2000 = 200ms)
 * @param retry_count - Maximum retry count (default: 8)
 *
 * @return 0 on success, negative error code otherwise
*******************************************************************************/
yaa_err_t w5500_tcp_config(eth_w5500_ctx_t *dev, uint16_t retry_time, uint8_t retry_count)
{
    yaa_err_t ret;
    uint8_t buf[2];

    if (retry_time)
    {
        buf[0] = W5500_BYTE_HIGH(retry_time);
        buf[1] = W5500_BYTE_LOW(retry_time);

        ret = w5500_reg_write(dev, W5500_COMMON_REG, W5500_RTR, buf, 2);
        if (ret != YAA_ERR_OK)
        {
            return ret;
        }
    }

    if (retry_count)
    {
        ret = w5500_reg_write(dev, W5500_COMMON_REG, W5500_RCR, &retry_count, 1);
        if (ret != YAA_ERR_OK)
        {
            return ret;
        }
    }

#if 0
    // Set TX and RX buffer size for socket 0
    buf[0] = W5500_Sn_TXBUF_SIZE_16KB;
    buf[1] = W5500_Sn_RXBUF_SIZE_16KB;
    (void)w5500_socket_reg_write(dev, 0, W5500_Sn_TXBUF_SIZE, &buf[0], 1);
    (void)w5500_socket_reg_write(dev, 0, W5500_Sn_RXBUF_SIZE, &buf[1], 1);

    // Sockets 1 to 7 are not used
    buf[0] = W5500_Sn_TXBUF_SIZE_0KB;
    buf[1] = W5500_Sn_RXBUF_SIZE_0KB;
    for(int i = 1; i <= 7; i++)
    {
        (void)w5500_socket_reg_write(dev, i, W5500_Sn_TXBUF_SIZE, &buf[0], 1);
        (void)w5500_socket_reg_write(dev, i, W5500_Sn_RXBUF_SIZE, &buf[1], 1);
    }

    // Configure socket 0 in MACRAW mode
    buf[0] = W5500_Sn_MR_MACRAW;
    w5500_socket_reg_write(dev, 0, W5500_Sn_MR, &buf[0], 1);

    // Open socket 0
    (void)w5500_socket_command_write(dev, 0, W5500_Sn_CR_OPEN);

    // Wait for command completion
    uint32_t timeout = 100;
    uint8_t status = 0;
    do
    {
        // Read status register
        (void)w5500_socket_read_status(dev, 0, &status);
        // Check the status of the socket

        if (status == W5500_Sn_SR_MACRAW)
        {
            break;  // MACRAW is set
        }
        yaa_mdelay(10);
    } while (--timeout > 0);

    if (timeout == 0)
    {
        W5500_ERR("Open MACRAW timeout");
        return YAA_ERR_TIMEOUT;
    }

    W5500_DEB("MACRAW open OK");

    // Configure socket 0 interrupts
    buf[0] = W5500_Sn_IMR_SEND_OK | W5500_Sn_IMR_RECV;
    (void)w5500_socket_reg_write(dev, 0, W5500_Sn_IMR, &buf[0], 1);

    // Enable socket 0 interrupts
    buf[0] = W5500_SIMR_S0_IMR;
    (void)w5500_reg_write(dev, W5500_COMMON_REG, W5500_SIMR, &buf[0], 1);
#endif
    W5500_DEB("TCP config done");

    return YAA_ERR_OK;
}

/******************************************************************************
 * @brief Set MAC address
 *
 * @param dev - The device descriptor
 * @param mac - 6-byte MAC address array
 *
 * @return 0 in case of success, negative error code otherwise
 *******************************************************************************/
yaa_err_t w5500_set_mac(eth_w5500_ctx_t *ctx, const uint8_t mac[6])
{
    W5500_DEB("Set MAC %02X:%02X:%02X:%02X:%02X:%02X",
              mac[0], mac[1], mac[2],
              mac[3], mac[4], mac[5]);

    return w5500_reg_write(ctx, W5500_COMMON_REG, W5500_SHAR, mac, 6);
}

/******************************************************************************
 * @brief Set IP address
 *
 * @param dev - The device descriptor
 * @param ip  - 4-byte IP address array
 *
 * @return 0 in case of success, negative error code otherwise
 *******************************************************************************/
yaa_err_t w5500_set_ip(eth_w5500_ctx_t *ctx, const uint8_t ip[4])
{
    W5500_DEB("Set IP %u.%u.%u.%u",
              ip[0], ip[1], ip[2], ip[3]);

    return w5500_reg_write(ctx, W5500_COMMON_REG, W5500_SIPR, ip, 4);
}

/******************************************************************************
 * @brief Set subnet mask
 *
 * @param dev    - The device descriptor
 * @param subnet - 4-byte subnet mask array
 *
 * @return 0 in case of success, negative error code otherwise
 *******************************************************************************/
yaa_err_t w5500_set_subnet(eth_w5500_ctx_t *ctx, const uint8_t subnet[4])
{
    W5500_DEB("Set Subnet %u.%u.%u.%u",
              subnet[0], subnet[1], subnet[2], subnet[3]);

    return w5500_reg_write(ctx, W5500_COMMON_REG, W5500_SUBR, subnet, 4);
}

/******************************************************************************
 * @brief Set gateway address
 *
 * @param dev     - The device descriptor
 * @param gateway - 4-byte gateway address array
 *
 * @return 0 in case of success, negative error code otherwise
 *******************************************************************************/
yaa_err_t w5500_set_gateway(eth_w5500_ctx_t *ctx, const uint8_t gateway[4])
{
    W5500_DEB("Set Gateway %u.%u.%u.%u",
              gateway[0], gateway[1], gateway[2], gateway[3]);

    return w5500_reg_write(ctx, W5500_COMMON_REG, W5500_GAR, gateway, 4);
}

/******************************************************************************
 * @brief Get MAC address
 *
 * @param dev - The device descriptor
 * @param mac - Buffer to store the 6-byte MAC address
 *
 * @return 0 in case of success, negative error code otherwise
 *******************************************************************************/
yaa_err_t w5500_get_mac(eth_w5500_ctx_t *ctx, uint8_t mac[6])
{
    return w5500_reg_read(ctx, W5500_COMMON_REG, W5500_SHAR, mac, 6);
}

/******************************************************************************
 * @brief Get IP address
 *
 * @param dev - The device descriptor
 * @param ip  - Buffer to store the 4-byte IP address
 *
 * @return 0 in case of success, negative error code otherwise
*******************************************************************************/
yaa_err_t w5500_get_ip(eth_w5500_ctx_t *ctx, uint8_t ip[4])
{
    return w5500_reg_read(ctx, W5500_COMMON_REG, W5500_SIPR, ip, 4);
}

/***************************************************************************//**
 * @brief Get subnet mask
 *
 * @param dev    - The device descriptor
 * @param subnet - Buffer to store the 4-byte subnet mask
 *
 * @return 0 in case of success, negative error code otherwise
*******************************************************************************/
yaa_err_t w5500_get_subnet(eth_w5500_ctx_t *ctx, uint8_t subnet[4])
{
    return w5500_reg_read(ctx, W5500_COMMON_REG, W5500_SUBR, subnet, 4);
}

/***************************************************************************//**
 * @brief Get gateway address
 *
 * @param dev     - The device descriptor
 * @param gateway - Buffer to store the 4-byte gateway address
 *
 * @return 0 in case of success, negative error code otherwise
*******************************************************************************/
yaa_err_t w5500_get_gateway(eth_w5500_ctx_t *ctx, uint8_t gateway[4])
{
    return w5500_reg_read(ctx, W5500_COMMON_REG, W5500_GAR, gateway, 4);
}

/******************************************************************************
 * @brief Convert socket_address to w5500_socket_address
 *
 * @param src - Source socket address (network interface)
 * @param dst - Destination socket address (W5500 specific)
 *
 * @return 0 on success, negative error code otherwise
*******************************************************************************/
yaa_err_t w5500_net_addr_net_to_w5500(const yaa_sockaddr_t *src, struct w5500_socket_address *dst)
{
    if (src == NULL || dst == NULL)
    {
        return YAA_ERR_BADARG;
    }

    uint32_t addr = src->addr.ip.v4.value;

    dst->ip[0] = (uint8_t)((addr >> 24) & 0xFF);
    dst->ip[1] = (uint8_t)((addr >> 16) & 0xFF);
    dst->ip[2] = (uint8_t)((addr >> 8)  & 0xFF);
    dst->ip[3] = (uint8_t)(addr & 0xFF);

    dst->port[0] = W5500_BYTE_HIGH(src->port);
    dst->port[1] = W5500_BYTE_LOW(src->port);

    return YAA_ERR_OK;
}

/******************************************************************************
 * @brief Convert w5500_socket_address to socket_address
 *
 * @param src - Source socket address (W5500 specific)
 * @param dst - Destination socket address (network interface)
 *
 * @return 0 on success, negative error code otherwise
*******************************************************************************/
yaa_err_t w5500_net_addr_w5500_to_net(struct w5500_socket_address *src, yaa_sockaddr_t *dst)
{
    if (src == NULL || dst == NULL)
    {
        return YAA_ERR_BADARG;
    }

    dst->addr.type = YAA_IPADDR_TYPE_V4;
    dst->addr.ip.v4.value =
          ((uint32_t)src->ip[0] << 24)
        | ((uint32_t)src->ip[1] << 16)
        | ((uint32_t)src->ip[2] << 8)
        | ((uint32_t)src->ip[3]);

    dst->port =
          ((uint16_t)src->port[0] << 8)
        | ((uint16_t)src->port[1]);

    return YAA_ERR_OK;
}

/******************************************************************************
 * @brief Dump registers for debugging purpose
 *
 * @param[in] interface Underlying network interface
 ******************************************************************************/

void w5500_dump_reg(eth_w5500_ctx_t *ctx)
{
    yaa_err_t ret;
    uint16_t addr;
    uint8_t buf[16];

    printf("W5500 COMMON REGISTER DUMP\r\n");
    printf("     00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\r\n");
    printf("----------------------------------------------------\r\n");

    memset(buf, 0, sizeof(buf));

    for (addr = 0; addr < 64; addr += 16)
    {
        ret = w5500_reg_read(ctx,
                             W5500_COMMON_REG,
                             addr,
                             buf,
                             sizeof(buf));

        printf("%02X : ", addr);

        if (ret != YAA_ERR_OK)
        {
            printf("Read error\r\n");
            continue;
        }

        for (uint8_t i = 0; i < sizeof(buf); i++)
        {
            printf("%02X ", buf[i]);
        }

        printf("\r\n");
    }

    printf("\r\n");
}

#ifdef YAA_W5500_UNDERLINE
static void spi_ll_on()
{
    if (eth_ctx == NULL)
    {
        return;
    }

    (void)yaa_gpio_set(eth_ctx->gpio_cs, 0);
}

static void spi_ll_off()
{
    if (eth_ctx == NULL)
    {
        return;
    }

    (void)yaa_gpio_set(eth_ctx->gpio_cs, 1);
}

static uint8_t spi_ll_spio(uint8_t val)
{
    LL_SPI_TransmitData8(SPI2, val);

    while (!LL_SPI_IsActiveFlag_RXNE(SPI2))
        ;
    return LL_SPI_ReceiveData8(SPI2);
}

static void wz_rst_on()
{
    if (eth_ctx == NULL)
    {
        return;
    }

    (void)yaa_gpio_set(eth_ctx->gpio_reset, 0);
}

static void wz_rst_off()
{
    if (eth_ctx == NULL)
    {
        return;
    }
    (void)yaa_gpio_set(eth_ctx->gpio_reset, 1);
}
#endif /* YAA_W5500_UNDERLINE */

static void w5500_irq_callback(yaa_gpio_port_t port, yaa_gpio_pin_t pin, void *ctx)
{
    YAA_UNUSED(port);
    YAA_UNUSED(pin);
    YAA_UNUSED(ctx);
    W5500_DEB("Irq port : %d, pin :%d", port, (int)pin);
    w5500_irq_handler(ctx);
}
