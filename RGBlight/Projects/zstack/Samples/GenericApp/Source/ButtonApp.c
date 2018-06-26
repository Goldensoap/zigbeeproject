/**************************************************************************************************
  Filename:       ButtonApp.c
  Revised:        $Date: 2009-03-18 15:56:27 -0700 (Wed, 18 Mar 2009) $
  Revision:       $Revision: 19453 $

  Description:    Generic Application (no Profile).


  Copyright 2004-2009 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED �AS IS?WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, 
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE, 
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com. 
**************************************************************************************************/

/*********************************************************************
  This application isn't intended to do anything useful, it is
  intended to be a simple example of an application's structure.

  This application sends "Hello World" to another "Generic"
  application every 15 seconds.  The application will also
  receive "Hello World" packets.

  The "Hello World" messages are sent/received as MSG type message.

  This applications doesn't have a profile, so it handles everything
  directly - itself.

  Key control:
    SW1:
    SW2:  initiates end device binding
    SW3:
    SW4:  initiates a match description request
*********************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "OSAL.h"
#include "AF.h"
#include "ZDApp.h"
#include "ZDObject.h"
#include "ZDProfile.h"
#include "OnBoard.h"

#include "ButtonApp.h"
#include "DebugTrace.h"

#if !defined( WIN32 )
  #include "OnBoard.h"
#endif

/* HAL */
#include "hal_led.h"
#include "hal_key.h"
#include "hal_uart.h"
#include "hal_oled.h"
/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

// This list should be filled with Application specific Cluster IDs.
const cId_t ButtonApp_ClusterList[BUTTONAPP_MAX_CLUSTERS] =
{
  BUTTONAPP_CLUSTERID
};

const SimpleDescriptionFormat_t ButtonApp_SimpleDesc =
{
  BUTTONAPP_ENDPOINT,              //  int Endpoint;
  BUTTONAPP_PROFID,                //  uint16 AppProfId[2];
  BUTTONAPP_DEVICEID,              //  uint16 AppDeviceId[2];
  BUTTONAPP_DEVICE_VERSION,        //  int   AppDevVer:4;
  BUTTONAPP_FLAGS,                 //  int   AppFlags:4;
  BUTTONAPP_MAX_CLUSTERS,          //  byte  AppNumInClusters;
  (cId_t *)ButtonApp_ClusterList,  //  byte *pAppInClusterList;
  BUTTONAPP_MAX_CLUSTERS,          //  byte  AppNumInClusters;
  (cId_t *)ButtonApp_ClusterList   //  byte *pAppInClusterList;
};

// This is the Endpoint/Interface description.  It is defined here, but
// filled-in in ButtonApp_Init().  Another way to go would be to fill
// in the structure here and make it a "const" (in code space).  The
// way it's defined in this sample app it is define in RAM.
endPointDesc_t ButtonApp_epDesc;

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
byte ButtonApp_TaskID;   // Task ID for internal task/event processing
                          // This variable will be received when
                          // ButtonApp_Init() is called.
devStates_t ButtonApp_NwkState;


byte ButtonApp_TransID;  // This is the unique message ID (counter)

afAddrType_t ButtonApp_DstAddr;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
void ButtonApp_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg );
void ButtonApp_HandleKeys( byte shift, byte keys );
void ButtonApp_MessageMSGCB( afIncomingMSGPacket_t *pckt );
void ButtonApp_SendTheMessage( void );

/*********************************************************************
 * NETWORK LAYER CALLBACKS
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      ButtonApp_Init
 *
 * @brief   Initialization function for the Generic App Task.
 *          This is called during initialization and should contain
 *          any application specific initialization (ie. hardware
 *          initialization/setup, table initialization, power up
 *          notificaiton ... ).
 *
 * @param   task_id - the ID assigned by OSAL.  This ID should be
 *                    used to send messages and set timers.
 *
 * @return  none
 */
