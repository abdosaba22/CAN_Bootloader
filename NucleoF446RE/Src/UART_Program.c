/* ************************************************ */
/* Author      : Abd-alrahman Amin                  */
/* SWC         : USART 				                */
/* Layer       : MCAL                               */
/* Version     : 1.0                                */
/* Date        : September 10, 2023                 */
/* Last Edit   : N/A 			                    */
/* ************************************************ */

#include "BITMATH.h"
#include "ERRORTYPE.h"
#include "LBIT_math.h"
#include "LSTD_types.h"
#include <stdint.h>

#include "UART_Private.h"
#include "UART_Interface.h"



static USART_RegDef_t *G_parrUSARTArr[USART_NUM] = {USART1,USART2,USART3,USART4,USART5,USART6};

//static volatile void(* pvoidfCallbackFunc[USART_NUM][3])(void)={NULL_POINTER};



STD_error_t MUSART_stderrInit(USART_Config_t *ARG_pUSARTHandle)
{
	STD_error_t L_stderrState = E_OK;
	if(ARG_pUSARTHandle != NULL)
	{
		/* need to check the validation of the struct values */

		//Variable to hold the APB clock
		uint32_t PCLKx = 8000000U; /* by default clk is 16Mhz as HSI */
		//variables to hold Mantissa and Fraction values
		uint32_t L_u32TempReg=0,L_u32USARTDiv;
		uint16_t L_u16DivMant;
		uint8_t  L_u8DivFra;
		/*  1st config CR1 */

		/* Enable UART */
		L_u32TempReg |= (1<<CR1_UE);
		/*Enable UART according to the Mode @USART_Mode_t */
		L_u32TempReg|= ( ARG_pUSARTHandle->USART_Mode << CR1_RE);
		/* Configure the Word length @USART_WordLength_t */
		L_u32TempReg |= ( ARG_pUSARTHandle->USART_WordLength << CR1_M );
		/* Configuration of parity control bit */
		L_u32TempReg |= ( ARG_pUSARTHandle->USART_ParityControl << CR1_PS);
		/* Configure the over sampling configuration item */
		L_u32TempReg |= ( ARG_pUSARTHandle->USART_Oversampling << CR1_OVER8);
		/* Configure Interrupt Bits  */
		L_u32TempReg |= ( ((ARG_pUSARTHandle->USART_InterruptEnable.RXNEIE)| (ARG_pUSARTHandle->USART_InterruptEnable.TCIE << 1 ) | (ARG_pUSARTHandle->USART_InterruptEnable.TXEIE << 2)  ) << CR1_RXNEIE  );
		/* Assign the TempReg to CR1 register */
		G_parrUSARTArr[ARG_pUSARTHandle->USART_Num]->CR1 = L_u32TempReg;

		/* 2nd Config CR2 */
		L_u32TempReg = 0;
		//Implement the code to configure the number of stop bits inserted during USART frame transmission */
		L_u32TempReg |= ARG_pUSARTHandle->USART_NoOfStopBits << CR2_STOP;
		//Implement the code to configure weather the peripheral Work as Sycnh or Asynch */
		L_u32TempReg |= ARG_pUSARTHandle->USART_SynchMode << CR2_CLKEN;
		/* ****
		 *if there is synchronous mode need to config CPOL : CPHA *******
		 **** */
		/* Assign the TempReg to CR2 register */
		G_parrUSARTArr[ARG_pUSARTHandle->USART_Num]->CR2 = L_u32TempReg;

		// 3rd Config CR3 */
		L_u32TempReg=0;
		//Configuration of USART hardware flow control */
		L_u32TempReg |= (ARG_pUSARTHandle->USART_HWFlowControl << CR3_RTSE);
		/* Assign the TempReg to CR3 register */
		G_parrUSARTArr[ARG_pUSARTHandle->USART_Num]->CR3 = L_u32TempReg;
		L_u32TempReg=0;

		// Configure the baud rate
		//Check for OVER8 configuration bit
		if ( 1== ARG_pUSARTHandle->USART_Oversampling)
		{
		   //OVER8 = 1 , over sampling by 8
		   L_u32USARTDiv = ((25 * PCLKx) / (2 * ARG_pUSARTHandle->USART_Baud));
		}
		else
		{
		   //Over sampling by 16
			L_u32USARTDiv = ((25 * PCLKx) / (4 *ARG_pUSARTHandle->USART_Baud));
		}
		//Calculate the Mantissa part
		L_u16DivMant = L_u32USARTDiv/100;
		//Place the Mantissa part in appropriate bit position. refer USART_BRR
		L_u32TempReg |= L_u16DivMant << USART_BRR_MANFRA_BIT;
		//Extract the fraction part
		L_u8DivFra = (L_u32USARTDiv - (L_u16DivMant * 100));

		//Calculate the final fractional
		if ( 1== ARG_pUSARTHandle->USART_Oversampling )
		{
			// over sampling by 8 -- bit 3 not considered must be cleared
			L_u8DivFra = ((( L_u8DivFra * 8)+ 50) / 100)& ((uint8_t)0x07);
		}
		else
		{
			//over sampling by 16
			L_u8DivFra = ((( L_u8DivFra * 16)+ 50) / 100) & ((uint8_t)0x0F);
		}

		//Place the fractional part in appropriate bit position . refer USART_BRR
		L_u32TempReg |= L_u8DivFra;
		/* Assign the TempReg to BRR register  set baud rate*/
		G_parrUSARTArr[ARG_pUSARTHandle->USART_Num]->BRR = L_u32TempReg;
//		G_parrUSARTArr[ARG_pUSARTHandle->USART_Num]->BRR = 11<<0 | 8<<4 ;

	}
	else
	{
		L_stderrState = E_NULL_POINTER;
	}

	return L_stderrState;
}


