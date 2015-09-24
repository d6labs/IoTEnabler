#ifndef XR20M1170_H
#define XR20M1170_H

#define	RHR		0x0
#define	THR		0x0
#define	IER		0x1
#define	FCR		0x2
#define	ISR		0x2
#define	LCR		0x3
#define	MCR		0x4
#define	LSR		0x5
#define	MSR		0x6
#define	TCR		0x6
#define	SPR		0x7
#define	TLR		0x07
#define	TXLVL		0x08
#define	RXLVL		0x09
#define	IODIR		0x0A
#define	IOSTATE	0x0B
#define	IOINT		0x0C
#define	IOCONT	0x0E
#define	EFCR		0x0F
// these registers are available when LCR[7] is 1
#define	DLL		0
#define	DLM		1
#define DLD     2
// these registers are available when LCR=0xBF
#define  EFR		2
#define	XON1		4
#define	XON2		5
#define	XOFF1		6
#define	XOFF2		7

#define XON1_CHAR 0x13
#define XON2_CHAR 0x14
#define XOFF1_CHAR 0x11
#define XOFF2_CHAR 0x12

#define DLL_115K 0x0d
#define DLM_115K 0x00
#define DLD_115K 0x00

#define DLL_9600 0x9c
#define DLM_9600 0x00
#define DLD_9600 0x04

int spiUartInitialize();
unsigned char spiUartRxBytesAvailable();
unsigned char spiUartTxSpacesAvailable();
unsigned char spiUartGetByte();
void spiUartSendByte(unsigned char byteToSend);
#endif