/**************************************************************************************************
  Filename:       SensorApp.c
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

#include "SensorApp.h"
#include "DebugTrace.h"

#if !defined( WIN32 )
  #include "OnBoard.h"
#endif

/* HAL */
#include "hal_led.h"
#include "hal_key.h"
#include "hal_uart.h"
#include "hal_oled.h"
#include "hal_adc.h"
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
const cId_t SensorApp_ClusterList[SENSORAPP_MAX_CLUSTERS] =
{
  SENSORAPP_CLUSTERID,
  SENSORAPP_CLUSTERID_2
};

const SimpleDescriptionFormat_t SensorApp_SimpleDesc =
{
  SENSORAPP_ENDPOINT,              //  int Endpoint;
  SENSORAPP_PROFID,                //  uint16 AppProfId[2];
  SENSORAPP_DEVICEID,              //  uint16 AppDeviceId[2];
  SENSORAPP_DEVICE_VERSION,        //  int   AppDevVer:4;
  SENSORAPP_FLAGS,                 //  int   AppFlags:4;
  SENSORAPP_MAX_CLUSTERS,          //  byte  AppNumInClusters;
  (cId_t *)SensorApp_ClusterList,  //  byte *pAppInClusterList;
  SENSORAPP_MAX_CLUSTERS,          //  byte  AppNumInClusters;
  (cId_t *)SensorApp_ClusterList   //  byte *pAppInClusterList;
};

// This is the Endpoint/Interface description.  It is defined here, but
// filled-in in SensorApp_Init().  Another way to go would be to fill
// in the structure here and make it a "const" (in code space).  The
// way it's defined in this sample app it is define in RAM.
endPointDesc_t SensorApp_epDesc;

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
byte SensorApp_TaskID;   // Task ID for internal task/event processing
                          // This variable will be received when
                          // SensorApp_Init() is called.
devStates_t SensorApp_NwkState;


byte SensorApp_TransID;  // This is the unique message ID (counter)

afAddrType_t SensorApp_DstAddr;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
void SensorApp_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg );
void SensorApp_HandleKeys( byte shift, byte keys );
void SensorApp_MessageMSGCB( afIncomingMSGPacket_t *pckt );
void SensorApp_SendTheMessage( void );

/*********************************************************************
 * NETWORK LAYER CALLBACKS
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      SensorApp_Init
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
void SensorApp_Init( byte task_id )
{
  SensorApp_TaskID = task_id;
  SensorApp_NwkState = DEV_INIT;
  SensorApp_TransID = 0;

  // Device hardware initialization can be added here or in main() (Zmain.c).
  // If the hardware is application specific - add it here.
  // If the hardware is other parts of the device add it in main().

  SensorApp_DstAddr.addrMode = (afAddrMode_t)AddrNotPresent;
  SensorApp_DstAddr.endPoint = SENSORAPP_ENDPOINT;
  SensorApp_DstAddr.addr.shortAddr = 0xFFFE;

  // Fill out the endpoint description.
  SensorApp_epDesc.endPoint = SENSORAPP_ENDPOINT;
  SensorApp_epDesc.task_id = &SensorApp_TaskID;
  SensorApp_epDesc.simpleDesc
            = (SimpleDescriptionFormat_t *)&SensorApp_SimpleDesc;
  SensorApp_epDesc.latencyReq = noLatencyReqs;

  // Register the endpoint description with the AF
  afRegister( &SensorApp_epDesc );

  // Register for all key events - This app will handle all key events
  RegisterForKeys( SensorApp_TaskID );
  
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
  
  APCFG |= 0x01;//��ʼ��ADC ģ��IO
  
  // Update the display

  Hal_Oled_WriteString(HAL_OLED_XSTART,HAL_OLED_LINE_3,"SensorApp",SIZE1 );
    
  ZDO_RegisterForZDOMsg( SensorApp_TaskID, End_Device_Bind_rsp );
  ZDO_RegisterForZDOMsg( SensorApp_TaskID, Match_Desc_rsp );
}

/*********************************************************************
 * @fn      SensorApp_ProcessEvent
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
UINT16 SensorApp_ProcessEvent( byte task_id, UINT16 events )
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
    MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SensorApp_TaskID );
    while ( MSGpkt )
    {
      switch ( MSGpkt->hdr.event )
      {
        case ZDO_CB_MSG:
          SensorApp_ProcessZDOMsgs( (zdoIncomingMsg_t *)MSGpkt );
          break;
          
        case KEY_CHANGE:
          SensorApp_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
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
          SensorApp_MessageMSGCB( MSGpkt );
          break;

        case ZDO_STATE_CHANGE:
          SensorApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
          if ( (SensorApp_NwkState == DEV_ZB_COORD)
              || (SensorApp_NwkState == DEV_ROUTER)
              || (SensorApp_NwkState == DEV_END_DEVICE) )
          {
            HalUARTWrite(0,"success join net\r\n",19);
            // Start sending "the" message in a regular interval.
            osal_start_timerEx( SensorApp_TaskID,
                                SENSORAPP_SEND_MSG_EVT,
                              SENSORAPP_SEND_MSG_TIMEOUT );
          }
          break;

        default:
          break;
      }

      // Release the memory
      osal_msg_deallocate( (uint8 *)MSGpkt );

      // Next
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SensorApp_TaskID );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }

  // Send a message out - This event is generated by a timer
  //  (setup in SensorApp_Init()).
  if ( events & SENSORAPP_SEND_MSG_EVT )
  {
    // Send "the" message
    SensorApp_SendTheMessage();

    // Setup to send message again
    osal_start_timerEx( SensorApp_TaskID,
                        SENSORAPP_SEND_MSG_EVT,
                      SENSORAPP_SEND_MSG_TIMEOUT );

    // return unprocessed events
    return (events ^ SENSORAPP_SEND_MSG_EVT);
  }

  // Discard unknown events
  return 0;
}

/*********************************************************************
 * Event Generation Functions
 */

