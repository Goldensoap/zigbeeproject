/**************************************************************************************************
  Filename:       LightApp.c
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
  PROVIDED “AS IS?WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, 
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

#include "LightApp.h"
#include "DebugTrace.h"

#if !defined( WIN32 )
  #include "OnBoard.h"
#endif

/* HAL */
#include "hal_led.h"
#include "hal_key.h"
#include "hal_uart.h"
#include "hal_oled.h"
#include "hal_timer.h"
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
const cId_t LightApp_ClusterList[LIGHTAPP_MAX_CLUSTERS] =
{
  GENERICAPP_CLUSTERID,
  BUTTONAPP_CLUSTERID,
  LIGHTAPP_CLUSTERID,
  SENSORAPP_CLUSTERID
};

const SimpleDescriptionFormat_t LightApp_SimpleDesc =
{
  LIGHTAPP_ENDPOINT,              //  int Endpoint;
  LIGHTAPP_PROFID,                //  uint16 AppProfId[2];
  LIGHTAPP_DEVICEID,              //  uint16 AppDeviceId[2];
  LIGHTAPP_DEVICE_VERSION,        //  int   AppDevVer:4;
  LIGHTAPP_FLAGS,                 //  int   AppFlags:4;
  LIGHTAPP_MAX_CLUSTERS,          //  byte  AppNumInClusters;
  (cId_t *)LightApp_ClusterList,//  byte *pAppInClusterList;
  LIGHTAPP_MAX_CLUSTERS,          //  byte  AppNumInClusters;
  (cId_t *)LightApp_ClusterList   //  byte *pAppInClusterList;
};

// This is the Endpoint/Interface description.  It is defined here, but
// filled-in in LightApp_Init().  Another way to go would be to fill
// in the structure here and make it a "const" (in code space).  The
// way it's defined in this sample app it is define in RAM.
endPointDesc_t LightApp_epDesc;

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
byte LightApp_TaskID;   // Task ID for internal task/event processing
                          // This variable will be received when
                          // LightApp_Init() is called.
devStates_t LightApp_NwkState;


byte LightApp_TransID;  // This is the unique message ID (counter)

afAddrType_t LightApp_DstAddr;

static byte RxBuf[80+1];
static uint8 SerialApp_TxLen;
static uint8 R=0,G=0,Blue=0,W=0;
static uint8 Rbk=0,Gbk=0,Bluebk=0,Wbk=0;
static unsigned char lightlevel[10]={0x00,0x1E,0x37,0x50,0x69,0x82,0x9B,0xB4,0xCD,0xE6};

/*********************************************************************
 * LOCAL FUNCTIONS
 */
void LightApp_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg );
void LightApp_HandleKeys( byte shift, byte keys );
void LightApp_MessageMSGCB( afIncomingMSGPacket_t *pckt );
void LightApp_SendTheMessage( void );

