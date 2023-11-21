#include <LBIT_math.h>
#include <LSTD_types.h>
#include <stdio.h>
#include <string.h>

#include <MRCC_interface.h>
#include <MGPIO_interface.h>
#include <CAN_interface.h>
#include <MSTK_interface.h>
#include <MFPEC_interface.h>
#include <MUART_interface.h>


void CAN_TX(u8* ARG_pu8BaseAddress, u8 ARG_u8DLCValue );
void CAN_TXRecord(u8* ARG_pu8BaseAddress);
u8 CAN_RX(void);
u16 Std_id_High(u16 local_u16Std_Id);
void CAN_FilterConfig(void);
u8 calculate_checksum(u8* hex_record);
void Parser_voidParseRecord(u8* Copy_u8BufData);
void BL_voidTimeout(void);
void Jump_APP(void);


/*CAN Messages*/
u8 gEndDATA[6]={"done*"};
u8 gDATA[50]; /*buffer to receive Data */
u8 gTXDATA[4]={"OK*"};

/* will be base address from flash lets say 0x08004000 */
#define APP_BaseAddress (0x08005000)

volatile u8 BL_u8Timeout = 0;

/*Jump to app pointer*/
typedef void (*Function_t)(void);
Function_t addr_to_call = 0;


int main()
{
	/* Rcc init */
	MRCC_stderrInitSystemClock(MRCC_SYSCLOCK_HSE_CRYSTAL); /*default is 8M */
	MRCC_stderrInitBusClock(MRCC_BUS_APB1, MRCC_APB1_NO_PRESCALAR);
	MRCC_stderrInitBusClock(MRCC_BUS_APB2, MRCC_APB2_NO_PRESCALAR);

	/*Enable Clock Security System*/
	MRCC_voidEnableCSS();

	/* GPIO init */
	MRCC_stderrEnablePeripheralClock(MRCC_PERIPHERAL_GPIOA);
	MRCC_stderrEnablePeripheralClock(MRCC_PERIPHERAL_GPIOC);

	/*Initialize SysTick*/
	MSTK_stderrInit(MSTK_CLOCKSRC_AHB_8);


	MGPIO_stderrSetPinMode(MGPIO_PIN_A7, MGPIO_MODE_INPUT_PULLDOWN);
	MGPIO_stderrSetPinMode(MGPIO_PIN_C13, MGPIO_MODE_OUTPUT_PUSHPULL_2MHZ);
	MGPIO_stderrSetPinState(MGPIO_PIN_C13, MGPIO_LOW);
	MSTK_voidBusyWait(1000000);
	MGPIO_stderrSetPinState(MGPIO_PIN_C13, MGPIO_HIGH);
	/* CAN init */
	MRCC_stderrEnablePeripheralClock(MRCC_PERIPHERAL_CAN);
	CAN_voidInit();
	/* CAN Filter set */
	CAN_FilterConfig();
	/* CAN Start */
	CAN_voidStart();

	volatile u8  L_u8Terminator = 0;
	u8 L_u8PinState = 0;

	/*Start SysTick and Wait for a timeout 10s */
	MSTK_stderrStartSingle(10000000, BL_voidTimeout);

	/*Poll Pin*/
	while(!L_u8PinState)
	{
		if(BL_u8Timeout)
		{
			BL_voidRestore();
		}
		else
		{
			MGPIO_stderrGetPinState(&L_u8PinState, MGPIO_PIN_A7);
		}

	}
	L_u8PinState = 0;
	/*Erase App Area*/
	MFPEC_voidEraseAppArea();

	CAN_TX(gTXDATA,3);
	MSTK_voidStop();


	while(1)
	{
		/* polling for new reord*/
		L_u8Terminator=CAN_RX();
		MSTK_voidBusyWait(1);
		/* in case CAN_RX return 2 means that code finished */
		if(L_u8Terminator==2 )
		{
			CAN_TX(gEndDATA,6);   /* send done before jump to app */
			MSTK_voidBusyWait(1);
			/* jump to new APP */
			Jump_APP();
		}
		if(calculate_checksum(gDATA))  /* check if the checksum is correct to check for can bus integrity */
		{
			Parser_voidParseRecord(gDATA); /* parse code after receive it */
			/* Send message like ACK */
			CAN_TX(gTXDATA,3); /* send OK to receive the next record */
		}
		else
		{
			/* Send message like NACK to resend the record */
			
		}
	}
	return 0;
}


void CAN_TX(u8* ARG_pu8BaseAddress, u8 ARG_u8DLCValue )
{

	CAN_TxHeaderTypeDef tx_header;
	tx_header.StdId = 0x55D;
	tx_header.IDE = CAN_ID_STD;
	tx_header.RTR = CAN_RTR_DATA;
	/* assign the DLC value */
	tx_header.DLC = ARG_u8DLCValue;
	tx_header.TransmitGlobalTime=DISABLE;

	CAN_voidAddTxMsg(&tx_header, ARG_pu8BaseAddress);
}

void CAN_TXRecord(u8* ARG_pu8BaseAddress)
{
	u8 i , counter=0 ;
	/* this func will transmit 8 byte from record in frame if the terminator found then send the length */
	/* just count the number of DLC in last frame */
	/* note the terminator will be  '\0'  */
	for( i= 0; ARG_pu8BaseAddress[i] != '\0' ; i++)
	{
		counter ++;

		if( counter == 8 )
		{

			/* transmit data and counter=0 */
			CAN_TX( &ARG_pu8BaseAddress[i - 7 ] , counter );
			counter=0 ;
		}
	}
	/* to send the last */
	if(counter != 0 )
	{
		/* we will call func CAN func TX it will transmit the frame */
		CAN_TX( &ARG_pu8BaseAddress[ i - counter ] , counter );
	}
	else
	{
		/* do nothing */
	}
}


u8 CAN_RX(void)
{
	CAN_RxHeaderTypeDef Rx_header;
	u8 i;
	for(i=0 ;!BL_u8Timeout && (i == 0) ;  ){
		while(!BL_u8Timeout && (CAN_voidRXPending(CAN_RX_FIFO0) == 0 ));
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
	CAN_FilterInit.FilterIdHigh = Std_id_High(0x85D);
	CAN_FilterInit.FilterIdLow = 0x0000;
	CAN_FilterInit.FilterMaskIdHigh = Std_id_High(0x85D);
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

void Jump_APP(void)
{
#define SCB_VTOR   *((volatile u32*)0xE000ED08)

	SCB_VTOR = 0x08005000;  /*new address for flash code App code */

	addr_to_call = *(Function_t*)(0x08005004);
	addr_to_call();

}


void BL_voidTimeout(void)
{
	BL_u8Timeout = 1;
}
