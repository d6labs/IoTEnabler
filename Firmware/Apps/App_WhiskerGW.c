/*-------------------------------------------------------------------------*
 * Includes:
 *-------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <HostApp.h>
#include <system/platform.h>
#include <ATCmdLib/AtCmdLib.h>
#include <sensors/Temperature.h>
#include <sensors/Potentiometer.h>
#include <system/eeprom.h>
#include <drv/Glyph/lcd.h>
#include <mstimer.h>
#include <system/console.h>
#include "Apps.h"
#include "led.h"
#include "NVSettings.h"
#include "mqtt.h"
#include "mqtt_creds.h"
#include "xr20m1170.h"
#include "led.h"

extern char WiFiMAC[];
extern NVSettings_t GNV_Setting;
extern ATLIBGS_TCPMessage rxm;
extern unsigned int G_receivedCount;
mqtt_broker_handle_t broker;
int mqttConnected=0;
static char WifiMAC[17];

typedef struct WhiskerModule
{
  char Name[32];
  unsigned long Mac;
} WhiskerModule;

WhiskerModule moduleList[] = 
{
  {"Sensor1",0xde1a0e50},
  {"LastModule",0x0}
};


void RSSIReading(void)
{
  int16_t rssi;
  char line[20];
  int rssiFound = 0;

  if (AtLibGs_IsNodeAssociated()) {
    if (AtLibGs_GetRssi() == ATLIBGS_MSG_ID_OK) {
      if (AtLibGs_ParseRssiResponse(&rssi)) {
        sprintf(line, "wl_rssi: %d", rssi);
        DisplayLCD(LCD_LINE5, (const uint8_t *)line);
        rssiFound = 1;
      }
    }
  }

  if (!rssiFound) {
    DisplayLCD(LCD_LINE5, "RSSI: ----");
  }
}
/*---------------------------------------------------------------------------*
 * Routine:  WIFI_init
 *---------------------------------------------------------------------------*
 * Description:
 *      Initial setting + DHCP and show status on the LCD.
 *
 * Inputs:
 *      void
 * Outputs:
 *      ATLIBGS_MSG_ID_E
 *---------------------------------------------------------------------------*/
