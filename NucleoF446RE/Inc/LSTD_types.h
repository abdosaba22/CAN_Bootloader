/* ************************************************ */
/* Author      : Abd-alrahman Amin                  */
/* SWC         : Standard Types                     */
/* Layer       : LIB                                */
/* Version     : 1.1                                */
/* Date        : August 2, 2023                     */
/* Last Edit   : August 3, 2023                     */
/* ************************************************ */


#ifndef _LSTD_TYPES_H_
#define _LSTD_TYPES_H_


typedef unsigned char 		u8;
typedef unsigned short int 	u16;
typedef unsigned long int 	u32;
typedef signed char 		s8;
typedef signed short int 	s16;
typedef signed long int 	s32;
typedef float  				f32;
typedef double 				f64;

typedef enum{
	E_OK,
	E_NOK,
	E_NULL_POINTER
}STD_error_t;

#define NULL_POINTER 		((void*)0)


#endif