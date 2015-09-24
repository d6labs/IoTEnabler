/*-------------------------------------------------------------------------*
 * File:  main.c
 *-------------------------------------------------------------------------*
 * Description:
 *     Sets up one of the hardware and drivers and then calls one of the
 *     tests or demos.
 *-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*
 * Includes:
 *-------------------------------------------------------------------------*/
#include <system/platform.h>
#include <HostApp.h>
#include <init/hwsetup.h>
#include <drv\Glyph\lcd.h>
#include <mstimer.h>
#include <system/Switch.h>
#include <sensors\Potentiometer.h>
#include <sensors\Temperature.h>
//#include <Tests\Tests.h>
#include <system\console.h>
#include <drv\UART.h>
#include <Sensors\LightSensor.h>
#include <drv\SPI.h>
#include <GainSpan_SPI.h>
#include <NVSettings.h>
#include <Apps/Apps.h>
#include <Apps/App_Swarm.h>
#include "stdio.h"
#include "string.h"
#include "led.h"
#include "system/Switch.h"
#include <includes.h>                                           /* Micrium firmware.                                    */

extern void LEDFlash(uint32_t timeout);
extern void led_task(void);
extern void DisplayLCD(uint8_t, const uint8_t *);
extern int16_t *Accelerometer_Get(void);
extern void Accelerometer_Init(void);
extern int16_t	gAccData[3];
extern void App_ProbeDemo(void);
/*-------------------------------------------------------------------------*
 * Macros:
 *-------------------------------------------------------------------------*/
/* Set option bytes */
#pragma location = "OPTBYTE"
__root const uint8_t opbyte0 = 0xEFU;
#pragma location = "OPTBYTE"
__root const uint8_t opbyte1 = 0xFFU;
#pragma location = "OPTBYTE"
__root const uint8_t opbyte2 = 0xF8U;
#pragma location = "OPTBYTE"
__root const uint8_t opbyte3 = 0x04U;

/* Set security ID */
#pragma location = "SECUID"
__root const uint8_t secuid[10] = 
    {0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};

/*-------------------------------------------------------------------------*
 * Types:
 *-------------------------------------------------------------------------*/
/* Application Modes */
typedef enum {
    GAINSPAN_DEMO=0,      //   0 0 0
    RUN_EXOSITE,          //   0 0 1
    RUN_PROVISIONING,     //   0 1 0
    PROGRAM_MODE,         //   0 1 1
    RUN_PROBE,            //   1 0 0
    GAINSPAN_CLIENT,      //   1 0 1
    SPI_PT_MODE,          //   1 1 0
    SWARM_CONN_MODE       //   1 1 1        RUN_BugLab
}AppMode_T;

typedef enum {
    UPDATE_LIGHT,
    UPDATE_TEMPERATURE,
    UPDATE_POTENIOMETER,
    UPDATE_ACCELEROMETER
} APP_STATE_E;

/*---------------------------------------------------------------------------*
 * Routine:  main
 *---------------------------------------------------------------------------*
 * Description:
 *      Setup the hardware, setup the peripherals, show the startup banner,
 *      wait for the module to power up, run a selected test or app.
 * Inputs:
 *      void
 * Outputs:
 *      int -- always 0.
 *---------------------------------------------------------------------------*/
uint16_t gAmbientLight;
float gTemp_F;
typedef union {
	int16_t		temp;
	uint8_t		T_tempValue[2];
} temp_u;
extern void SPI2_Init(void);
extern void SPI_Init(uint32_t bitsPerSecond);
extern void App_Exosite(void);
extern void App_WhiskerGW(void);
extern void App_WebProvisioning_OverAirPush(void);
extern void App_StartupADKDemo(uint8_t isLimiteAPmode);
extern int LimitedAP_TCP_SereverBulkMode(void);

int  main(void)
{
    AppMode_T AppMode;  APP_STATE_E state=UPDATE_TEMPERATURE; 
    
  
    HardwareSetup();  

    AppMode = RUN_EXOSITE;
    if(Switch2IsPressed())
    {
        AppMode = RUN_PROVISIONING;
    }
    
    
    /************************initializa LCD module********************************/
    SPI2_Init();
    InitialiseLCD();
    led_init();
    MSTimerInit();
    
    // DisplayLCD(LCD_LINE1, "Starting..."); 
    /*****************************************************************************/  
    SPI_Init(GAINSPAN_SPI_RATE);  
   /* Setup LCD SPI channel for Chip Select P10, active low, active per byte  */
    SPI_ChannelSetup(GAINSPAN_SPI_CHANNEL, false, true);
    GainSpan_SPI_Start();
        
    // Set SAU0 enable and UART0/UART1 baud rate
    RL78G14RDK_UART_Start(GAINSPAN_CONSOLE_BAUD, GAINSPAN_CONSOLE_BAUD);
    PM15 &= ~(1 << 2);       //EInk hand
    P15 &= ~(1 << 2);

      if (AppMode == RUN_EXOSITE)
    {          
      
        LCDSelectFont(FONT_SMALL);
        DisplayLCD(LCD_LINE1, "W.io Gateway");
        DisplayLCD(LCD_LINE2, "Version 0.A");
        Temperature_Init();
        Potentiometer_Init();  
        //App_Exosite();                                  // Run the Exosite cloud demo
        App_WhiskerGW();
    }
    else if(AppMode == RUN_PROVISIONING)
    {
      App_WebProvisioning_OverAirPush();               // run provisioning and over air upgrade
    }
    
    return 0;
}
/*-------------------------------------------------------------------------*
 * End of File:  main.c
 *-------------------------------------------------------------------------*/

