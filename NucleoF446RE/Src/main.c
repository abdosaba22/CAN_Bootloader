/* ************************************************ */
/* Author      : Abd-alrahman Amin                  */
/* SWC         : main			                    */
/* Layer       : App                                */
/* Version     : 1.1                                */
/* Date        : November 10, 2023                  */
/* Last Edit   : N/A             			        */
/* ************************************************ */

#include <stdint.h>

#include "LBIT_math.h"
#include "LSTD_types.h"

#include "MRCC_interface.h"
#include "MGPIO_interface.h"
#include "MNVIC_interface.h"
#include "MEXTI_interface.h"
#include "MSYSTICK_interface.h"
#include "UART_Interface.h"
#include "CAN_interface.h"
//#include "MFPEC_interface.h"

#include "hexfile.h"

void CAN_TX(u8* ARG_pu8BaseAddress, u8 ARG_u8DLCValue );
void CAN_TXRecord(u8* ARG_pu8BaseAddress);
u8 CAN_RX(void);
u16 Std_id_High(u16 local_u16Std_Id);
void CAN_FilterConfig(void);

u8 calculate_checksum(u8* hex_record);

void Parser_voidParseRecord(u8* Copy_u8BufData);
u8 AsciToHex(u8 Copy_u8Asci);

void BL_voidTimeout(void);
void Jump_APP(void);
void callff(void);


/*Backup Variables*/
u8 lastPageFlag = 0;
u8 backupPageCount = 0;
u8 burstDataWrite[1024] = {0};
u8 burstDataRead[1024] = {0};
char debugData[100]={0};

/*CAN Messages*/
//u8 gEndDATA[6]={"done*"};
u8 gDATA[50]; //= {":10C1000091180008000000009118000885150008AC"}; /*buffer to receive Data */
//u8 gTXDATA[4]={"OK*"};

/* will be base address from flash lets say 0x08004000 */
#define APP_BaseAddress (0x0800C000) /* scetor 3 */

volatile u8 BL_u8Timeout = 0;

/*Jump to app pointer*/
typedef void (*Function_t)(void);
Function_t addr_to_call = 0;



int main(void)
{
	/* Init RCC HSE 8 MHZ */
	MRCC_voidInit();
	MRCC_stderrEnablePeripheralClock(AHB1_GPIOA_EN);
	MRCC_stderrEnablePeripheralClock(AHB1_GPIOC_EN);
	MRCC_stderrEnablePeripheralClock(APB2_SYSCFG_EN);
    MRCC_stderrEnablePeripheralClock(APB1_USART2_EN);
	/**  Enable the CAN interface clock **/
	MRCC_stderrEnablePeripheralClock(APB1_CAN1_EN);

	/* Systick init*/
	MSTK_stderrInit(MSTK_CLOCKSRC_AHB_8);

	/* configure PinA5 as: Output Pushpull, Nopull Lowspeed */
	MGPIO_CONFIG_t LedPin1={
			.Port=PORTA,
			.Pin=PIN5,
			.Mode=OUTPUT,
			.Speed=LOW,
			.OutputType=PUSH_PULL,
			.PullType=NOPULL
	};

	/* configure USART pins */
	MGPIO_CONFIG_t USART2_RxPin ={
			.Port = PORTA,
			.Pin = PIN3,
			.Mode = ALTERNATE_FUNCTION,
			.AltFunc = AF7,
			.PullType = NOPULL
	};

	MGPIO_CONFIG_t USART2_TxPin ={
			.Port = PORTA,
			.Pin = PIN2,
			.Mode = ALTERNATE_FUNCTION,
			.AltFunc = AF7,
			.PullType = NOPULL
	};

	MGPIO_stderrPinConfig(&USART2_RxPin);
	MGPIO_stderrPinConfig(&USART2_TxPin);


	/* CAN Pins */
	MGPIO_CONFIG_t CAN_RXA11={
			.Port=PORTA,
			.Pin=PIN11,
			.Mode=ALTERNATE_FUNCTION,
			.AltFunc = AF9,
			.PullType=NOPULL
	};
	MGPIO_CONFIG_t CAN_TXA12={
			.Port=PORTA,
			.Pin=PIN12,
			.Mode=ALTERNATE_FUNCTION,
			.OutputType=PUSH_PULL,
			.AltFunc = AF9,
	};

	/** Configure CAN pins **/
	MGPIO_stderrPinConfig(&CAN_TXA12);
	MGPIO_stderrPinConfig(&CAN_RXA11);

	/** Configure GPIO pins **/
	MGPIO_stderrPinConfig(&LedPin1); /* LED */
	MGPIO_stderrSetPinValue(PORTA, PIN5, PIN_LOW);

	/* Blink Led A5 to check STK time */
	MGPIO_stderrSetPinValue(PORTA, PIN5, PIN_HIGH);
	MSTK_voidBusyDelay(4000000); // 4s
	MGPIO_stderrSetPinValue(PORTA, PIN5, PIN_LOW);

	/* initialize USART peripheral */
	USART_Config_t USART2_Recive ={
			.USART_Num = USART_2,
			.USART_Baud = USART_BAUD_115200,
			.USART_HWFlowControl = USART_HW_FLOW_CTRL_NONE,
			.USART_Mode = USART_MODE_TXRX,
			.USART_NoOfStopBits = USART_STOPBITS_1,
			.USART_Oversampling = USART_OVERSAMPLING_16,
			.USART_SynchMode = ASYNCH,
			.USART_ParityControl = USART_PARITY_DISABLE,
			.USART_WordLength = USART_WORD_WIDTH_8BITS,
			.USART_InterruptEnable={0,0,0,0,0,0}
	};

	MUSART_stderrInit( &USART2_Recive);


	/* CAN init */
	CAN_voidInit();
	/* CAN Filter set */
	CAN_FilterConfig();
	/* CAN Start */
	CAN_voidStart();
	MGPIO_stderrSetPinValue(PORTA, PIN5, PIN_HIGH);
	/* wait response from Node */
	CAN_RX();
	MGPIO_stderrSetPinValue(PORTA, PIN5, PIN_LOW);
	MSTK_voidBusyDelay(1000); // 1ms
	/* Loop forever */
	for(;;)
	{

		CAN_TXRecord(&hexdata[0][0]);

	}

}