int16_t G_Extra2B=1;
ATLIBGS_MSG_ID_E WIFI_init(int16_t showMessage)
{
  ATLIBGS_MSG_ID_E rxMsgId = ATLIBGS_MSG_ID_NONE;

 // Check the link
#ifdef HOST_APP_DEBUG_ENABLE
    ConsolePrintf("Checking link\r\n");
#endif

  AtLibGs_Init();
  // Wait for the banner
  MSTimerDelay(500);

  /* Send command to check */
  do {
    AtLibGs_FlushIncomingMessage();
    DisplayLCD(LCD_LINE4, "wl_check..  ");
    rxMsgId = AtLibGs_Check();
  } while (ATLIBGS_MSG_ID_OK != rxMsgId);
   
  do {   
       rxMsgId = AtLibGs_SetEcho(0);               // disable Echo
  }while (ATLIBGS_MSG_ID_OK != rxMsgId);
  
  do {                                               
       rxMsgId = AtLibGs_Version();                // check the GS version
    }while (ATLIBGS_MSG_ID_OK != rxMsgId);
#if 0  
  if(strstr((const char *)MRBuffer, "2.3."))       // still debug why receive 2 extra bytes: ESC S
  {
    G_Extra2B = 2;
  }
#endif
  
  do{  
       rxMsgId = AtLibGs_EnableRadio(1);                       // enable radio
  }while(rxMsgId != ATLIBGS_MSG_ID_OK);
  
  /* Get MAC Address & Show */
  rxMsgId = AtLibGs_GetMAC(WiFiMAC);    
  if (rxMsgId == ATLIBGS_MSG_ID_OK)
    AtLibGs_ParseGetMacResponse(WifiMAC);
  if (showMessage > 0) {   
    DisplayLCD(LCD_LINE3, (const uint8_t *)WifiMAC);
    MSTimerDelay(2000);
  }
  
  do {
    AtLibGs_FlushIncomingMessage();
    DisplayLCD(LCD_LINE4, "wl_disass.. ");
    rxMsgId = AtLibGs_DisAssoc();
  } while (ATLIBGS_MSG_ID_OK != rxMsgId);
    
    // Enable DHCP
  do { 
    DisplayLCD(LCD_LINE4, "wl_dhcpon.. ");
    rxMsgId = AtLibGs_DHCPSet(1);
  } while (ATLIBGS_MSG_ID_OK != rxMsgId);
 
  if(strlen(GNV_Setting.webprov.ssid) > 0)
  { 

    if(GNV_Setting.webprov.security == ATLIBGS_PROVSECU_WEP)
    {
        do {
          DisplayLCD(LCD_LINE4, "wl_setwep.. ");
          rxMsgId = AtLibGs_SetWEP1((int8_t*)GNV_Setting.webprov.password);
        } while (ATLIBGS_MSG_ID_OK != rxMsgId); 
        DisplayLCD(LCD_LINE4, "wl_wepset.. ");      
    }
    else if(GNV_Setting.webprov.security == ATLIBGS_PROVSECU_WPA_PER)
    {
        do {
          DisplayLCD(LCD_LINE4, "wl_setpsk.. ");
          rxMsgId = AtLibGs_CalcNStorePSK(GNV_Setting.webprov.ssid, GNV_Setting.webprov.password);
        } while (ATLIBGS_MSG_ID_OK != rxMsgId); 
        DisplayLCD(LCD_LINE4, "wl_pskset.. ");
    }
    else if(GNV_Setting.webprov.security == ATLIBGS_PROVSECU_WPA_ENT)
    {
     /* Set AT+WAUTH=0 for WPA or WPA2  */
         do {
           DisplayLCD(LCD_LINE4, "       " );
           rxMsgId = AtLibGs_SetAuthentictionMode(ATLIBGS_AUTHMODE_NONE_WPA);
         } while (ATLIBGS_MSG_ID_OK != rxMsgId);
       /* Security Configuration */
         do {
           DisplayLCD(LCD_LINE4, "wl_setsec.. ");
           rxMsgId = AtLibGs_SetSecurity(ATLIBGS_SMAUTO);
         } while (ATLIBGS_MSG_ID_OK != rxMsgId);  
     }
   }
   else
   {
#ifdef HOST_APP_SEC_WEP
        // Set AT+WAUTH=2 for WEP
        do {
          DisplayLCD(LCD_LINE4, "wl_wepauth.." );
          rxMsgId = AtLibGs_SetAuthentictionMode(2);
        } while (ATLIBGS_MSG_ID_OK != rxMsgId);
        // Set WEP
        do {
          rxMsgId = AtLibGs_SetWEP1(HOST_APP_AP_SEC_WEP);
        } while (ATLIBGS_MSG_ID_OK != rxMsgId);
          /* Security Configuration */
        do {
          rxMsgId = AtLibGs_SetSecurity(2);        // WEP
          } while (ATLIBGS_MSG_ID_OK != rxMsgId);
#endif
    
#ifdef HOST_APP_SEC_PSK
        /* Store the PSK value. This call takes might take few seconds to return */
        do {
          DisplayLCD(LCD_LINE4, "wl_setpsk.. ");
          rxMsgId = AtLibGs_CalcNStorePSK(HOST_APP_AP_SSID, HOST_APP_AP_SEC_PSK);
        } while (ATLIBGS_MSG_ID_OK != rxMsgId);
#endif

#ifdef HOST_APP_SEC_OPEN
        /* Store the PSK value. This call takes might take few seconds to return */
        do {
          DisplayLCD(LCD_LINE4, "wl_nosec..  " );
          rxMsgId = AtLibGs_SetAuthentictionMode(1);
        } while (ATLIBGS_MSG_ID_OK != rxMsgId);
#endif

#ifdef HOST_APP_WPA
        // Set AT+WAUTH=0 for WPA or WPA2
        do {
          DisplayLCD(LCD_LINE4, "wl_wpa..   " );
          rxMsgId = AtLibGs_SetAuthentictionMode(0);
        } while (ATLIBGS_MSG_ID_OK != rxMsgId);
      
        /* Store the PSK value. This call takes might take few seconds to return */
        do {
          DisplayLCD(LCD_LINE4, "wl_setpsk.. ");
          rxMsgId = AtLibGs_CalcNStorePSK(HOST_APP_AP_SSID, HOST_APP_AP_SEC_PSK);
        } while (ATLIBGS_MSG_ID_OK != rxMsgId);   
      
        /* Security Configuration */
        do {
          DisplayLCD(LCD_LINE4, "wl_wpa..    ");
          rxMsgId = AtLibGs_SetSecurity(4);
        } while (ATLIBGS_MSG_ID_OK != rxMsgId);
#endif

#ifdef HOST_APP_WPA2
        // Set AT+WAUTH=0 for WPA or WPA2
        do {
          DisplayLCD(LCD_LINE4, "wl_wpa2..   " );
          rxMsgId = AtLibGs_SetAuthentictionMode(ATLIBGS_AUTHMODE_NONE_WPA);
        } while (ATLIBGS_MSG_ID_OK != rxMsgId);
          
        /* Store the PSK value. This call takes might take few seconds to return */
        do {
          DisplayLCD(LCD_LINE4, "wl_setpsk.. ");
          rxMsgId = AtLibGs_CalcNStorePSK(HOST_APP_AP_SSID, HOST_APP_AP_SEC_PSK);
        } while (ATLIBGS_MSG_ID_OK != rxMsgId);   

        /* Security Configuration */
        do {
          DisplayLCD(LCD_LINE4, "wl_setwpa..  ");
          rxMsgId = AtLibGs_SetSecurity(ATLIBGS_SMWPA2PSK);
        } while (ATLIBGS_MSG_ID_OK != rxMsgId);
#endif
      }
  
  return rxMsgId;
}
// Connect to the MQTT broker.  Call this function whenever connection needs
// to be re-established.
// returns:  0 - everything OK
//          -1 - packet error
//          -2 - failed to get connack
int App_ConnectMqtt()
{  
    int packet_length;

    mqtt_init(&broker, WIO_CLIENTID);
    mqtt_init_auth(&broker, WIO_USERNAME, WIO_PASSWORD);
    mqtt_connect(&broker);
    // wait for CONNACK	
    packet_length = mqtt_read_packet(6000);

    if(packet_length < 0) 
    {
        DisplayLCD(LCD_LINE4, "MQTT packet error");
        return -1;
    }
    if(MQTTParseMessageType(rxm.message) != MQTT_MSG_CONNACK) 
    { 
        return -2;
    }  
    if(rxm.message[3] != 0x00)
    {
        return -2;
    }
    App_PrepareIncomingData();
    AtLibGs_FlushIncomingMessage();
    mqttConnected=1;
    return 0;
}
/*---------------------------------------------------------------------------*
 * Routine:  WIFI_Associate
 *---------------------------------------------------------------------------*
 * Description:
 *      Association and show result on the LCD
 * Inputs:
 *      void
 * Outputs:
 *      ATLIBGS_MSG_ID_E
 *---------------------------------------------------------------------------*/
