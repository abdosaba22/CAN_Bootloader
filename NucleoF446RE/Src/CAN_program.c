/*********************************************************************************/
/* Author    : Abdalrahman Amin                                                  */
/* Version   : V03                                                               */
/* Date      : 30 OCT  2023                                                      */
/*********************************************************************************/
#include <LSTD_TYPES.h>
#include <LBIT_MATH.h>
#include <MRCC_interface.h>
//#include "NVIC_interface.h"
//#include <MGPIO_interface.h>

#include <CAN_interface.h>
#include <CAN_config.h>
#include <CAN_private.h>

u32 CAN_ErrorCode = CAN_ERROR_NONE;
/** BoudRate value **/
CAN_BitTimingConfig CAN_bitRateConfig[7];





void CAN_voidInit(void)
{
    /**  Enable the CAN interface clock **/
	MRCC_stderrEnablePeripheralClock(APB1_CAN1_EN);
	/** Configure CAN pins **/
    /**  exit CAN sleep mode **/
    /* CLEAR SLEEP bit (SLEEP) */
    CLR_BIT(CAN->MCR,1);
    /* wait to exit sleep mode */
 // while (!(GET_BIT(CAN->MSR,1) == 0));
     /**  Set CAN Initialization mode **/
    /* SET Initialization request BIT (INRQ) */
    SET_BIT(CAN->MCR,0);
    /* wait to enter in Initialization mode */
  //  while(!(GET_BIT(CAN->MSR,0) == 1));

    /** CHECK Transmit FIFO priority **/
#if TransmitFifoPriority == ENABLE
     /* by the request order */
        SET_BIT(CAN->MCR,2); //TXFP
#else
     /* by the identifier of the message */
        CLR_BIT(CAN->MCR,2); //TXFP
#endif
    /** CHECK Receive FIFO locked mode **/
#if ReceiveFifoLocked == ENABLE
     /* Receive FIFO locked against overrun.
         Once a receive FIFO is full the next incoming message will be discarded. */
        SET_BIT(CAN->MCR,3); //RFLM
#else
     /* Receive FIFO not locked on overrun.
         Once a receive FIFO is full the next incoming message will overwrite the previous one. */
        CLR_BIT(CAN->MCR,3); //RFLM
#endif
    /** CHECK automatic retransmission **/
#if AutoRetransmission == ENABLE
        CLR_BIT(CAN->MCR,4); //NART
#else
        SET_BIT(CAN->MCR,4); //NART
#endif

    /** CHECK automatic wakeup mode **/
#if AutoBussOff == ENABLE
        SET_BIT(CAN->MCR,5); //AWUM
#else
       CLR_BIT(CAN->MCR,5); //AWUM
#endif

    /** CHECK automatic bus-off management **/
#if AutoBussOff == ENABLE
        SET_BIT(CAN->MCR,6); //ABOM
#else
        CLR_BIT(CAN->MCR,6); //ABOM
#endif

    /** CHECK Time triggered communication mode **/
#if TimeTriggeredMode == ENABLE
        SET_BIT(CAN->MCR,7); //TTCM
#else
        CLR_BIT(CAN->MCR,7); //TTCM
#endif


     /** Set the bit timing register **/
  WRITE_REG(CAN->BTR, (u32) (MODE |
		  ((CAN_bitRateConfig[BOUDRATE].TS2-1) << 20) |
		  ((CAN_bitRateConfig[BOUDRATE].TS1-1) << 16) |
		  (CAN_bitRateConfig[BOUDRATE].BRP-1)));
}


