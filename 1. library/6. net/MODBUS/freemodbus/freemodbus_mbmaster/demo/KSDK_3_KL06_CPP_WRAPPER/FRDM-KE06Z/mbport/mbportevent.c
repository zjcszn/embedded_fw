/* 
 * MODBUS Library: KL06 KDS/PE/FreeRTOS port
 * Copyright (c)  2019 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>

/* ----------------------- Platform includes --------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "common/mbtypes.h"
#include "common/mbportlayer.h"
#include "common/mbframe.h"
#include "common/mbutils.h"

/* ----------------------- Defines ------------------------------------------*/
#define MAX_EVENT_HDLS          ( 4 )
#define IDX_INVALID             ( 255 )
#define EV_NONE                 ( 0 )

#define HDL_RESET( x ) do { \
    ( x )->ubIdx = IDX_INVALID; \
    ( x )->xQueueHdl = 0; \
} while( 0 );

/* ----------------------- Type definitions ---------------------------------*/
typedef struct
{
    UBYTE           ubIdx;
    xQueueHandle    xQueueHdl;
} xEventInternalHandle;

/* ----------------------- Static variables ---------------------------------*/
STATIC BOOL     bIsInitialized = FALSE;

STATIC xEventInternalHandle arxEventHdls[MAX_EVENT_HDLS];

/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Start implementation -----------------------------*/

eMBErrorCode
eMBPEventCreate( xMBPEventHandle * pxEventHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xQueueHandle    xQueueHdl;
    UBYTE           i;

    if( NULL != pxEventHdl )
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        if( !bIsInitialized )
        {
            for( i = 0; i < MAX_EVENT_HDLS; i++ )
            {
                HDL_RESET( &arxEventHdls[i] );
            }
            bIsInitialized = TRUE;
        }
        for( i = 0; i < MAX_EVENT_HDLS; i++ )
        {
            if( IDX_INVALID == arxEventHdls[i].ubIdx )
            {
                xQueueHdl = xQueueCreate( 1, sizeof( xMBPEventType ) );
                if( 0 != xQueueHdl )
                {
                    arxEventHdls[i].ubIdx = i;
                    arxEventHdls[i].xQueueHdl = xQueueHdl;
                    *pxEventHdl = &arxEventHdls[i];
                    eStatus = MB_ENOERR;
                }
                else
                {
                    eStatus = MB_EPORTERR;
                }
                break;
            }
        }
        MBP_EXIT_CRITICAL_SECTION(  );
    }
    return eStatus;
}

eMBErrorCode
eMBPEventPost( const xMBPEventHandle xEventHdl, xMBPEventType xEvent )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xEventInternalHandle *pxEventHdl = xEventHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxEventHdl, arxEventHdls ) )
    {
        if( pdTRUE != xQueueSend( pxEventHdl->xQueueHdl, &xEvent, 0 ) )
        {
            eStatus = MB_EPORTERR;
        }
        else
        {
            eStatus = MB_ENOERR;
        }
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}


BOOL
bMBPEventGet( const xMBPEventHandle xEventHdl, xMBPEventType * pxEvent )
{
    BOOL            bEventInQueue = FALSE;
    xEventInternalHandle *pxEventHdl = xEventHdl;

    if( MB_IS_VALID_HDL( pxEventHdl, arxEventHdls ) )
    {
        if( pdTRUE == xQueueReceive( pxEventHdl->xQueueHdl, pxEvent, 50 / portTICK_RATE_MS ) )
        {
            bEventInQueue = TRUE;
        }
    }

    return bEventInQueue;
}

void
vMBPEventDelete( xMBPEventHandle xEventHdl )
{
    xEventInternalHandle *pxEventHdl = xEventHdl;

    if( MB_IS_VALID_HDL( pxEventHdl, arxEventHdls ) )
    {
        if( NULL != pxEventHdl->xQueueHdl )
        {
            vQueueDelete( pxEventHdl->xQueueHdl );
        }
        MBP_ENTER_CRITICAL_SECTION(  );
        HDL_RESET( pxEventHdl );
        MBP_EXIT_CRITICAL_SECTION(  );
    }
}
