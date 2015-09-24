//#include <drv/SPI.h>
#include "..\YRDKRL78G14\applilet3_src\r_cg_macrodriver.h"
#include "..\YRDKRL78G14\applilet3_src\r_cg_serial.h"
#include <drv/RDKRL78_spi.h>
//#include <system/platform.h>
#include "xr20m1170.h"
#define NULL 0

int spiTransferComplete;
void write(unsigned char reg, unsigned char val)
{
    unsigned int scrold = SCR11;
    unsigned char buff[2];//, rxBuff[2];
    buff[0] = reg<<3;
    buff[1] = val;
    // wait for the SPI to free up.
    //while(SPI_IsBusy(4));
    ///spiTransferComplete=0;
    SCR11 = 0xf007;
    ST1 |= _0002_SAU_CH1_STOP_TRG_ON;        /* disable CSI21 */
    SOE1 &= ~_0002_SAU_CH1_OUTPUT_ENABLE;    /* disable CSI21 output */
    SO1 &= ~_0200_SAU_CH1_CLOCK_OUTPUT_1;    /* CSI21 clock initial level */
    SOE1 |= _0002_SAU_CH1_OUTPUT_ENABLE;           /* enable CSI21 output */
    SS1 |= _0002_SAU_CH1_START_TRG_ON;             /* enable CSI21 */
    SPI_Send(3,buff,2);
    SCR11 = scrold;    
    ST1 |= _0002_SAU_CH1_STOP_TRG_ON;        /* disable CSI21 */
    SOE1 &= ~_0002_SAU_CH1_OUTPUT_ENABLE;    /* disable CSI21 output */
    SO1 |= _0200_SAU_CH1_CLOCK_OUTPUT_1;    /* CSI21 clock initial level */
    SOE1 |= _0002_SAU_CH1_OUTPUT_ENABLE;           /* enable CSI21 output */
    SS1 |= _0002_SAU_CH1_START_TRG_ON;             /* enable CSI21 */
    //SPI_Transfer(4, 2, buff, rxBuff, SPI_TransferComplete);
    //MSTimerDelay(250);
    //while(spiTransferComplete==0);
}

unsigned char read(unsigned char reg)
{
    unsigned int scrold = SCR11;
    unsigned char buff[2];
    buff[0] = reg<<3 | 0x80;
    buff[1] = 0;
    // wait for the SPI to free up.
    //while(SPI_IsBusy(4));
   // spiTransferComplete=0;
    SCR11 = 0xf007;    
    ST1 |= _0002_SAU_CH1_STOP_TRG_ON;        /* disable CSI21 */
    SOE1 &= ~_0002_SAU_CH1_OUTPUT_ENABLE;    /* disable CSI21 output */
    SO1 &= ~_0200_SAU_CH1_CLOCK_OUTPUT_1;    /* CSI21 clock initial level */
    SOE1 |= _0002_SAU_CH1_OUTPUT_ENABLE;           /* enable CSI21 output */
    SS1 |= _0002_SAU_CH1_START_TRG_ON;             /* enable CSI21 */
    SPI_SendReceive(3,buff,2,buff);
    SCR11 = scrold;
        ST1 |= _0002_SAU_CH1_STOP_TRG_ON;        /* disable CSI21 */
    SOE1 &= ~_0002_SAU_CH1_OUTPUT_ENABLE;    /* disable CSI21 output */
    SO1 |= _0200_SAU_CH1_CLOCK_OUTPUT_1;    /* CSI21 clock initial level */
    SOE1 |= _0002_SAU_CH1_OUTPUT_ENABLE;           /* enable CSI21 output */
    SS1 |= _0002_SAU_CH1_START_TRG_ON;             /* enable CSI21 */
    //SPI_Transfer(4, 2, buff, buff, SPI_TransferComplete);
    //while(spiTransferComplete==0);
   // MSTimerDelay(250);
    return buff[1];
}
// Initialize XR20M1170 
// returns: 0 - Initialization OK
//          -1 - Could not communicate with XR20M1170
int spiUartInitialize()
{
   // SPI_ChannelSetup(4,0,1);
    PM8 = 0xf7;
    P8 = 0xff;
    // first, test to make sure we can actually communicate with the 
    // device
    write(SPR,0x55);
    if(read(SPR)!=0x55)
      return -1;
    // enable access to enhanced registers
    write (LCR, 0xBF);	
    // set the default XON/XOFF characters
    // note: we don't use software flow control, so these
    // are being set just for completeness because I was too lazy to
    // make sure it works properly if we don't set them - sjm 090615
    write (XOFF1, XOFF1_CHAR);
    write (XON1, XON1_CHAR);
    write (XOFF2, XOFF2_CHAR);
    write (XON2, XON2_CHAR);
    // enable shaded bits and access to DLD, and  no flow control
    write (EFR, 0x10 );	
    // enable access to dividers
    write (LCR, 0x80);	
    write (DLM, DLM_115K);
    write (DLL, DLL_115K);	
    write (DLD, DLD_115K);
    // set data format 8N1
    write (LCR, 0x03);	
    // RX trigger level = 56 chacters in buffer
    // TX trigger level = 8 spaces empty
    // enable and reset FIFO
    // NOTE: we are not actually using interrupts here, so the trigger 
    // values are not critical.  These would need to be tuned if 
    // interrupts were going to be used to match the command and response
    // lengths for the Whisker.IO Engine.
    write(FCR,0x87);
    write (IER, 0x00);  
    //write(MCR,0x08);
    return 0;
}
unsigned char spiUartRxBytesAvailable()
{
    unsigned char retVal = read(LSR); 
      retVal = read(RXLVL);
    return retVal;
}
unsigned char spiUartTxSpacesAvailable()
{
    unsigned char retVal = read(TXLVL);
    return retVal;
}
unsigned char spiUartGetByte()
{
    unsigned char retVal = read(RHR);
    return retVal;
}
unsigned char spiUartResetFIFO()
{
  	
    // RX trigger level = 56 chacters in buffer
    // TX trigger level = 8 spaces empty
    // enable and reset FIFO
    // NOTE: we are not actually using interrupts here, so the trigger 
    // values are not critical.  These would need to be tuned if 
    // interrupts were going to be used to match the command and response
    // lengths for the Whisker.IO Engine.
    write(FCR,0x87);
    write (IER, 0x00); 
}
void spiUartSendByte(unsigned char byteToSend)
{
    write(THR, byteToSend);
}