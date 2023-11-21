# CAN_Bootloader
This Repo includes CAN Bootloader App For STM32F446RE and STM32F103C8T6, the App is Display an message on LCD. 

# SWC : 
1- RCC driver.

2- GPIO driver. 

3- UART driver.

4- CAN driver.

5- SYSTICK driver. 

6- Parser

7- Flash driver.

8- LCD driver.

implemented for both NucleoF446RE and STM32F103 Blue Pill.

# Note:

in this App we have two MCUs the 1st is Nucleo which contain the needed hex file to be flashed on the other MCU which is Blue Pill we transmit the hex over CAN bus record by record then check the record by use Calculate the Check Sum for the recoed if correct then parse it and flash it. else send message to Mcu Nucleo to resend the record again. 