ATLIBGS_MSG_ID_E WIFI_Associate(void)
{
    ATLIBGS_MSG_ID_E rxMsgId = ATLIBGS_MSG_ID_NONE;
    static ATLIBGS_AUTHMODE_E WEPMode=ATLIBGS_AUTHMODE_OPEN_WEP;
    int retVal;

    DisplayLCD(LCD_LINE4, "wl_trycon.. ");

    /* Associate to a particular AP specified by SSID  */
    if (strlen(GNV_Setting.webprov.ssid) > 0)
    {
        rxMsgId = AtLibGs_Assoc(GNV_Setting.webprov.ssid,NULL,HOST_APP_AP_CHANNEL);
    }
    else
    {
        rxMsgId = AtLibGs_Assoc(HOST_APP_AP_SSID, NULL, HOST_APP_AP_CHANNEL);
    }
    if (ATLIBGS_MSG_ID_OK != rxMsgId) 
    {
        /* Association error - we can retry */
        if(GNV_Setting.webprov.security == ATLIBGS_PROVSECU_WEP)
        {
            if(WEPMode==ATLIBGS_AUTHMODE_OPEN_WEP)
            {
                rxMsgId = AtLibGs_SetAuthentictionMode(ATLIBGS_AUTHMODE_SHARED_WEP);// Potenially it's a WEP shared AP
                WEPMode=ATLIBGS_AUTHMODE_SHARED_WEP;
            }
            else
            {
                rxMsgId = AtLibGs_SetAuthentictionMode(ATLIBGS_AUTHMODE_OPEN_WEP);// Potenially it's a WEP shared AP
                WEPMode=ATLIBGS_AUTHMODE_OPEN_WEP;         
            }
        }
        DisplayLCD(LCD_LINE4, "Assoc failed...");
        MSTimerDelay(2000);
        DisplayLCD(LCD_LINE4, "Trying again...");
    } 
    else 
    {
        /* Association success */
        AtLibGs_SetNodeAssociationFlag();
        DisplayLCD(LCD_LINE4, "wl_connect  ");
        MSTimerDelay(2000);
        retVal = App_ConnectMqtt();
        switch(retVal)
        {
        case 0:    
            DisplayLCD(LCD_LINE5, "MQTT Connected");
            break;
        case -1:    
            DisplayLCD(LCD_LINE5, "MQTT packet error");
            break;
        case -2:    
            DisplayLCD(LCD_LINE5, "MQTT connack error");
            break;      
        }
    }

    return rxMsgId;
}
WhiskerModule* findModule(unsigned long mac)
{
    int ptr = 0;
    while(moduleList[ptr].Mac!=0x0)
    {
      if(moduleList[ptr].Mac == mac)
        return &moduleList[ptr];
      ptr++;
    }
    return 0;
}
// handler for messages that are published to topics we are subscribed to.
void App_MQTTMsgPublished()
{
}
void App_WhiskerGW()
{
    char pubTopic[100];
    char pubMsg[250];
    unsigned char msgType;
    int counter=0;
    char cntStr[20];
    char commandBuffer[64];
    int commandBufferPointer=0;
    
    led_all_off();
    // Give the unit a little time to start up
    // (300 ms for GS1011 and 1000 ms for GS1500)
    MSTimerDelay(1000);

    NVSettingsLoad(&GNV_Setting);
    led_on(4);
    WIFI_init(1);  // Show MAC address and Version
    led_on(5);
    WIFI_Associate();
    led_on(6);
    DisplayLCD(LCD_LINE8, "Demo starting.");
    DisplayLCD(LCD_LINE3, (const uint8_t *)WifiMAC);
    
    
    
    // UART
    if(spiUartInitialize()!=0)
    {
        DisplayLCD(LCD_LINE7, "!! SPIUART_ERROR !!");
        while(1)
        {
              led_all_on();
              MSTimerDelay(250);
              led_all_off();
              MSTimerDelay(250);
        }
    }
    
    while (1) 
    {
        // Do we need to connect to the AP?
        if (!AtLibGs_IsNodeAssociated())
        {
            led_off(6);
            WIFI_Associate();
            led_on(6);
        }
        else
        {    
            if(mqttConnected==0)
                App_ConnectMqtt();
            int charCount = spiUartRxBytesAvailable();
            while(charCount>0)
            {
                commandBuffer[commandBufferPointer++] = spiUartGetByte();
                charCount--;
                if(charCount==0)
                  commandBufferPointer=0;
                if(commandBufferPointer>4)
                {
                  if(strstr(commandBuffer,"RMPU")==commandBuffer)
                  {
                    if(commandBufferPointer>24)
                    {
                      // process response
                      char macStr[9];
                      char lenStr[5];
                      memcpy(macStr,&commandBuffer[6],8);
                      macStr[8] = 0;
                      memcpy(lenStr,&commandBuffer[4],2);
                      lenStr[2]=0;
                      int len = (int)strtol(lenStr,0,16);
                      unsigned long mac = strtoul(macStr,NULL,16);
                      WhiskerModule *wm = findModule(mac);
                      if(wm!=0)
                      {
                          if(commandBufferPointer>len+16)
                          {
                            char rssiStr[3];
                            memcpy(rssiStr,&commandBuffer[commandBufferPointer-3],2);
                            rssiStr[2]=0;
                            int rssi = (int)strtol(rssiStr,0,16);
                            if((rssi & 0x80) == 0x80)
                              rssi-=256;
                            int puMsgPointer=14;
                            mqtt_initJsonMsg(pubMsg);
                            mqtt_addStringValToMsg("Name",wm->Name,pubMsg,0);
                            mqtt_addStringValToMsg("Mac",macStr,pubMsg,1);
                            char rstr[8];
                            sprintf(rstr,"%d dbm",rssi);
                            mqtt_addStringValToMsg("Rssi",rstr,pubMsg,1);
                            sprintf(pubMsg+strlen(pubMsg),",\"Values\":{");
                            puMsgPointer=14;
                            int comma=0;
                            while(puMsgPointer < (commandBufferPointer-3))
                            {
                              char cidStr[3];
                              memcpy(cidStr,&commandBuffer[puMsgPointer],2);
                              puMsgPointer+=2;
                              cidStr[2]=0;
                              unsigned char cid=(unsigned char)strtol(cidStr,0,16);
                              unsigned char channel = cid & 0x1f;
                              char valStr[9];
                              long valInt;
                              switch(cid)
                              {
                              case 0x21:
                                //digital input
                                memcpy(valStr,&commandBuffer[puMsgPointer],2);
                                valStr[2]=0;
                                puMsgPointer+=2;
                                valInt = (int)strtol(valStr,0,16);
                                if(valInt)
                                    mqtt_addStringValToMsg("DIN1","True",pubMsg,comma);
                                else
                                    mqtt_addStringValToMsg("DIN1","False",pubMsg,comma);
                                break;
                              case 0x22:
                                //digital input
                                memcpy(valStr,&commandBuffer[puMsgPointer],2);
                                valStr[2]=0;
                                puMsgPointer+=2;
                                valInt = (int)strtol(valStr,0,16);
                                if(valInt)
                                    mqtt_addStringValToMsg("DIN2","True",pubMsg,comma);
                                else
                                    mqtt_addStringValToMsg("DIN2","False",pubMsg,comma);
                                break;                              
                              case 0x43:
                                //battery analog in
                                memcpy(valStr,&commandBuffer[puMsgPointer],4);
                                valStr[4]=0;
                                puMsgPointer+=4;                                
                                valInt = (int)strtol(valStr,0,16);
                                mqtt_addIntValToMsg("Battery",valInt,pubMsg,comma);
                                break;          
                              case 0x44:
                                //battery analog in
                                memcpy(valStr,&commandBuffer[puMsgPointer],4);
                                valStr[4]=0;
                                puMsgPointer+=4;                                
                                valInt = (int)strtol(valStr,0,16);
                                mqtt_addIntValToMsg("Temperature",valInt,pubMsg,comma);
                                break;          
                              case 0x45:
                                //battery analog in
                                memcpy(valStr,&commandBuffer[puMsgPointer],4);
                                valStr[4]=0;
                                puMsgPointer+=4;                                
                                valInt = (int)strtol(valStr,0,16);
                                mqtt_addIntValToMsg("RH",valInt,pubMsg,comma);
                                break;  
                              case 0x5d:
                                //internal temperature
                                memcpy(valStr,&commandBuffer[puMsgPointer],4);
                                valStr[4]=0;
                                puMsgPointer+=4;                                
                                valInt = (int)strtol(valStr,0,16);
                                mqtt_addIntValToMsg("IntTemp",valInt,pubMsg,comma);
                                break;
                              case 0x57:                                
                                //air quality
                                memcpy(valStr,&commandBuffer[puMsgPointer],4);
                                valStr[4]=0;
                                puMsgPointer+=4;                                
                                valInt = (int)strtol(valStr,0,16);
                                mqtt_addIntValToMsg("AirQual",valInt,pubMsg,comma);
                                break;
                              case 0x58:
                                //air quality
                                memcpy(valStr,&commandBuffer[puMsgPointer],4);
                                valStr[4]=0;
                                puMsgPointer+=4;                                
                                valInt = (int)strtol(valStr,0,16);
                                mqtt_addIntValToMsg("AirQual",valInt,pubMsg,comma);
                                break;
                                
                              case 0x61:
                                // digital counter input
                                memcpy(valStr,&commandBuffer[puMsgPointer],4);
                                valStr[4]=0;
                                puMsgPointer+=4;                                
                                valInt = (int)strtol(valStr,0,16);
                                mqtt_addIntValToMsg("Count1",valInt,pubMsg,comma);
                                break;
                                
                              case 0x62:
                                // digital counter input
                                memcpy(valStr,&commandBuffer[puMsgPointer],4);
                                valStr[4]=0;
                                puMsgPointer+=4;                                
                                valInt = (int)strtol(valStr,0,16);
                                mqtt_addIntValToMsg("Count2",valInt,pubMsg,comma);
                                break;
                              }                              
                              comma=1;
                            }                            
                            sprintf(pubMsg+strlen(pubMsg),"}");
                            mqtt_finishJsonMsg(pubMsg);
                            sprintf(pubTopic, "%s/%s", WIO_DOMAIN, macStr);
                            int res=mqtt_publish(&broker, pubTopic, pubMsg,0)<0;
                            if(res<0)
                              mqttConnected=0;
                            led_on(8);
                            //spiUartResetFIFO();
                          }
                      }
                    }
                  }
                }
            }
            // Send data if
            RSSIReading();
            AtLibGs_WaitForTCPMessage(1000);
            led_off(7);
            led_off(8);
            if(G_receivedCount>0)
            {
                AtLibGs_ParseTCPData(G_received,G_receivedCount,&rxm);
                msgType = MQTTParseMessageType(rxm.message);
                switch(msgType)
                {
                case MQTT_MSG_SUBACK:
                  // todo: display subscription acknowledgement
                  break;
                case MQTT_MSG_PUBLISH:
                  App_MQTTMsgPublished();
                  break;
                case MQTT_MSG_PUBACK:
                  // todo: display publish acknowledgement
                  break;
                default:
                  break;
                }
            }
            counter++;
            sprintf(cntStr,"Counter=%d",counter);
            DisplayLCD(LCD_LINE6,cntStr);
            if(counter>30)
            {
                counter=0;
                sprintf(pubTopic, "%s/%s", WIO_DOMAIN, WifiMAC);
                mqtt_initJsonMsg(pubMsg);
                mqtt_addStringValToMsg("msg","status ok",pubMsg,0);
                mqtt_finishJsonMsg(pubMsg);
                int res1=mqtt_publish(&broker, pubTopic, pubMsg,0)<0;
                if(res1<0)
                    mqttConnected=0;
                led_on(7);
            }
        }       
    // Send data if END
    }
}