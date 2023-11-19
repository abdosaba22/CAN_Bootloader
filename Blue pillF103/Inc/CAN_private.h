/*********************************************************************************/
/* Author    : Abdalrahman Amin                                                  */
/* Version   : V03                                                               */
/* Date      : 30 OCT  2023                                                      */
/*********************************************************************************/
#ifndef CAN_PRIVATE_H
#define CAN_PRIVATE_H

typedef void (*CAN_Interrupt_Callbacks)(void);

/**  CAN Operating Mode **/

#define CAN_MODE_NORMAL           			  ((uint32_t)(0x0))                        			       /*!< Normal mode   */
#define CAN_MODE_LOOPBACK        		 	  ((uint32_t)(0x1<<30))                					   /*!< Loopback mode BIT 30 < 0x40000000 > */
#define CAN_MODE_SILENT            			  ((uint32_t)(0x1<<31))                					   /*!< Silent mode  BIT 31 < 0x80000000 > */
#define CAN_MODE_SILENT_LOOPBACK  			  ((uint32_t)(CAN_MODE_LOOPBACK | CAN_MODE_SILENT)) 	 	   /*!< Loopback combined with silent mode */


/*******************   BoudRate Definition     ***********************/
enum BITRATE{CAN_50KBPS, CAN_100KBPS, CAN_125KBPS, CAN_250KBPS, CAN_500KBPS, CAN_800KBPS, CAN_1MBPS};

typedef const struct
{
	uint8_t TS2;
	uint8_t TS1;
	uint8_t BRP;
} CAN_BitTimingConfig;

/******** BoudRate value rating to table at web site {http://www.bittiming.can-wiki.info/}
 * 1. select ST Microelectronics bxCAN
 * 2. put freq with 36Mhz (APB1 freq)
 * 3. click Request Table						        ***********************************/

//CAN_BitTimingConfig CAN_bitRateConfig[7] = {{2, 13, 45}, {2, 15, 20}, {2, 13, 18}, {2, 13, 9}, {2, 15, 4},{2, 12, 3}, {2, 15, 2}};

CAN_BitTimingConfig CAN_bitRateConfig[7] = {
		{2, 13, 16}, 	/* CAN_50KBPS    */
		{2, 13, 5},	/* CAN_100KBPS   */
		{2, 13, 4},	/* CAN_125KBPS   */
		{2, 13, 2},	/* CAN_250KBPS   */
		{2, 13, 1},	/* CAN_500KBPS   */
		{1, 8,  1},	/* CAN_800KBPS   */
		{1, 6,  1}	/* CAN_1MBPS	 */
};


/*******************   Register Definition     ***********************/

/** Controller Area Network TxMailBox **/

typedef struct
{
  volatile uint32_t TIR;
  volatile uint32_t TDTR;
  volatile uint32_t TDLR;
  volatile uint32_t TDHR;

} CAN_TxMailBox_TypeDef;

/** Controller Area Network FIFOMailBox **/

typedef struct
{
  volatile uint32_t RIR;
  volatile uint32_t RDTR;
  volatile uint32_t RDLR;
  volatile uint32_t RDHR;
} CAN_FIFOMailBox_TypeDef;

/** Controller Area Network FilterRegister **/

typedef struct
{
  volatile uint32_t FR1;
  volatile uint32_t FR2;
} CAN_FilterRegister_TypeDef;

/** Controller Area Network  **/
typedef struct
{
	volatile uint32_t MCR;
	volatile uint32_t MSR;
	volatile uint32_t TSR;
	volatile uint32_t RF0R;
	volatile uint32_t RF1R;
	volatile uint32_t IER;
	volatile uint32_t ESR;
	volatile uint32_t BTR;
	uint32_t RESERVED0[88];
	CAN_TxMailBox_TypeDef TxMailBox[3];
    CAN_FIFOMailBox_TypeDef FIFOMailBox[2];
    uint32_t  RESERVED1[12];
    volatile uint32_t FMR;
    volatile uint32_t FM1R;
    uint32_t  RESERVED2;
    volatile uint32_t FS1R;
    uint32_t  RESERVED3;
    volatile uint32_t FFA1R;
    uint32_t  RESERVED4;
    volatile uint32_t FA1R;
    uint32_t  RESERVED5[8];
    CAN_FilterRegister_TypeDef FilterBankRegister[14];

}CAN_t;

/*******************   BASE Address Definition    *********************/
#define CAN_BASE_ADDRESS	0x40006400 		//define the boundary address of bxCAN1
#define CAN			((volatile CAN_t *) CAN_BASE_ADDRESS)





#endif /* CAN_PRIVATE_H */
