/* 
 * PLL oscillator for PIC12F1501
 *   use ADI(MAXIM) MAX2870 PLL-IC
 *
 *  Copyright 2023 JK1MLY:Hidekazu Inaba
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, 
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, 
 * this list of conditions and the following disclaimer in the documentation and/or 
 * other materials provided with the distribution.
*/

#include <stdio.h>
#include <stdint.h>
#include <xc.h>
#include <pic.h>

#define _XTAL_FREQ 4000000
// CONFIG1
#pragma config FOSC = INTOSC    // Oscillator Selection
#pragma config WDTE = OFF       // Watchdog Timer
#pragma config PWRTE = OFF      // Power-up Timer
#pragma config MCLRE = OFF      // MCLR Pin Function Select
#pragma config CP = OFF         // Flash Program Memory Code Protection
#pragma config BOREN = ON       // Brown-out Reset Enable
#pragma config CLKOUTEN = OFF   // Clock Out Enable
// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection
// #pragma config PLLEN = OFF      // 4x PLL OFF
#pragma config STVREN = OFF     // Stack Overflow/Underflow Reset Enable
#pragma config BORV = LO        // Brown-out Reset Voltage Selection
// #pragma config LPBOREN = OFF    // Low Power Brown-out Reset Enable
#pragma config LVP = OFF        // Low-Voltage Programming Enable

#define	SPI_DAT_LOW     LATA0 = 0   // 7pin
#define SPI_DAT_HIGH	LATA0 = 1
#define	SPI_CLK_LOW     LATA1 = 0   // 6pin
#define	SPI_CLK_HIGH	LATA1 = 1
#define	SPI_LE_LOW      LATA2 = 0   // 5pin
#define	SPI_LE_HIGH     LATA2 = 1
#define	SPI_CS_LOW      LATA4 = 0   // 3pin
#define	SPI_CS_HIGH     LATA4 = 1

#define SW_CONF         RA5         // 2pin
#define SW_ON           0
#define SW_OFF          1

#define false           0
#define true            1

/*
 * MAX2870
 * https://www.analog.com/jp/products/max2870.html
 * 
 * datasheet
 * https://www.analog.com/media/en/technical-documentation/data-sheets/MAX2870.pdf
 * EV kit S/W (register calculation)
 * https://www.analog.com/ja/design-center/evaluation-hardware-and-software/software/software-download?swpart=SFW0004870A
 * 
 * example for a transverter
 */

// 700M for TEST
#define REG0        0x80700000
#define REG1        0x200303E9
#define REG2        0x00010042
#define REG3        0x00000133
#define REG4        0x61BF42FC
#define REG5        0x01400005

// INT 1150M + 145M = 1295M
#define REG0_INT    0x805C0000
#define REG1_INT    0x800303E9
#define REG2_INT    0x00010A42
#define REG3_INT    0x00000133
#define REG4_INT    0x61AF42FC
#define REG5_INT    0x01400005

// 1150M + 145M = 1295M
#define REG0_1G2    0x005C0000
#define REG1_1G2    0x200303E9
#define REG2_1G2    0x00010A42
#define REG3_1G2    0x00000133
#define REG4_1G2    0x61AF42FC
#define REG5_1G2    0x01400005

// 1995M + 432M = 2427M
#define REG0_2G4    0x004F8258
#define REG1_2G4    0x200303E9
#define REG2_2G4    0x00010C42
#define REG3_2G4    0x00000133
#define REG4_2G4    0x619F42FC
#define REG5_2G4    0x01400005

// 5328M + 432M = 5760M
#define REG0_5G6    0x006A8078
#define REG1_5G6    0x200303E9
#define REG2_5G6    0x00010E42
#define REG3_5G6    0x00000133
#define REG4_5G6    0x618F42FC
#define REG5_5G6    0x01400005