void CAN_TX(u8* ARG_pu8BaseAddress, u8 ARG_u8DLCValue )
{

	CAN_TxHeaderTypeDef tx_header;
	tx_header.StdId = 0x85D;/* change id if needed*/
	tx_header.IDE = CAN_ID_STD;
	tx_header.RTR = CAN_RTR_DATA;
	/* assign the DLC value */
	tx_header.DLC = ARG_u8DLCValue;
	tx_header.TransmitGlobalTime=DISABLE;

	CAN_voidAddTxMsg(&tx_header, ARG_pu8BaseAddress);
}

void CAN_TXRecord(uint8_t * ARG_pu8BaseAddress)
{
	 uint8_t i=0 , counter=0  ;
//	 uint16  ID=0;
	/* this func will transmit 8 byte from record in frame if the terminator found then send the length */
	/* just count the number of DLC in last frame */
	/* note the terminator will be  '\0' */
	while(1)
	{
		for( i= 0; ARG_pu8BaseAddress[i] != '\0'; i++)
		{
			counter ++;
			if( counter == 8 )
			{
				MSTK_voidBusyDelay(2000);
				/* transmit data and counter=0 */
				CAN_TX( &ARG_pu8BaseAddress[i - 7 ] , counter );
				counter=0 ;
			}
		}
		/* to send the last part of record */
		if(counter != 0 )
		{
			MSTK_voidBusyDelay(2000);
			/* we will call func CAN func TX it will transmit the frame */
			CAN_TX( &ARG_pu8BaseAddress[ i - counter ] , counter);
		}
		else
		{
			/* do nothing */
		}
		CAN_RX();

		MUSART_stderrSendString(USART_2,ARG_pu8BaseAddress);
		MUSART_stderrSendString(USART_2,"\r\nFlashed..\r\n");

		if (strcmp(gDATA, "done") == 0)
		{
			MUSART_stderrSendString(USART_2,"\r\nDone Code Flashed..\r\n");
			break;
		}

		counter=0;
		ARG_pu8BaseAddress += 50;
	}
}
//void CAN_TXRecord(u8* ARG_pu8BaseAddress)
//{
//	u8 i , counter=0 ;
//	/* this func will transmit 8 byte from record in frame if the terminator found then send the length */
//	/* just count the number of DLC in last frame */
//	/* note the terminator will be  '\0'  */
//	for( i= 0; ARG_pu8BaseAddress[i] != '\0' ; i++)
//	{
//		counter ++;
//
//		if( counter == 8 )
//		{
//
//			/* transmit data and counter=0 */
//			CAN_TX( &ARG_pu8BaseAddress[i - 7 ] , counter );
//			counter=0 ;
//		}
//	}
//	/* to send the last */
//	if(counter != 0 )
//	{
//		/* we will call func CAN func TX it will transmit the frame */
//		CAN_TX( &ARG_pu8BaseAddress[ i - counter ] , counter );
//	}
//	else
//	{
//		/* do nothing */
//	}
//}