/*********************************************************************
 * @fn      SensorApp_ProcessZDOMsgs()
 *
 * @brief   Process response messages
 *
 * @param   none
 *
 * @return  none
 */
void SensorApp_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg )
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
            SensorApp_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
            SensorApp_DstAddr.addr.shortAddr = pRsp->nwkAddr;
            // Take the first endpoint, Can be changed to search through endpoints
            SensorApp_DstAddr.endPoint = pRsp->epList[0];

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
 * @fn      SensorApp_HandleKeys
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
void SensorApp_HandleKeys( byte shift, byte keys )
{
  zAddrType_t dstAddr;

    if ( keys & HAL_KEY_SW_1 )
    {
      HalLedSet ( HAL_LED_1, HAL_LED_MODE_OFF );

      // Initiate an End Device Bind Request for the mandatory endpoint
      dstAddr.addrMode = Addr16Bit;
      dstAddr.addr.shortAddr = 0x0000; // Coordinator
      ZDP_EndDeviceBindReq( &dstAddr, NLME_GetShortAddr(), 
                            SensorApp_epDesc.endPoint,
                            SENSORAPP_PROFID,
                            SENSORAPP_MAX_CLUSTERS, (cId_t *)SensorApp_ClusterList,
                            SENSORAPP_MAX_CLUSTERS, (cId_t *)SensorApp_ClusterList,
                            FALSE );
    }
    
    if ( keys & HAL_KEY_SW_2 )
    {
      HalLedSet ( HAL_LED_1, HAL_LED_MODE_OFF );
      // Initiate a Match Description Request (Service Discovery)
      dstAddr.addrMode = AddrBroadcast;
      dstAddr.addr.shortAddr = NWK_BROADCAST_SHORTADDR;
      ZDP_MatchDescReq( &dstAddr, NWK_BROADCAST_SHORTADDR,
                        SENSORAPP_PROFID,
                        SENSORAPP_MAX_CLUSTERS, (cId_t *)SensorApp_ClusterList,
                        SENSORAPP_MAX_CLUSTERS, (cId_t *)SensorApp_ClusterList,
                        FALSE );
    }
  
}

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * @fn      SensorApp_MessageMSGCB
 *
 * @brief   Data message processor callback.  This function processes
 *          any incoming data - probably from other devices.  So, based
 *          on cluster ID, perform the intended action.
 *
 * @param   none
 *
 * @return  none
 */
void SensorApp_MessageMSGCB( afIncomingMSGPacket_t *pkt )
{
  HalUARTWrite(0,pkt->cmd.Data,pkt->cmd.DataLength);
  Hal_Oled_WriteString(HAL_OLED_XSTART,HAL_OLED_LINE_6,pkt->cmd.Data,SIZE1);
  switch ( pkt->clusterId )
  {
    case SENSORAPP_CLUSTERID:
     
      // "the" message
      
      break;
  }
}

/*********************************************************************
 * @fn      SensorApp_SendTheMessage
 *
 * @brief   Send "the" message.
 *
 * @param   none
 *
 * @return  none
 */
void SensorApp_SendTheMessage( void )
{
  char light = 0;
  
  static char level = 0;
  static char str_uart[]="light:00 level:0";
  char theMessageData[2] = {0};
  
  light = HalAdcRead ( HAL_ADC_CHANNEL_0, HAL_ADC_RESOLUTION_10 );
  
  light = (unsigned char)(light*100/255)-10;
  if((light/10) != level)
  {
    level = light/10;
    theMessageData[1]=light;
    theMessageData[0]=level;
    
    if ( AF_DataRequest( &SensorApp_DstAddr, &SensorApp_epDesc,
                       SENSORAPP_CLUSTERID,
                       (byte)osal_strlen( theMessageData ) + 1,
                       (byte *)&theMessageData,
                       &SensorApp_TransID,
                       AF_DISCV_ROUTE, AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
    {
      // Successfully requested to be sent.
      str_uart[6]=light/10+'0';
      str_uart[7]=light%10+'0';
      str_uart[15]=level+'0';
      HalUARTWrite(0,str_uart,osal_strlen( str_uart ));
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
  else
  {
    theMessageData[0]='L';
    theMessageData[1]=light;
    
    if ( AF_DataRequest( &SensorApp_DstAddr, &SensorApp_epDesc,
                       SENSORAPP_CLUSTERID_2,
                       (byte)osal_strlen( theMessageData ) + 1,
                       (byte *)&theMessageData,
                       &SensorApp_TransID,
                       AF_DISCV_ROUTE, AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
    {
      // Successfully requested to be sent.
      str_uart[6]=light/10+'0';
      str_uart[7]=light%10+'0';
      str_uart[15]=level+'0';
      HalUARTWrite(0,str_uart,osal_strlen( str_uart ));
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
}

/*********************************************************************
*********************************************************************/