void ButtonApp_Init( byte task_id )
{
  ButtonApp_TaskID = task_id;
  ButtonApp_NwkState = DEV_INIT;
  ButtonApp_TransID = 0;

  // Device hardware initialization can be added here or in main() (Zmain.c).
  // If the hardware is application specific - add it here.
  // If the hardware is other parts of the device add it in main().

  ButtonApp_DstAddr.addrMode = (afAddrMode_t)AddrNotPresent;
  ButtonApp_DstAddr.endPoint = BUTTONAPP_ENDPOINT;
  ButtonApp_DstAddr.addr.shortAddr = 0xFFFE;

  // Fill out the endpoint description.
  ButtonApp_epDesc.endPoint = BUTTONAPP_ENDPOINT;
  ButtonApp_epDesc.task_id = &ButtonApp_TaskID;
  ButtonApp_epDesc.simpleDesc
            = (SimpleDescriptionFormat_t *)&ButtonApp_SimpleDesc;
  ButtonApp_epDesc.latencyReq = noLatencyReqs;

  // Register the endpoint description with the AF
  afRegister( &ButtonApp_epDesc );

  // Register for all key events - This app will handle all key events
  RegisterForKeys( ButtonApp_TaskID );
  
  halUARTCfg_t uartConfig;
  uartConfig.configured           = TRUE;              // 2x30 don't care - see uart driver.
  uartConfig.baudRate             = HAL_UART_BR_115200;
  uartConfig.flowControl          = FALSE;
  uartConfig.flowControlThreshold = 64;   // 2x30 don't care - see uart driver.
  uartConfig.rx.maxBufSize        = 128;  // 2x30 don't care - see uart driver.
  uartConfig.tx.maxBufSize        = 128;  // 2x30 don't care - see uart driver.
  uartConfig.idleTimeout          = 6;    // 2x30 don't care - see uart driver.
  uartConfig.intEnable            = TRUE; // 2x30 don't care - see uart driver.
  //uartConfig.callBackFunc =rxCB;
  HalUARTOpen(0,&uartConfig);
  MicroWait(50000);
  HalUARTWrite(0,"system start\r\n",14);
  
  // Update the display

  Hal_Oled_WriteString(HAL_OLED_XSTART,HAL_OLED_LINE_1,"ButtonApp",SIZE1 );
    
  ZDO_RegisterForZDOMsg( ButtonApp_TaskID, End_Device_Bind_rsp );
  ZDO_RegisterForZDOMsg( ButtonApp_TaskID, Match_Desc_rsp );
}

/*********************************************************************
 * @fn      ButtonApp_ProcessEvent
 *
 * @brief   Generic Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The OSAL assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  none
 */
UINT16 ButtonApp_ProcessEvent( byte task_id, UINT16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
  afDataConfirm_t *afDataConfirm;

  // Data Confirmation message fields
  byte sentEP;
  ZStatus_t sentStatus;
  byte sentTransID;       // This should match the value sent
  (void)task_id;  // Intentionally unreferenced parameter

  if ( events & SYS_EVENT_MSG )
  {
    MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( ButtonApp_TaskID );
    while ( MSGpkt )
    {
      switch ( MSGpkt->hdr.event )
      {
        case ZDO_CB_MSG:
          ButtonApp_ProcessZDOMsgs( (zdoIncomingMsg_t *)MSGpkt );
          break;
          
        case KEY_CHANGE:
          ButtonApp_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
          break;

        case AF_DATA_CONFIRM_CMD:
          // This message is received as a confirmation of a data packet sent.
          // The status is of ZStatus_t type [defined in ZComDef.h]
          // The message fields are defined in AF.h
          afDataConfirm = (afDataConfirm_t *)MSGpkt;
          sentEP = afDataConfirm->endpoint;
          sentStatus = afDataConfirm->hdr.status;
          sentTransID = afDataConfirm->transID;
          (void)sentEP;
          (void)sentTransID;

          // Action taken when confirmation is received.
          if ( sentStatus != ZSuccess )
          {
            // The data wasn't delivered -- Do something
          }
          break;

        case AF_INCOMING_MSG_CMD:
          ButtonApp_MessageMSGCB( MSGpkt );
          break;

        case ZDO_STATE_CHANGE:
          ButtonApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
          if ( (ButtonApp_NwkState == DEV_ZB_COORD)
              || (ButtonApp_NwkState == DEV_ROUTER)
              || (ButtonApp_NwkState == DEV_END_DEVICE) )
          {
            HalUARTWrite(0,"success join net\r\n",19);
          }
          break;

        default:
          break;
      }

      // Release the memory
      osal_msg_deallocate( (uint8 *)MSGpkt );

      // Next
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( ButtonApp_TaskID );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }

  // Discard unknown events
  return 0;
}

/*********************************************************************
 * Event Generation Functions
 */

/*********************************************************************
 * @fn      ButtonApp_ProcessZDOMsgs()
 *
 * @brief   Process response messages
 *
 * @param   none
 *
 * @return  none
 */
void ButtonApp_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg )
{
  switch ( inMsg->clusterID )
  {
    case End_Device_Bind_rsp:
      if ( ZDO_ParseBindRsp( inMsg ) == ZSuccess )
      {
        // Light LED
        HalLedSet( HAL_LED_1, HAL_LED_MODE_ON );
      }
#if defined(BLINK_LEDS)
      else
      {
        // Flash LED to show failure
        HalLedSet ( HAL_LED_1, HAL_LED_MODE_FLASH );
      }
#endif
      break;

    case Match_Desc_rsp:
      {
        ZDO_ActiveEndpointRsp_t *pRsp = ZDO_ParseEPListRsp( inMsg );
        if ( pRsp )
        {
          if ( pRsp->status == ZSuccess && pRsp->cnt )
          {
            ButtonApp_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
            ButtonApp_DstAddr.addr.shortAddr = pRsp->nwkAddr;
            // Take the first endpoint, Can be changed to search through endpoints
            ButtonApp_DstAddr.endPoint = pRsp->epList[0];

            // Light LED
            HalLedSet( HAL_LED_1, HAL_LED_MODE_ON );
          }
          osal_mem_free( pRsp );
        }
      }
      break;
  }
}