u8 CAN_RX(void)
{
	CAN_RxHeaderTypeDef Rx_header;
	u8 i;
	for(i=0 ;(i == 0) ;  ){
		while( (CAN_voidRXPending(CAN_RX_FIFO0) == 0 ));
		i=CAN_voidGetRxMsg(CAN_RX_FIFO0,&Rx_header,gDATA);
	}
	return i ; /* to indicate the end of code will check this value in main before parse code if(== 2  ) { jjump to APP by call Func}*/

}

u16 Std_id_High(u16 local_u16Std_Id)
{
	return (local_u16Std_Id << 5);
}
void CAN_FilterConfig(void)
{
	CAN_FilterTypeDef CAN_FilterInit;
	CAN_FilterInit.FilterActivation = ENABLE;
	CAN_FilterInit.FilterBank = 0;
	CAN_FilterInit.FilterFIFOAssignment = CAN_FILTER_FIFO0;
	CAN_FilterInit.FilterIdHigh = Std_id_High(0x55D); // 95D
	CAN_FilterInit.FilterIdLow = 0x0000;
	CAN_FilterInit.FilterMaskIdHigh = Std_id_High(0x55D);
	CAN_FilterInit.FilterMaskIdLow = 0x0000;
	CAN_FilterInit.FilterMode = CAN_FILTERMODE_IDMASK;
	CAN_FilterInit.FilterScale = CAN_FILTERSCALE_32BIT;
	CAN_voidConfigFilter(&CAN_FilterInit);
}

u8 calculate_checksum(u8* hex_record)
{
	u16 sum = 0;
	u16 length = strlen(hex_record)-2;
	u8 checksum=((AsciToHex(hex_record[length])<<4) | AsciToHex(hex_record[length+1]));
	for (u8 i = 1; i < length; i += 2) {
		if (i + 1 < length) {
			sum += ((AsciToHex(hex_record[i])<<4) | AsciToHex(hex_record[i+1])) ;
		}
	}
	sum = (u8)(0x100 - (sum & 0xFF)) ;
	return(sum==checksum)?1:0;
}

void callff(void){
	MGPIO_stderrTogglePinValue(PORTA, PIN5);
}


//void Jump_APP(void)
//{
//#define SCB_VTOR   *((volatile u32*)0xE000ED08)
//
//	SCB_VTOR = 0x0800C000;  /*new address for flash code App code */
//
//	addr_to_call = *(Function_t*)(0x0800C004);
//	addr_to_call();
//
//}

//void BL_voidBackup(void)
//{
//
//	/*Backup process*/
//	/*16 is number of pages to be backed up (TODO #define)*/
//	while(backupPageCount<16)
//	{
//		/*Read From Flash Page by Page until a known delimiter is found*/
//		for(int i =0; i<1024; i++)
//		{
//			burstDataWrite[i] = *((u8 *)(0x08004000+i+(backupPageCount*1024))) /*Replace (i) with byte read from flash*/;
//		}
//
//		/*Write the read page from the flash to EEPROM*/
//		for(int i =0; i<1024; i+=64)
//		{
//			HNVM_stderrBurstWrite(i+(backupPageCount*1024), &burstDataWrite[i&960], 64);
//			MSTK_voidBusyWait(5000);
//		}
//
//		backupPageCount++;
//	}
//
//	/*Save the number of pages in a location to be known in this case 0x200FF*/
//	HNVM_voidWriteByte(0x200FF, backupPageCount);
//	MSTK_voidBusyWait(5000);
//
//}


//void BL_voidRestore(void)
//{
//	/*Restore From Backup Process*/
//	/*Read the number of pages saved during last backup*/
//	backupPageCount = HNVM_u8ReadByte(0x200FF);
//
//	/*Erase App Area*/
//	MFPEC_voidEraseAppArea();
//
//	/*Restore From Backup*/
//	for(int i =0; i<backupPageCount; i++)
//	{
//		/*Read a page from EEPROM*/
//		HNVM_stderrBurstRead(i*1024 , burstDataRead, 1024);
//
//		/*Write the page into flash using FPEC inside a loop*/
//		MFPEC_voidFlashWrite(((u32)(APP_BaseAddress+ (i*1024))) ,(u16*)(&burstDataRead[0]),512);
//
//
//		//		/*Debugging Only*/
//		//		sprintf(debugData, "Start Address: 0x%X\n",i);
//		//		MUART_voidSendString(MUART_UART2, debugData);
//		//
//		//		for(int j = 0; j<1024; j++)
//		//		{
//		//			sprintf(debugData, "Address: 0x%X\tData: 0x%X\n",i+j , burstDataRead[j]);
//		//			MUART_voidSendString(MUART_UART2, debugData);
//		//		}
//	}
//
//	Jump_APP();
//}

//void BL_voidTimeout(void)
//{
//	BL_u8Timeout = 1;
//}