void CAN_voidAddTxMsg( CAN_TxHeaderTypeDef *pTxHeader, u8 Local_u8Data[])
{
	u32 local_u8TransmitMailbox; // which mailbox is empty to set Local_u8Data in it (read CODE bit in TSR register)
	u32 local_u32TSR; //transmit status register
	local_u32TSR = READ_REG(CAN->TSR);
	/* Check that all the Tx mailboxes are not full */
	if ( (GET_BIT(local_u32TSR, 26) != 0x0) || /* TME0 */
			(GET_BIT (local_u32TSR,27) != 0x0) || /* TME1 */
			(GET_BIT(local_u32TSR,28) != 0x0) )  /* TME2 */
	{
		/* Select an empty transmit mailbox */
		local_u8TransmitMailbox = ((local_u32TSR >> 24) & 0x3); //(read CODE[1:0] bit in TSR register)
		/* Check transmit mailbox value */
		if (local_u8TransmitMailbox <=2)
		{
			/* Set up the Id */
			if (pTxHeader->IDE == CAN_ID_STD)
			{
				CAN->TxMailBox[local_u8TransmitMailbox].TIR = ((pTxHeader->StdId << 21) |
						pTxHeader->RTR);
			}
			else
			{
				CAN->TxMailBox[local_u8TransmitMailbox].TIR = ((pTxHeader->ExtId << 3) |
						pTxHeader->IDE | pTxHeader->RTR);
			}

			/* Set up the DLC */
			CAN->TxMailBox[local_u8TransmitMailbox].TDTR = (pTxHeader->DLC);
			/* Set up the Transmit Global Time mode */
			if (pTxHeader->TransmitGlobalTime == ENABLE)
			{
				SET_BIT(CAN->TxMailBox[local_u8TransmitMailbox].TDTR, 8);
			}
			/* Set up the Local_u8Data field */
			WRITE_REG(CAN->TxMailBox[local_u8TransmitMailbox].TDHR,
					((u32)Local_u8Data[7] << 24) |
					((u32)Local_u8Data[6] << 16) |
					((u32)Local_u8Data[5] << 8 ) |
					((u32)Local_u8Data[4]));
			WRITE_REG(CAN->TxMailBox[local_u8TransmitMailbox].TDLR,
					((u32)Local_u8Data[3] << 24) |
					((u32)Local_u8Data[2] << 16) |
					((u32)Local_u8Data[1] << 8 ) |
					((u32)Local_u8Data[0]));
			/* Request transmission */
			SET_BIT(CAN->TxMailBox[local_u8TransmitMailbox].TIR, 0);
		}
	}
}

void CAN_voidStart(void)
{
	/* CLEAR Initialization request BIT (INRQ) */
	CLR_BIT(CAN->MCR,0);
	/* wait to enter in Initialization mode */
	//while(!(GET_BIT(CAN->MSR,0) == 0));
}


u8 CAN_voidGetRxMsg(u32 local_u8RxFifo, CAN_RxHeaderTypeDef *pRxHeader, u8 *Local_u8Data)
{
	u8 u8_ReturnFlag=0;
	static u8 j=0;
    /* Get Identifier extension */
	pRxHeader->IDE =  GET_BIT(CAN->FIFOMailBox[local_u8RxFifo].RIR,2);
    if (pRxHeader->IDE == CAN_ID_STD)
    {/* GET the Id */
      pRxHeader->StdId = CAN->FIFOMailBox[local_u8RxFifo].RIR >> 21;
    }
    else
    {
    	pRxHeader->ExtId = CAN->FIFOMailBox[local_u8RxFifo].RIR >> 3;
    }
    /* GET the RTR */
    pRxHeader->RTR = GET_BIT(CAN->FIFOMailBox[local_u8RxFifo].RIR,1);
    /* GET the DLC */
    pRxHeader->DLC = (CAN->FIFOMailBox[local_u8RxFifo].RDTR & 0xF);
    /* GET the (FMI) Filter Match Index */
    pRxHeader->FilterMatchIndex = (CAN->FIFOMailBox[local_u8RxFifo].RDTR >> 8) & 0xFF;
    /* GET the Time stamp */
    pRxHeader->TimeStamp = (CAN->FIFOMailBox[local_u8RxFifo].RDTR >> 16) & 0XFFFF;

    /* Get the data */
    for(u8 i=j ; i < 8 ; i++)
    {/*clear buff*/
    	Local_u8Data[i] = 0;
    }
    for(u8 i=0 ; i < pRxHeader->DLC ; i++)
    {
    	if (i<4)
    		Local_u8Data[j] = (u8)((CAN->FIFOMailBox[local_u8RxFifo].RDLR >>(i*8) ) & 0XFF);
    	else
    		Local_u8Data[j] = (u8)((CAN->FIFOMailBox[local_u8RxFifo].RDHR >>((i-4)*8) ) & 0XFF);
    	/*Local_u8Data*/
    	if(Local_u8Data[j]=='*')
    	{
    		u8_ReturnFlag=1;
    		Local_u8Data[j] = '\0';
    		j=0;
    		break;
    	}
    	j++;
    }
		
    /* Release the FIFO */
    if (local_u8RxFifo == CAN_RX_FIFO0)
    {
      /* Release RX FIFO 0 */
      SET_BIT(CAN->RF0R, 5);
    }
    else
    {
      /* Release RX FIFO 1 */
      SET_BIT(CAN->RF1R, 5);
    }

    return u8_ReturnFlag;
}

