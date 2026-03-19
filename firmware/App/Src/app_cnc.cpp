/**
 * @file app_cnc.c
 * @author Software development team
 * @brief Application logging implementation
 * @version 1.0
 * @date 2026-03-19
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


/* Core includes. */
#include <rdnx_macro.h>
#include <rtt/rdnx_rtt.h>

/* App includes. */
#include "app_cnc.h"
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif
    #include <devmw.h>
#ifdef __cplusplus
}
#endif

/* ============================================================================
 * Private Macro Definitions
 * ==========================================================================*/

 /* ============================================================================
 * Global Function Declarations (prototypes)
 * ==========================================================================*/

extern "C" int initloop(int code);
extern "C" int mainloop(int code);

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

uint8_t app_cnc_init(void)
{
    int code = 0;
    do
    {
        code = initloop(code);
    } while (code);

    return RDNX_ERR_OK;
}

uint8_t app_cnc_loop(void)
{
    static int code = 0;
    code = mainloop(code);
    return RDNX_ERR_OK;
}
