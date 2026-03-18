/**
 * @file    w5500_regs.h
 * @author  Software development team
 * @brief   Register map and bit definitions for the W5500 Ethernet controller.
 *
 * This file contains register addresses, block identifiers,
 * and bit masks for the W5500 chip.
 *
 * It does not contain driver logic and may be included by both
 * internal driver sources and unit tests.
 *
 * @note Register addresses are taken from the official
 *       WIZnet W5500 datasheet.
 */

#ifndef W5500_REGS_H
#define W5500_REGS_H

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Common Register Block (Block = 0x00)
 * ==========================================================================*/

/* Common Register Offsets */
#define W5500_MR             0x0000  /* Mode Register */
#define W5500_GAR            0x0001  /* Gateway Address Register (4 bytes) */
#define W5500_SUBR           0x0005  /* Subnet Mask Register (4 bytes) */
#define W5500_SHAR           0x0009  /* Source Hardware Address Register (6 bytes) */
#define W5500_SIPR           0x000F  /* Source IP Address Register (4 bytes) */
#define W5500_INTLEVEL       0x0013  /* Interrupt Low Level Timer Register (2 bytes) */
#define W5500_IR             0x0015  /* Interrupt Register */
#define W5500_IMR            0x0016  /* Interrupt Mask Register */
#define W5500_SIR            0x0017  /* Socket Interrupt Register */
#define W5500_SIMR           0x0018  /* Socket Interrupt Mask Register */
#define W5500_RTR            0x0019  /* Retry Time Register (2 bytes) */
#define W5500_RCR            0x001B  /* Retry Count Register */
#define W5500_PTIMER         0x001C  /* PPP LCP Request Timer Register */
#define W5500_PMAGIC         0x001D  /* PPP LCP Magic Number Register */
#define W5500_PHAR           0x001E  /* PPP Destination MAC Register (6 bytes) */
#define W5500_PSID           0x0024  /* PPP Session ID Register (2 bytes) */
#define W5500_PMRU           0x0026  /* PPP Maximum Segment Size (2 bytes) */
#define W5500_UIPR           0x0028  /* Unreachable IP Address Register (4 bytes) */
#define W5500_UPORTR         0x002C  /* Unreachable Port Register (2 bytes) */
#define W5500_PHYCFGR        0x002E  /* PHY Configuration Register */
#define W5500_VERSIONR       0x0039  /* Chip Version Register */

/* ============================================================================
 * Socket Register Offsets (Base per socket block)
 * ==========================================================================*/

/* Socket Register Offsets */
#define W5500_Sn_MR          0x0000  /* Socket n Mode Register */
#define W5500_Sn_CR          0x0001  /* Socket n Command Register */
#define W5500_Sn_IR          0x0002  /* Socket n Interrupt Register */
#define W5500_Sn_SR          0x0003  /* Socket n Status Register */
#define W5500_Sn_PORT        0x0004  /* Socket n Source Port Register (2 bytes) */
#define W5500_Sn_DHAR        0x0006  /* Socket n Destination Hardware Address Register (6 bytes) */
#define W5500_Sn_DIPR        0x000C  /* Socket n Destination IP Address Register (4 bytes) */
#define W5500_Sn_DPORT       0x0010  /* Socket n Destination Port Register (2 bytes) */
#define W5500_Sn_MSSR        0x0012  /* Socket n Maximum Segment Size Register (2 bytes) */
#define W5500_Sn_TOS         0x0015  /* Socket n Type of Service Register */
#define W5500_Sn_TTL         0x0016  /* Socket n Time to Live Register */
#define W5500_Sn_RXBUF_SIZE  0x001E  /* Socket n RX Buffer Size Register */
#define W5500_Sn_TXBUF_SIZE  0x001F  /* Socket n TX Buffer Size Register */
#define W5500_Sn_TX_FSR      0x0020  /* Socket n TX Free Size Register (2 bytes) */
#define W5500_Sn_TX_RD       0x0022  /* Socket n TX Read Pointer Register (2 bytes) */
#define W5500_Sn_TX_WR       0x0024  /* Socket n TX Write Pointer Register (2 bytes) */
#define W5500_Sn_RX_RSR      0x0026  /* Socket n RX Received Size Register (2 bytes) */
#define W5500_Sn_RX_RD       0x0028  /* Socket n RX Read Pointer Register (2 bytes) */
#define W5500_Sn_RX_WR       0x002A  /* Socket n RX Write Pointer Register (2 bytes) */
#define W5500_Sn_IMR         0x002C  /* Socket n Interrupt Mask Register */
#define W5500_Sn_FRAG        0x002D  /* Socket n Fragment Offset in IP Header (2 bytes) */
#define W5500_Sn_KPALVTR     0x002F  /* Socket n Keep Alive Timer Register */

/* ============================================================================
 * Block Select Values (Control Phase)
 * ==========================================================================*/

/* Block Select Bit values */
#define W5500_COMMON_REG             0x00    /* Common Register Block */
#define W5500_SOCKET_REG_BLOCK(n)    (0x01 + (n) * 4)  /* Socket n Register Block */
#define W5500_SOCKET_TX_BUF_BLOCK(n) (0x02 + (n) * 4)  /* Socket n TX Buffer Block */
#define W5500_SOCKET_RX_BUF_BLOCK(n) (0x03 + (n) * 4)  /* Socket n RX Buffer Block */

/* ============================================================================
 * Mode Register (MR) Bit Definitions
 * ==========================================================================*/

/* Mode Register (MR) bit values */
#define W5500_MR_RST         RDNX_BIT(7)    /* Reset */
#define W5500_MR_WOL         RDNX_BIT(5)    /* Wake on LAN */
#define W5500_MR_PB          RDNX_BIT(4)    /* Ping Block */
#define W5500_MR_PPPOE       RDNX_BIT(3)    /* PPPoE Mode */
#define W5500_MR_FARP        RDNX_BIT(1)    /* Force ARP */

