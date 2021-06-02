/************************************************************************************************************************************
 * File Name: ADCInit.h
 *
 * Original Creator: Jonathan Ciulla, Luis Camargo
 *
 * Date Created: 1/21/21
 *
 * Person Last Edited: Jonathan Ciulla
 *
 * Version: 2.1
 *
 * Description: This header file provides the function for to initialize the 12-BIT ADC.
 *              The ADC12 memories are mapped to the respective cell to analyze the
 *              analog voltage. The conversion cycles through the ADC12MCTL0 to
 *              ADC12MCTL9 and ends to eliminate excess processing.
 *
 *
 * Failure Mode Analysis: Hazards: Incorrect sequencing.
 *                        Mitigation: .
 *
 ***************************************************************************************************************************************/
#ifndef ADCINIT_H_
#define ADCINIT_H_

void init_ADC(void);

/********************************************************************************
 Initializing all ADC12 PORTS and REGISTERS
 ********************************************************************************/

void init_ADC(void)
{

    P1SEL0 |= BIT3;           //Analog input for the battery current multiplexer
    P1SEL1 |= BIT3;

    P8SEL0 |= BIT5;                 //Thermocouple 1 input

    P9SEL0 |= BIT0;                 //Thermocouple 2 input
    P9SEL1 |= BIT0;

    P8SEL0 |= BIT4;                   // P8.4 Analog in
    P8SEL1 |= BIT7 | BIT6;                  // P8.6, 8.7 for expansion analog in

    P9SEL0 |= BIT1 | BIT2 | BIT3 | BIT5; // P9.1,P9.2,P9.3,P9.5 ADC option select
    P9SEL1 |= BIT1 | BIT2 | BIT3 | BIT5;

    ADC12CTL0 &= ~ADC12ENC; // Disable conversion for to setup ADC control registers
    ADC12CTL1 |= ADC12CONSEQ_1;    // Conversion mode selection: single sequence

    ADC12CTL0 |= ADC12SHT0_5 | ADC12SHT1_5 | ADC12ON | ADC12MSC; // Set sampling time: 96 ADC12CLK's times
// Turn on ADC12
// Using the Multiple Sample and Convert

    ADC12CTL1 |= ADC12PDIV__1 | ADC12SHP | ADC12DIV_5 | ADC12SSEL_0; // ADC12 Clock predivider Select: 1
// The SHI signal is used to trigger the sampling timer
// The SHT0 and SHT1 bits in ADC12CTL0 control the interval of the
// sampling timer that defines the SAMPCON sample period.
// ADC12 Clock Divider Select: 5
// ADC12SSELx bits to select a source: internal oscillator ADC12OSC.

    ADC12CTL2 |= ADC12RES_2;                   // ADC Resolution : 12 Bit
    //ADC12IER0 |=
    // BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5 | BIT6 | BIT7 | BIT8 | BIT9;

// Selection of data registers for ADC channels:
    ADC12MCTL0 = ADC12INCH_10;   // (Batt sense) Data from CH7 P8.4  -> ADC12MEM0//9.21861
    ADC12MCTL1 = ADC12INCH_4;       // (Cell 0) Data from CH4 P8.7  -> ADC12MEM1
    ADC12MCTL2 = ADC12INCH_3;       // (Cell 1) Data - CH5 P8.6  -> ADC12MEM2//1.3
    ADC12MCTL3 = ADC12INCH_9;       // (Cell 2) Data from CH9 P9.1  -> ADC12MEM3
    ADC12MCTL4 = ADC12INCH_7;      // (Cell 3) Data from CH10 P9.2 -> ADC12MEM4//8.4
    ADC12MCTL5 = ADC12INCH_11;      // (Cell 4) Data from CH11 P9.3 -> ADC12MEM5
    ADC12MCTL6 = ADC12INCH_13;      // (Cell 5) Data from CH13 P9.5 -> ADC12MEM6
    ADC12MCTL7 = ADC12INCH_3; // (Battery Current Multiplexer) Data from CH3 P1.3 -> ADC12MEM7
    ADC12MCTL8 = ADC12INCH_6; // (Themocouple 1) Data from CH6 P8.5 -> ADC12MEM8
    ADC12MCTL9 = ADC12INCH_8; // (Themocouple 2) Data from CH3 P9.0 -> ADC12MEM9
    ADC12MCTL10 = ADC12EOS;                   // End of ADC Sequencer

    ADC12CTL0 |= ADC12ENC;                   //Enable conversion
}

#endif /* ADCINIT_H_ */