STD_error_t MUSART_stderrSendChar(USART_Number_t ARG_numUSARTNum , char ARG_charString )
{
	STD_error_t L_stderrState = E_OK;
	if(ARG_numUSARTNum < USART_NUM )
	{
		//wait until TXE flag is set in the SR
		while(!MUSART_u8GetFlagStatus(ARG_numUSARTNum, USART_FLAG_TXE));
		//set data transfer
		G_parrUSARTArr[ARG_numUSARTNum]->DR = ARG_charString  ;
	}
	else
	{
		L_stderrState = E_NOK;
	}
	
	return L_stderrState;
}

STD_error_t MUSART_stderrSendString	(USART_Number_t ARG_numUSARTNum , const char *ARG_ccharpString )
{

	STD_error_t L_stderrState = E_OK;
	
	if(ARG_numUSARTNum < USART_NUM )
	{
		uint32_t L_u32i=0;
		
		do{
			
			MUSART_stderrSendChar(ARG_numUSARTNum,ARG_ccharpString[L_u32i]);
			L_u32i++;
			//wait until TC flag is set in the SR
			while(!MUSART_u8GetFlagStatus(ARG_numUSARTNum, USART_FLAG_TC));
		}while(ARG_ccharpString[L_u32i] != '\0');
		
	}
	else
	{
		L_stderrState = E_NOK;

	}
	return L_stderrState;
}


char MUSART_charReceiveCharNonBlocking(USART_Number_t ARG_numUSARTNum )
{
	char L_charData=0;	
	//check RXNE flag 
	if( MUSART_u8GetFlagStatus(ARG_numUSARTNum, USART_FLAG_RXNE))
	{
		L_charData = (G_parrUSARTArr[ARG_numUSARTNum]->DR & 0xFF);
		
	}
	else
	{
		L_charData=0;
	}
	return L_charData;
}

char MUSART_charReceiveCharBlocking(USART_Number_t ARG_numUSARTNum )
{
	char L_charData=0;	
	
	//wait until RXNE flag is set in the SR
	while(!MUSART_u8GetFlagStatus(ARG_numUSARTNum, USART_FLAG_RXNE));
		
	L_charData = (G_parrUSARTArr[ARG_numUSARTNum]->DR & 0xFF);
	
	return L_charData;
}


STD_error_t MUSART_stderrReceiveStringUntil(USART_Number_t ARG_numUSARTNum , char *ARG_charpReceived, char  ARG_charTerminator)
{

	STD_error_t L_stderrState = E_OK;
	
	if(ARG_numUSARTNum < USART_NUM )
	{
		uint32_t L_u32i=0;
		
		do{
			
			ARG_charpReceived[L_u32i]= MUSART_charReceiveCharBlocking(ARG_numUSARTNum);
			L_u32i++;
	
		}while(ARG_charpReceived[L_u32i-1] != ARG_charTerminator);
		ARG_charpReceived[L_u32i-1] = '\0';
	}
	else
	{
		L_stderrState = E_NOK;

	}
	
	return L_stderrState;
}



uint8_t MUSART_u8GetFlagStatus(USART_Number_t ARG_numUSARTNum, uint8_t ARG_u8Flag)
{
	if ( (G_parrUSARTArr[ARG_numUSARTNum]->SR & ARG_u8Flag) ) {
		return FLAG_SET;
	}

	return FLAG_RESET;
}


STD_error_t MUART_stderrSetCallback(USART_Number_t ARG_numUSARTx, uint8_t ARG_u8InterruptSource , void(*ARG_pvoidfUserFunction)(void))
{
	STD_error_t L_stderrState = E_OK;
	/* will be handeled  later to set */
	/* may be changed to be func that handel the ISR and use another func to send_IT and Receive_IT */
	return L_stderrState;
}


/* USART2 Interrupt handler */
void USART2_IRQHandler(void)
{
	/* will be handeled later 
	 * read data in DR and callbackF  
	 */
	
}