void port_init(void) {
    /* CONFIGURE GPIO */ 
    OSCCON  = 0b01101000;    //4MHz
    TRISA   = 0b00101000;    //Input(1)
    OPTION_REG = 0b00000000; //MSB WPUENn
    WPUA    = 0b00101000;    //PupOn(1)
    INTCON  = 0b00000000;
    LATA    = 0b00000111;
    ANSELA  = 0b00000000;
}

void spi_byte(uint8_t data)
{
	for(uint8_t i = 0; i < 8; i++){
		if ( (data & 0x80) == 0  ){
			SPI_DAT_LOW;
		}else{
			SPI_DAT_HIGH;
		}
		
		data = (uint8_t)(data << 1) ;
		__delay_us(2);
		SPI_CLK_HIGH;
		__delay_us(4);
		SPI_CLK_LOW;
		__delay_us(2);
	}
}

void spi_snd(uint32_t reg)
{
    uint8_t data;
	data = (uint8_t)((reg >> 24) & 0xFF );
    spi_byte(data);	
	data = (uint8_t)((reg >> 16) & 0xFF );
    spi_byte(data);	
	data = (uint8_t)((reg >>  8) & 0xFF );
    spi_byte(data);	
	data = (uint8_t)((reg      ) & 0xFF );
    spi_byte(data);	
	__delay_us(2);
	SPI_LE_HIGH;
	__delay_us(10);
	SPI_LE_LOW;
	__delay_us(2);
}

void spi_open(void){
	SPI_CS_HIGH;
	__delay_us(2);
	SPI_LE_HIGH;
	__delay_us(10);
	SPI_DAT_LOW;
	__delay_us(2);
	SPI_CLK_LOW;
	__delay_us(2);
	SPI_CS_LOW;
	__delay_us(10);    
	SPI_LE_LOW;
	__delay_us(2);    
}

void spi_close(void){
	SPI_CLK_LOW;
	__delay_us(2);
	SPI_DAT_LOW;
	__delay_us(10);
	SPI_CS_HIGH;
	__delay_us(10);
}

void pll_700(void)
{
    spi_open();
    spi_snd((uint32_t)REG5);
	__delay_ms(20);
    spi_snd((uint32_t)REG4);
    spi_snd((uint32_t)REG3);
    spi_snd((uint32_t)REG2);
    spi_snd((uint32_t)REG1);
    spi_snd((uint32_t)REG0);
    spi_close();
	__delay_ms(10);
}

void pll_1G2(void)
{
    spi_open();
    spi_snd((uint32_t)REG5_1G2);
	__delay_ms(20);
    spi_snd((uint32_t)REG4_1G2);
    spi_snd((uint32_t)REG3_1G2);
    spi_snd((uint32_t)REG2_1G2);
    spi_snd((uint32_t)REG1_1G2);
    spi_snd((uint32_t)REG0_1G2);
    spi_close();
	__delay_ms(10);
}

void pll_2G4(void)
{
    spi_open();
    spi_snd((uint32_t)REG5_2G4);
	__delay_ms(20);
    spi_snd((uint32_t)REG4_2G4);
    spi_snd((uint32_t)REG3_2G4);
    spi_snd((uint32_t)REG2_2G4);
    spi_snd((uint32_t)REG1_2G4);
    spi_snd((uint32_t)REG0_2G4);
    spi_close();
	__delay_ms(10);
}

void pll_5G6(void)
{
    spi_open();
    spi_snd((uint32_t)REG5_5G6);
	__delay_ms(20);
    spi_snd((uint32_t)REG4_5G6);
    spi_snd((uint32_t)REG3_5G6);
    spi_snd((uint32_t)REG2_5G6);
    spi_snd((uint32_t)REG1_5G6);
    spi_snd((uint32_t)REG0_5G6);
    spi_close();
	__delay_ms(10);
}

void main(void) {

//Initialize
	port_init();

    __delay_ms(1000);        
    pll_700();
    __delay_ms(10000);        
    
//Loop    
    while(1){
        pll_1G2();
        __delay_ms(10000);        
        pll_2G4();
        __delay_ms(10000);        
        pll_5G6();
        __delay_ms(10000);        
    }
}