void Parse_RGBW(uint8* RGBW, uint8 len);
void RGB_change(void);
void RGB_PWMset(void);
/*********************************************************************
 * NETWORK LAYER CALLBACKS
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      LightApp_Init
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
void LightApp_Init( byte task_id )
{
  LightApp_TaskID = task_id;
  LightApp_NwkState = DEV_INIT;
  LightApp_TransID = 0;

  // Device hardware initialization can be added here or in main() (Zmain.c).
  // If the hardware is application specific - add it here.
  // If the hardware is other parts of the device add it in main().

  LightApp_DstAddr.addrMode = (afAddrMode_t)AddrNotPresent;
  LightApp_DstAddr.endPoint = LIGHTAPP_ENDPOINT;
  LightApp_DstAddr.addr.shortAddr = 0xFFFE;

  // Fill out the endpoint description.
  LightApp_epDesc.endPoint = LIGHTAPP_ENDPOINT;
  LightApp_epDesc.task_id = &LightApp_TaskID;
  LightApp_epDesc.simpleDesc
            = (SimpleDescriptionFormat_t *)&LightApp_SimpleDesc;
  LightApp_epDesc.latencyReq = noLatencyReqs;

  // Register the endpoint description with the AF
  afRegister( &LightApp_epDesc );

  // Register for all key events - This app will handle all key events
  RegisterForKeys( LightApp_TaskID );
  
  
  // Update the display

  Hal_Oled_WriteString(HAL_OLED_XSTART,HAL_OLED_LINE_3,"LightApp",SIZE1 );
    
  ZDO_RegisterForZDOMsg( LightApp_TaskID, End_Device_Bind_rsp );
  ZDO_RegisterForZDOMsg( LightApp_TaskID, Match_Desc_rsp );
}

/*********************************************************************
 * @fn      LightApp_ProcessEvent
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
UINT16 LightApp_ProcessEvent( byte task_id, UINT16 events )
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
    MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( LightApp_TaskID );
    while ( MSGpkt )
    {
      switch ( MSGpkt->hdr.event )
      {
        case ZDO_CB_MSG:
          LightApp_ProcessZDOMsgs( (zdoIncomingMsg_t *)MSGpkt );
          break;
          
        case KEY_CHANGE:
          LightApp_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
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
          LightApp_MessageMSGCB( MSGpkt );
          break;

        case ZDO_STATE_CHANGE:
          LightApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
          if ( (LightApp_NwkState == DEV_ZB_COORD)
              || (LightApp_NwkState == DEV_ROUTER)
              || (LightApp_NwkState == DEV_END_DEVICE) )
          {
            //HalUARTWrite(0,"success join net\r\n",19);
            Hal_Oled_WriteString(HAL_OLED_XSTART,HAL_OLED_LINE_4,"success join net",SIZE1 );
            // Start sending "the" message in a regular interval.
          }
          break;

        default:
          break;
      }

      // Release the memory
      osal_msg_deallocate( (uint8 *)MSGpkt );

      // Next
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( LightApp_TaskID );
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
 * @fn      LightApp_ProcessZDOMsgs()
 *
 * @brief   Process response messages
 *
 * @param   none
 *
 * @return  none
 */
void LightApp_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg )
{
  switch ( inMsg->clusterID )
  {
    case End_Device_Bind_rsp:
      if ( ZDO_ParseBindRsp( inMsg ) == ZSuccess )
      {
        // Light LED
        //HalLedSet( HAL_LED_1, HAL_LED_MODE_ON );
        Hal_Oled_WriteString(HAL_OLED_XSTART,HAL_OLED_LINE_4,"bind success",SIZE1 );
      }
#if defined(BLINK_LEDS)
      else
      {
        // Flash LED to show failure
        //HalLedSet ( HAL_LED_1, HAL_LED_MODE_FLASH );
        Hal_Oled_WriteString(HAL_OLED_XSTART,HAL_OLED_LINE_4,"bind failure",SIZE1 );
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
            LightApp_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
            LightApp_DstAddr.addr.shortAddr = pRsp->nwkAddr;
            // Take the first endpoint, Can be changed to search through endpoints
            LightApp_DstAddr.endPoint = pRsp->epList[0];

            // Light LED
            //HalLedSet( HAL_LED_1, HAL_LED_MODE_ON );
            Hal_Oled_WriteString(HAL_OLED_XSTART,HAL_OLED_LINE_4,"bind success",SIZE1 );
          }
          osal_mem_free( pRsp );
        }
      }
      break;
  }
}