/*********************************************************************
 * @fn      ButtonApp_HandleKeys
 *
 * @brief   Handles all key events for this device.
 *
 * @param   shift - true if in shift/alt.
 * @param   keys - bit field for key events. Valid entries:
 *                 HAL_KEY_SW_4
 *                 HAL_KEY_SW_3
 *                 HAL_KEY_SW_2
 *                 HAL_KEY_SW_1
 *
 * @return  none
 */
void ButtonApp_HandleKeys( byte shift, byte keys )
{
  zAddrType_t dstAddr;

    if ( keys & HAL_KEY_SW_1 )
    {
      HalLedSet ( HAL_LED_1, HAL_LED_MODE_OFF );

      // Initiate an End Device Bind Request for the mandatory endpoint
      dstAddr.addrMode = Addr16Bit;
      dstAddr.addr.shortAddr = 0x0000; // Coordinator
      ZDP_EndDeviceBindReq( &dstAddr, NLME_GetShortAddr(), 
                            ButtonApp_epDesc.endPoint,
                            BUTTONAPP_PROFID,
                            BUTTONAPP_MAX_CLUSTERS, (cId_t *)ButtonApp_ClusterList,
                            BUTTONAPP_MAX_CLUSTERS, (cId_t *)ButtonApp_ClusterList,
                            FALSE );
    }
    
    if ( keys & HAL_KEY_SW_2 )
    {
      ButtonApp_SendTheMessage();
    }
  
}

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * @fn      ButtonApp_MessageMSGCB
 *
 * @brief   Data message processor callback.  This function processes
 *          any incoming data - probably from other devices.  So, based
 *          on cluster ID, perform the intended action.
 *
 * @param   none
 *
 * @return  none
 */
void ButtonApp_MessageMSGCB( afIncomingMSGPacket_t *pkt )
{
  HalUARTWrite(0,pkt->cmd.Data,pkt->cmd.DataLength);
  Hal_Oled_WriteString(HAL_OLED_XSTART,HAL_OLED_LINE_6,pkt->cmd.Data,SIZE1);
  switch ( pkt->clusterId )
  {
    case BUTTONAPP_CLUSTERID:
     
      // "the" message
#if defined( WIN32 )
      WPRINTSTR( pkt->cmd.Data );
#endif
      break;
  }
}

/*********************************************************************
 * @fn      ButtonApp_SendTheMessage
 *
 * @brief   Send "the" message.
 *
 * @param   none
 *
 * @return  none
 */
void ButtonApp_SendTheMessage( void )
{
  static unsigned char LedState;
  byte str_uart[5]={0};
  LedState = !LedState;
  sprintf(str_uart, "%d", LedState);

  if ( AF_DataRequest( &ButtonApp_DstAddr, &ButtonApp_epDesc,
                       BUTTONAPP_CLUSTERID,
                       (byte)osal_strlen( str_uart ) + 1,
                       (byte *)&str_uart,
                       &ButtonApp_TransID,
                       AF_DISCV_ROUTE, AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
  {
    // Successfully requested to be sent.
    HalUARTWrite(0,"success send messages",21);
    //OLED_Clear();
    Hal_Oled_WriteString(HAL_OLED_XSTART,HAL_OLED_LINE_5,str_uart,SIZE1 );
  }
  else
  {
    HalUARTWrite(0,"send failed",11);
    //OLED_Clear();
    Hal_Oled_WriteString(HAL_OLED_XSTART,HAL_OLED_LINE_5,"send failed",SIZE1 );
    // Error occurred in request to send.
  }
}

/*********************************************************************
*********************************************************************/