/* ============================================================================
 * Socket Mode Register (Sn_MR) Values
 * ==========================================================================*/

/* Socket Mode Register (Sn_MR) values */
#define W5500_Sn_MR_CLOSE    0x00    /* Closed */
#define W5500_Sn_MR_TCP      0x01    /* TCP */
#define W5500_Sn_MR_UDP      0x02    /* UDP */
#define W5500_Sn_MR_MACRAW   0x04    /* MAC Raw mode */
#define W5500_Sn_MR_NDMC     RDNX_BIT(5)    /* No Delayed ACK */
#define W5500_Sn_MR_MULTI    RDNX_BIT(7)    /* Multicasting */
#define W5500_Sn_MR_BCASTB   RDNX_BIT(6)    /* Broadcast Blocking */
#define W5500_Sn_MR_UCASTB   RDNX_BIT(4)    /* Unicast Blocking (UDP mode) */
#define W5500_Sn_MR_MIP6B    RDNX_BIT(4)    /* IPv6 Blocking (MACRAW mode) */

/* ============================================================================
 * Socket Command Register (Sn_CR) Commands
 * ==========================================================================*/

/* Socket Command Register (Sn_CR) values */
#define W5500_Sn_CR_OPEN             0x01    /* Initialize and open socket */
#define W5500_Sn_CR_LISTEN           0x02    /* Wait connection request in TCP mode (Server mode) */
#define W5500_Sn_CR_CONNECT          0x04    /* Send connection request in TCP mode (Client mode) */
#define W5500_Sn_CR_DISCON           0x08    /* Send closing request in TCP mode */
#define W5500_Sn_CR_CLOSE            0x10    /* Close socket */
#define W5500_Sn_CR_SEND             0x20    /* Send data */
#define W5500_Sn_CR_SEND_MAC         0x21    /* Send data with MAC address (UDP mode) */
#define W5500_Sn_CR_SEND_KEEP        0x22    /* Send keep-alive packet (TCP mode) */
#define W5500_Sn_CR_RECV             0x40    /* Receive data */

/* ============================================================================
 * Socket Status Register (Sn_SR) Values
 * ==========================================================================*/

/* Socket Status Register (Sn_SR) values */
#define W5500_Sn_SR_CLOSED           0x00    /* Socket is closed */
#define W5500_Sn_SR_INIT             0x13    /* Socket is initialized */
#define W5500_Sn_SR_LISTEN           0x14    /* Socket is in listen state */
#define W5500_Sn_SR_ESTABLISHED      0x17    /* Connection is established */
#define W5500_Sn_SR_CLOSE_WAIT       0x1C    /* Closing state */
#define W5500_Sn_SR_UDP              0x22    /* Socket is in UDP mode */
#define W5500_Sn_SR_MACRAW           0x42    /* Socket is in MACRAW mode */
#define W5500_Sn_SR_SYNSENT          0x15    /* SYN packet sent (TCP client) */
#define W5500_Sn_SR_SYNRECV          0x16    /* SYN packet received (TCP server) */
#define W5500_Sn_SR_FIN_WAIT         0x18    /* Socket closing (FIN sent) */
#define W5500_Sn_SR_CLOSING          0x1A    /* Socket closing (FIN exchanged) */
#define W5500_Sn_SR_TIME_WAIT        0x1B    /* Socket closing (2MSL timer active) */
#define W5500_Sn_SR_LAST_ACK         0x1D    /* Socket closing (Last ACK) */

/* ============================================================================
 * Socket Interrupt Mask Register Values
 * ==========================================================================*/

#define W5500_SIMR_S7_IMR               0x80
#define W5500_SIMR_S6_IMR               0x40
#define W5500_SIMR_S5_IMR               0x20
#define W5500_SIMR_S4_IMR               0x10
#define W5500_SIMR_S3_IMR               0x08
#define W5500_SIMR_S2_IMR               0x04
#define W5500_SIMR_S1_IMR               0x02
#define W5500_SIMR_S0_IMR               0x01

/* ============================================================================
 * Socket n Receive Buffer Size register
 * ==========================================================================*/

#define W5500_Sn_RXBUF_SIZE_0KB         0x00
#define W5500_Sn_RXBUF_SIZE_1KB         0x01
#define W5500_Sn_RXBUF_SIZE_2KB         0x02
#define W5500_Sn_RXBUF_SIZE_4KB         0x04
#define W5500_Sn_RXBUF_SIZE_8KB         0x08
#define W5500_Sn_RXBUF_SIZE_16KB        0x10

/* ============================================================================
 * Socket n Transmit Buffer Size register
 * ==========================================================================*/

#define W5500_Sn_TXBUF_SIZE_0KB         0x00
#define W5500_Sn_TXBUF_SIZE_1KB         0x01
#define W5500_Sn_TXBUF_SIZE_2KB         0x02
#define W5500_Sn_TXBUF_SIZE_4KB         0x04
#define W5500_Sn_TXBUF_SIZE_8KB         0x08
#define W5500_Sn_TXBUF_SIZE_16KB        0x10

/* ============================================================================
 * Socket n Interrupt Mask register
 * ==========================================================================*/

#define W5500_Sn_IMR_SEND_OK            0x10
#define W5500_Sn_IMR_TIMEOUT            0x08
#define W5500_Sn_IMR_RECV               0x04
#define W5500_Sn_IMR_DISCON             0x02
#define W5500_Sn_IMR_CON                0x01

#ifdef __cplusplus
}
#endif

#endif /* W5500_REGS_H */