/*********************************************************************
 * @fn      LightApp_HandleKeys
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
void LightApp_HandleKeys( byte shift, byte keys )
{
  zAddrType_t dstAddr;

    if ( keys & HAL_KEY_SW_1 )
    {
      //HalLedSet ( HAL_LED_1, HAL_LED_MODE_OFF );
      Hal_Oled_WriteString(HAL_OLED_XSTART,HAL_OLED_LINE_4,"bind waitting",SIZE1 );
      // Initiate an End Device Bind Request for the mandatory endpoint
      dstAddr.addrMode = Addr16Bit;
      dstAddr.addr.shortAddr = 0x0000; // Coordinator
      ZDP_EndDeviceBindReq( &dstAddr, NLME_GetShortAddr(), 
                            LightApp_epDesc.endPoint,
                            LIGHTAPP_PROFID,
                            LIGHTAPP_MAX_CLUSTERS, (cId_t *)LightApp_ClusterList,
                            LIGHTAPP_MAX_CLUSTERS, (cId_t *)LightApp_ClusterList,
                            FALSE );
    }
    
    if ( keys & HAL_KEY_SW_2 )
    {
      //HalLedSet ( HAL_LED_1, HAL_LED_MODE_OFF );
      Hal_Oled_WriteString(HAL_OLED_XSTART,HAL_OLED_LINE_4,"bind waitting",SIZE1 );
      // Initiate a Match Description Request (Service Discovery)
      dstAddr.addrMode = AddrBroadcast;
      dstAddr.addr.shortAddr = NWK_BROADCAST_SHORTADDR;
      ZDP_MatchDescReq( &dstAddr, NWK_BROADCAST_SHORTADDR,
                        LIGHTAPP_PROFID,
                        LIGHTAPP_MAX_CLUSTERS, (cId_t *)LightApp_ClusterList,
                        LIGHTAPP_MAX_CLUSTERS, (cId_t *)LightApp_ClusterList,
                        FALSE );
    }
}

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * @fn      LightApp_MessageMSGCB
 *
 * @brief   Data message processor callback.  This function processes
 *          any incoming data - probably from other devices.  So, based
 *          on cluster ID, perform the intended action.
 *
 * @param   none
 *
 * @return  none
 */
void LightApp_MessageMSGCB( afIncomingMSGPacket_t *pkt )
{
  unsigned char deviceID = '1';

  //Hal_Oled_WriteString(HAL_OLED_XSTART,HAL_OLED_LINE_6,pkt->cmd.Data,SIZE1);
  osal_memset(RxBuf, 0, 80+1);
  SerialApp_TxLen=pkt->cmd.DataLength;
  osal_memcpy(RxBuf, pkt->cmd.Data, SerialApp_TxLen);
  
  switch ( pkt->clusterId )
  {
    case GENERICAPP_CLUSTERID: 
      // "the" message
        if(deviceID == RxBuf[0])
        {
          Parse_RGBW(RxBuf, SerialApp_TxLen);
      
          Wbk=W;
          Rbk=R;
          Gbk=G;
          Bluebk=Blue;
      
          RGB_change();
          RGB_PWMset();
        } 
      break;
    
    case SENSORAPP_CLUSTERID:
    
      W = lightlevel[RxBuf[0]];
      
      R=Rbk;
      G=Gbk;
      Blue=Bluebk;
      
      RGB_change();
      RGB_PWMset();
      
      Wbk=W;
      
      break;
      
    case BUTTONAPP_CLUSTERID:
      if(RxBuf[0]=='1')
      {
        W=Wbk;
        R=Rbk;
        G=Gbk;
        Blue=Bluebk;
        if(W==0 && R==0 && G==0 && Blue==0)
        {
          R=255;
          G=255;
          Blue=255;
          W=180;
        }
        RGB_change();
        RGB_PWMset();
      }
      else if(RxBuf[0]=='0')
      {
        W=255;
        G=255;
        Blue=255;
        R=255;
        RGB_PWMset();
      }
        break;
     default:break;
   }
}

/*********************************************************************
 * @fn      LightApp_SendTheMessage
 *
 * @brief   Send "the" message.
 *
 * @param   none
 *
 * @return  none
 */