/** CAN RX Pending **/
u8 CAN_voidRXPending(u8 local_u8RxFifo)
{
	/* return the Rx FIFO  message pending */
	if (local_u8RxFifo == CAN_RX_FIFO0) /* Rx element is assigned to Rx FIFO 0 */
	{
		/* return value of the Rx FIFO 0  */
	 return (CAN->RF0R & 0X3);

	}
	else /* Rx element is assigned to Rx FIFO 1 */
	{
		/* return value of the Rx FIFO 1  */
		return (CAN->RF1R & 0X3);
	}
}
/** CAN CONFIG filter **/
void CAN_voidConfigFilter( CAN_FilterTypeDef *pFilterConfig)
{
	/* Initialization mode for the filter */
	SET_BIT(CAN->FMR, 0);
	/* Convert filter number into bit position */
	u32 local_u32FilterBitPos = pFilterConfig->FilterBank ;

	/* Filter Deactivation */
	CLR_BIT(CAN->FA1R, pFilterConfig->FilterBank);

	/* Filter Scale */
	if (pFilterConfig->FilterScale == CAN_FILTERSCALE_16BIT)
	{
	  /* 16-bit scale for the filter */
	  CLR_BIT(CAN->FS1R, local_u32FilterBitPos);

	  /* Filter mapping
	     *   15-8          7-5    4   3    2-0
	     * STID[10:3] STID[2:0] RTR IDE EXID[17:15]
	     * */

	  /* First 16-bit identifier and First 16-bit mask */
	  /* Or First 16-bit identifier and Second 16-bit identifier */
	  CAN->FilterBankRegister[pFilterConfig->FilterBank].FR1 =(
			  	  	  	  	  	  	  ((pFilterConfig->FilterMaskIdLow & 0x0000FFFF) << 16) |
									  (pFilterConfig->FilterIdLow	   & 0x0000FFFF) );

	  /* Second 16-bit identifier and Second 16-bit mask */
	  /* Or Third 16-bit identifier and Fourth 16-bit identifier */
	  CAN->FilterBankRegister[pFilterConfig->FilterBank].FR2 =(
			  	  	  	  	  	  	  	  	  	 ((pFilterConfig->FilterMaskIdHigh & 0x0000FFFF) << 16) |
			  									  (pFilterConfig->FilterIdHigh 	   & 0x0000FFFF) );
	}

	else if (pFilterConfig->FilterScale == CAN_FILTERSCALE_32BIT)
	{
	  /* 32-bit scale for the filter */
	  SET_BIT(CAN->FS1R, local_u32FilterBitPos);

	  /* Filter mapping                                   
		 *   31-24      23-21     21-16  		15-8	 7-3	  2   1  0
		 * STID[10:3] STID[2:0] EXID[17:13] EXID[12:5] EXID[4:0] IDE RTR 0
		 * */

	  /* 32-bit identifier or First 32-bit identifier */
	  CAN->FilterBankRegister[pFilterConfig->FilterBank].FR1 =(
											((pFilterConfig->FilterIdHigh & 0x0000FFFF) << 16) |
											 (pFilterConfig->FilterIdLow  & 0x0000FFFF));

	  /* 32-bit mask or Second 32-bit identifier */
	  CAN->FilterBankRegister[pFilterConfig->FilterBank].FR2 =(
												((pFilterConfig->FilterMaskIdHigh & 0x0000FFFF) << 16) |
												 (pFilterConfig->FilterMaskIdLow & 0x0000FFFF));
	}

	/* Filter Mode */
	if (pFilterConfig->FilterMode == CAN_FILTERMODE_IDMASK)
	{
	  /* Id/Mask mode for the filter*/
	  CLR_BIT(CAN->FM1R, local_u32FilterBitPos);
	}
	else
	{
	  /* Identifier list mode for the filter*/
	  SET_BIT(CAN->FM1R, local_u32FilterBitPos);
	}

	/* Filter FIFO assignment */
	if (pFilterConfig->FilterFIFOAssignment == CAN_FILTER_FIFO0)
	{
	  /* FIFO 0 assignation for the filter */
	  CLR_BIT(CAN->FFA1R, local_u32FilterBitPos);
	}
	else
	{
	  /* FIFO 1 assignation for the filter */
	  SET_BIT(CAN->FFA1R, local_u32FilterBitPos);
	}

	/* Filter activation */
	if (pFilterConfig->FilterActivation == ENABLE)
	{
	  SET_BIT(CAN->FA1R, local_u32FilterBitPos);
	}

	/* Leave the initialisation mode for the filter */
	CLR_BIT(CAN->FMR, 0);
}

u32 CAN_voidGetErrorCode(void)
{
	return CAN_ErrorCode;
}


void CAN_voidEnableInterrupt(u32 local_u32ActiveITs)
{
	CAN->IER |= local_u32ActiveITs;
}

void CAN_voidDisableInterrupt(u32 local_u32ActiveITs)
{
	CAN->IER &= ~ local_u32ActiveITs;
}