void LightApp_SendTheMessage( void )
{
  char theMessageData[] = "Hello World";

  if ( AF_DataRequest( &LightApp_DstAddr, &LightApp_epDesc,
                       LIGHTAPP_CLUSTERID,
                       (byte)osal_strlen( theMessageData ) + 1,
                       (byte *)&theMessageData,
                       &LightApp_TransID,
                       AF_DISCV_ROUTE, AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
  {
    // Successfully requested to be sent.

    //OLED_Clear();
    Hal_Oled_WriteString(HAL_OLED_XSTART,HAL_OLED_LINE_5,"send success",SIZE1 );
  }
  else
  {

    //OLED_Clear();
    Hal_Oled_WriteString(HAL_OLED_XSTART,HAL_OLED_LINE_5,"send failed",SIZE1 );
    // Error occurred in request to send.
  }
}

/*********************************************************************
*********************************************************************/

/*********************************************************************
 * @fn      Parse_RGBW
 *
 * @brief   parse data from homeassistant.
 *
 * @param   Rxbuf lenght
 *
 * @return  none
 */

void Parse_RGBW(uint8* RGBW, uint8 len)
{
  uint8 i=0,m=19,j=0,flag=0;
  uint8 Data[20]={0};
  R=0;G=0;Blue=0;W=0;
  
  for(i=0; i < len; i++)
  {
    if(47<RGBW[i] && RGBW[i]<58)
    {
      Data[j]=RGBW[i] - '0';
      j++;
      if (47>RGBW[i+1] || RGBW[i+1]>58)
      {
        Data[j]=10;
        j++;
      }
    }
  }
  
  for(m=19; m>0; m--)
  {
    j = 1;
    if(Data[m]==10)
    {
      flag++;
    }
    
    if(flag == 1)
      {
        while(Data[m-j] != 10)
        {
          if(j==1)Blue += Data[m-j];
          if(j==2)Blue += (Data[m-j]*10);
          if(j==3)Blue += (Data[m-j]*100);
          j++;
        }
        flag++;
        j = 1;
      }
     if(flag == 3)
      {
        while(Data[m-j] != 10)
        {
          if(j==1)G += Data[m-j];
          if(j==2)G += (Data[m-j]*10);
          if(j==3)G += (Data[m-j]*100);
          j++;
        }
        flag++;
        j = 1;
      }
     if(flag == 5)
      {
        while(Data[m-j] != 10)
        {
          
          if(j==1)R += Data[m-j];
          if(j==2)R += (Data[m-j]*10);
          if(j==3)R += (Data[m-j]*100);
          j++;
        }
        flag++;
        j = 1;
      }
     if(flag == 7)
      {
        while(Data[m-j] != 10)
        {
          if(j==1)W += Data[m-j];
          if(j==2)W += (Data[m-j]*10);
          if(j==3)W += (Data[m-j]*100);
          j++;
        }
        flag++;
      //  j = 1;
        break;
      }
     }
}

void RGB_change(void)
{
      
    R = (uint8)(W/(255.0)*R);
    G = (uint8)(W/(255.0)*G);
    Blue = (uint8)(W/(255.0)*Blue);
  
    if(R>245)R=245;
    if(G>245)G=245;
    if(Blue>245)Blue=245;
    if(W>245)W=245;
    
    if(R == 0)R=255;
    if(G == 0)G=255;
    if(Blue == 0)Blue=255;
    if(W == 0)W=255;
  
    if(R<8)R=8;
    if(G<8)G=8;
    if(Blue<8)Blue=8;
    if(W<8)W=8;

}

void RGB_PWMset(void)
{
    static char show[]="R:000 G:000 B:000 W:000";
    
    show[2]='0';
    show[3]='0';
    show[4]='0';
    
    show[8]= '0';
    show[9]= '0';
    show[10]= '0';
    
    show[14]= '0';
    show[15]= '0';
    show[16]= '0';
    
    show[20]= '0';
    show[21]= '0';
    show[22]= '0';
    
    
    T1CC1H = 0x00;
    T1CC1L = 255-W;
    
    T1CC2H = 0x00;
    T1CC2L = 255-G;
    
    T1CC3H = 0x00;
    T1CC3L = 255-Blue;
    
    T1CC4H = 0x00;
    T1CC4L = 255-R;
    

    show[2]=  R/100+'0';
    show[3]= (R/10)%10+'0';
    show[4]= R%10+'0';
    
    show[8]= G/100+'0';
    show[9]= (G/10)%10+'0';
    show[10]= G%10+'0';
    
    show[14]= Blue/100+'0';
    show[15]= (Blue/10)%10+'0';
    show[16]= Blue%10+'0';
    
    show[20]= W/100+'0';
    show[21]= (W/10)%10+'0';
    show[22]= W%10+'0';
    
    Hal_Oled_WriteString(HAL_OLED_XSTART,HAL_OLED_LINE_6,show,SIZE1 );
}