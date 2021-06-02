/************************************************************************************************************************************
 * File Name: LiPo_Charger_Main.c
 *
 * Original Creator: Jonathan Ciulla, Luis Camargo
 *
 * Date Created: 1/12/21
 *
 * Person Last Edited: Luis Camargo
 *
 * Version: 1.5
 *
 * Description: Program prompts user to either bypass or enable input fault conditions for the temp limit (40C-45C) and
 *              timer limit (5-60 minutes) in 5 degree intervals. Safety Features ( Continuously Checks for over-current
 *              at 3A and over-under-voltage and faulty cells). Program auto detects the number of cells in the battery
 *              pack based off of cell voltages. Next the battery charger will either enter constant current (Fixed 2.5A
 *              , variable voltage) if all cells are below 4.0V. Once a single cell has a voltage of 4.0V, the charger enters
 *              the constant voltage mode ( Fixed 4.2V, variable current). Once the current reaches 0.1C of the attached
 *              battery pack (0.1 * 1300mA == 130mA), charging is completed.
 *
 *
 * Failure Mode Analysis: Hazards: over and under voltage/current, over-charging .
 *                        Mitigation: Redundancy in the form of hardware (DW01A) and software( loops to detect over/under current
 *                                    form the current sense resistor).
 ***************************************************************************************************************************************/
#include <LiquidCrystal_I2C.h>
#include <msp430.h>
#include <msp430fr6989.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <Wire.h>
#include "Functions.h"
#include "ADC12Init.h"
#include <inttypes.h>

// Define Rotary pins and variables
int RotCLK = 37, RotData = 5, RotSw = 40, counter = 0;

bool faultFlg, faultFlag2, faultFlag3,BattFullCharge;

int Timer, FinalTimer, FinalTimerLoop = 0;

//Raw Cell Voltages in HEX
volatile int Cell0v, Cell1v, Cell2v, Cell3v, Cell4v, Cell5v = 0;
int AvgPackCharge2;
//Reordered Cell Voltages in Ascending Order
int Max1ChargeCell, Max2ChargeCell, Max3ChargeCell, Max4ChargeCell,
        Max5ChargeCell, Max6ChargeCell = 0;

float Cell0TrueVoltage, Cell1TrueVoltage, Cell2TrueVoltage, Cell3TrueVoltage,
        Cell4TrueVoltage, Cell5TrueVoltage = 0, AvgPackCharge = 0;

float AdjustedCC = 0, AdjustedCV = 0, ChargeTimer = 0, Time = 0;
float A, b, F, BattCurrent, x, y;
float Cell0TV, Cell1TV, Cell2TV, Cell3TV, Cell4TV, Cell5TV;
float RealTemp1, RealTemp2, AvgTemp;

int DecVal = 0, TempLimit = 40, TimeLimit = 5, TimeLimit2 =
        0, startup, halt = 0, count = 0, SetConditions = 0, k, loop1 = 1,
        testval2;

unsigned long testVal5, sec, min, hr;

int CellVoltageArray[6], CellArr[6], CellVOrder[6], hexArr[4];

unsigned int decValue = 0;

void HexArrToDec(int hex[]);
void TrueCellVoltage(int ADC12MEM1, int ADC12MEM2, int ADC12MEM3, int ADC12MEM4,
                     int ADC12MEM5, int ADC12MEM6);
void CellCurrent(int a);
void CellVoltageTrace(int CellVoltageArray1[], int n, int CellArr[]);
void loop();

void timer_setup();
void TrueTemp(int a, int b);

LiquidCrystal_I2C lcd(0x27, 20, 4); // set the LCD address to 0x20 for a 20 chars and 4 line display

void setup()
{

    pinMode(RotCLK, INPUT_PULLUP);
    pinMode(RotData, INPUT_PULLUP);
    pinMode(RotSw, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(RotCLK), Rot_CLK, FALLING);
    attachInterrupt(digitalPinToInterrupt(RotData), Rot_Data, FALLING);

    lcd.init();                      // initialize the lcd
    lcd.backlight();
    pinMode(RED_LED, OUTPUT);

    TA0CCTL0 = CCIE;
    TA0CCR0 = 32767;
    TA0CTL = TASSEL_1 + MC_1;

    TB0CCTL0 = CCIE;
    TB0CCR0 = 2500;
    TB0CTL = TASSEL_1 + MC_1;

    WDTCTL = WDTPW | WDTHOLD;                 // Stop watchdog timer

    // Disable the GPIO power-on default high-impedance mode to activate
    // previously configured port settings
    PM5CTL0 &= ~LOCKLPM5;
    //Red TimerB
    P1DIR |= BIT0;                          // for LED1
    P1OUT &= ~BIT0;                         // turn off LED1
                                            // Green TimerA
    P9DIR |= BIT7;                          // for LED2
    P9OUT &= ~BIT7;                         // turn off LED1

    P1REN |= BIT2;                         //Enable pullup resistor for button 2
    P1DIR |= BIT2;
    P1OUT |= BIT2;

    P1REN |= BIT1;                         //Enable pullup resistor for button 1
    P1DIR |= BIT1;
    P1OUT |= BIT1;

    /******************************************************************************
     * Initialization of CC & CV Signal Pins (MOSFETs)
     ******************************************************************************/

    P2DIR |= BIT4 | BIT5;    // output direction P2.4 & P2.5 CC & CV
    P2OUT &= ~ BIT4 | BIT5;  //Disable CC & CV pins

}
void loop()
{

    //lcd.setCursor(5, 0);              // (x,y)
    //lcd.print("welcome!!");
    init_ADC();                 //initialize ADC
    CellAutoDetect();           //Autodetect the number of cells connected

    //Does the user want fault conditions enabled
    lcd.setCursor(0, 1);
    lcd.print("Enable Fault Condit");

    while (digitalRead(RotSw) == 1)
    {

        if (counter % 2 == 0) // // Enable User Fault Conditions
        {
            lcd.setCursor(0, 2);
            lcd.print("No ");
            faultFlg = 0;

        }
        else
        {
            lcd.setCursor(0, 2);
            lcd.print("Yes");
            faultFlg = 1;
        }
    }
    while (digitalRead(RotSw) == 0)
        ;

    lcd.clear();
    counter = 0;
    if (faultFlg == 1)
    {

        while (digitalRead(RotSw) == 1)
        {
            //User inputs settable alarms *Timer* *Temperature* *User Defined Thermal Cutoff*
            //Printing Current
            lcd.setCursor(0, 1);
            lcd.print("Temp Limit:");
            lcd.setCursor(11, 1);
            if (counter > 5)
            {
                counter = 5;
            }
            lcd.print(counter + 40);
            lcd.setCursor(13, 1);
            lcd.print(char(0xDF));    //degree symbol
            lcd.setCursor(14, 1);
            lcd.print("C");
        }
        while (digitalRead(RotSw) == 0)
            ;

        TempLimit = counter + 40;
        counter = 0;
        lcd.clear();

        while (digitalRead(RotSw) == 1)
        {

            //User fault condition for timer set
            lcd.setCursor(0, 2);
            lcd.print("Timer Limit:");
            lcd.setCursor(13, 2);
            if (counter > 60)
            {
                counter = 60;
            }
            lcd.print((counter * 5) + 5);

            if (counter == 2)
            {
                lcd.setCursor(14, 2);
                lcd.print("0");
            }
            lcd.setCursor(15, 2);
            lcd.print("min");

        }
        while (digitalRead(RotSw) == 0)
            ;

        TimeLimit = ((counter * 5) + 5) * 60;
        counter = 0;
        lcd.clear();
        while (digitalRead(RotSw) == 1)
        {

            lcd.setCursor(0, 1);
            lcd.print("Temp Limit:");
            lcd.setCursor(13, 1);
            lcd.print(TempLimit);

            lcd.setCursor(0, 2);
            lcd.print("Timer Limit:");
            lcd.setCursor(13, 2);
            lcd.print(TimeLimit / 60);

            lcd.setCursor(9, 3);
            lcd.print("Ready");
        }
        while (digitalRead(RotSw) == 0)
            ;

    }
    else
    {

        lcd.clear();
        while (digitalRead(RotSw) == 1)
        {
            TempLimit = 45;
            TimeLimit = 3600;

            lcd.setCursor(0, 1);
            lcd.print("Temp Limit:");
            lcd.setCursor(13, 1);
            lcd.print(TempLimit);

            lcd.setCursor(0, 2);
            lcd.print("Timer Limit:");
            lcd.setCursor(13, 2);
            lcd.print(TimeLimit / 60);

            lcd.setCursor(9, 4);
            lcd.print("Ready");
        }
        while (digitalRead(RotSw) == 0)
            ;
    }

    //clears the LCD sets cursor to top left of the screen
    lcd.clear();

    Timer = 0;     // Reset Timer Prior to charge phase

    while (1)
    {
        //Overcurrent check if over 3A
        while (BattCurrent < 3)
        {
            if (AvgTemp <= TempLimit)
            {

                if (Timer < TimeLimit)
                {

                    //If battery is not fully charged
                    if (!BattFullCharge )
                    {
                        //Assigns Hex value to respective variable
                        Cell0v = ADC12MEM1;
                        Cell1v = ADC12MEM2;
                        Cell2v = ADC12MEM3;
                        Cell3v = ADC12MEM4;
                        Cell4v = ADC12MEM5;
                        Cell5v = ADC12MEM6;

                        CellAutoDetect();

                        //Updates screen every half second
                        if (testVal < 5)
                        {

                            TrueCellVoltage(ADC12MEM1, ADC12MEM2, ADC12MEM3,
                                            ADC12MEM4, ADC12MEM5, ADC12MEM6);
                            CellAutoDetect();

                            if (faultFlag3) //Condition check for over voltage 4.25V
                            {
                                lcd.setCursor(0, 0);
                                lcd.print("High Voltage! Faulty Cell");
                                lcd.setCursor(0, 1);
                                lcd.print("                    ");
                                lcd.setCursor(0, 2);
                                lcd.print("                    ");
                                lcd.setCursor(0, 3);
                                lcd.print("                    ");

                            }

                            else
                            {

                                switch (NumberOfCells)
                                {

                                case 0:

                                    lcd.setCursor(0, 3);
                                    lcd.print("Connect Battery");
                                    lcd.setCursor(0, 1);
                                    lcd.print("                    ");
                                    lcd.setCursor(0, 2);
                                    lcd.print("                    ");
                                    lcd.setCursor(0, 0);
                                    lcd.print("                    ");

                                    break;

                                case 1:
                                    //Print number of cells
                                    lcd.setCursor(1, 0);
                                    lcd.print("Cells");
                                    lcd.setCursor(0, 0);
                                    lcd.print(NumberOfCells);
                                    //Current
                                    lcd.setCursor(7, 0);
                                    lcd.print("Current");
                                    lcd.setCursor(19, 0);
                                    lcd.print("A");
                                    lcd.setCursor(14, 0);
                                    lcd.print(":");
                                    lcd.setCursor(15, 0);
                                    lcd.print(BattCurrent);
                                    //Cell0
                                    lcd.setCursor(0, 1);
                                    lcd.print("V0:  ");
                                    lcd.setCursor(5, 1);
                                    lcd.print(Cell0TV);
                                    lcd.setCursor(9, 1);
                                    lcd.print("V");
                                    break;
                                case 2:
                                    //Print number of cells
                                    lcd.setCursor(1, 0);
                                    lcd.print("Cells");
                                    lcd.setCursor(0, 0);
                                    lcd.print(NumberOfCells);
                                    //Current
                                    lcd.setCursor(7, 0);
                                    lcd.print("Current");
                                    lcd.setCursor(19, 0);
                                    lcd.print("A");
                                    lcd.setCursor(14, 0);
                                    lcd.print(":");
                                    lcd.setCursor(15, 0);
                                    lcd.print(BattCurrent);
                                    //Cell0
                                    lcd.setCursor(0, 1);
                                    lcd.print("V0:  ");
                                    lcd.setCursor(5, 1);
                                    lcd.print(Cell0TV);
                                    lcd.setCursor(9, 1);
                                    lcd.print("V");
                                    //Cell1
                                    lcd.setCursor(0, 2);
                                    lcd.print("V1: ");
                                    lcd.setCursor(5, 2);
                                    lcd.print(Cell1TV);
                                    lcd.setCursor(9, 2);
                                    lcd.print("V ");
                                    break;
                                case 3:
                                    //Print number of cells
                                    lcd.setCursor(1, 0);
                                    lcd.print("Cells");
                                    lcd.setCursor(0, 0);
                                    lcd.print(NumberOfCells);
                                    //Current
                                    lcd.setCursor(7, 0);
                                    lcd.print("Current");
                                    lcd.setCursor(19, 0);
                                    lcd.print("A");
                                    lcd.setCursor(14, 0);
                                    lcd.print(":");
                                    lcd.setCursor(15, 0);
                                    lcd.print(BattCurrent);
                                    //Cell0
                                    lcd.setCursor(0, 1);
                                    lcd.print("V0:  ");
                                    lcd.setCursor(5, 1);
                                    lcd.print(Cell0TV);
                                    lcd.setCursor(9, 1);
                                    lcd.print("V");
                                    //Cell1
                                    lcd.setCursor(0, 2);
                                    lcd.print("V1: ");
                                    lcd.setCursor(5, 2);
                                    lcd.print(Cell1TV);
                                    lcd.setCursor(9, 2);
                                    lcd.print("V ");
                                    //Cell2
                                    lcd.setCursor(0, 3);
                                    lcd.print("V2:  ");
                                    lcd.setCursor(5, 3);
                                    lcd.print(Cell2TV);
                                    lcd.setCursor(9, 3);
                                    lcd.print("V");
                                    break;
                                case 4:
                                    //Print number of cells
                                    lcd.setCursor(1, 0);
                                    lcd.print("Cells");
                                    lcd.setCursor(0, 0);
                                    lcd.print(NumberOfCells);
                                    //Current
                                    lcd.setCursor(7, 0);
                                    lcd.print("Current");
                                    lcd.setCursor(19, 0);
                                    lcd.print("A");
                                    lcd.setCursor(14, 0);
                                    lcd.print(":");
                                    lcd.setCursor(15, 0);
                                    lcd.print(BattCurrent);
                                    //Cell0
                                    lcd.setCursor(0, 1);
                                    lcd.print("V0:  ");
                                    lcd.setCursor(5, 1);
                                    lcd.print(Cell0TV);
                                    lcd.setCursor(9, 1);
                                    lcd.print("V");
                                    //Cell1
                                    lcd.setCursor(0, 2);
                                    lcd.print("V1: ");
                                    lcd.setCursor(5, 2);
                                    lcd.print(Cell1TV);
                                    lcd.setCursor(9, 2);
                                    lcd.print("V ");
                                    //Cell2
                                    lcd.setCursor(0, 3);
                                    lcd.print("V2:  ");
                                    lcd.setCursor(5, 3);
                                    lcd.print(Cell2TV);
                                    lcd.setCursor(9, 3);
                                    lcd.print("V");
                                    //Cell3
                                    lcd.setCursor(10, 1);
                                    lcd.print(" V3: ");
                                    lcd.setCursor(15, 1);
                                    lcd.print(Cell3TV);
                                    lcd.setCursor(19, 1);
                                    lcd.print("V");
                                    break;
                                case 5:
                                    //Print number of cells
                                    lcd.setCursor(1, 0);
                                    lcd.print("Cells");
                                    lcd.setCursor(0, 0);
                                    lcd.print(NumberOfCells);
                                    //Current
                                    lcd.setCursor(7, 0);
                                    lcd.print("Current");
                                    lcd.setCursor(19, 0);
                                    lcd.print("A");
                                    lcd.setCursor(14, 0);
                                    lcd.print(":");
                                    lcd.setCursor(15, 0);
                                    lcd.print(BattCurrent);
                                    //Cell0
                                    lcd.setCursor(0, 1);
                                    lcd.print("V0:  ");
                                    lcd.setCursor(5, 1);
                                    lcd.print(Cell0TV);
                                    lcd.setCursor(9, 1);
                                    lcd.print("V");
                                    //Cell1
                                    lcd.setCursor(0, 2);
                                    lcd.print("V1: ");
                                    lcd.setCursor(5, 2);
                                    lcd.print(Cell1TV);
                                    lcd.setCursor(9, 2);
                                    lcd.print("V ");
                                    //Cell2
                                    lcd.setCursor(0, 3);
                                    lcd.print("V2:  ");
                                    lcd.setCursor(5, 3);
                                    lcd.print(Cell2TV);
                                    lcd.setCursor(9, 3);
                                    lcd.print("V");
                                    //Cell3
                                    lcd.setCursor(10, 1);
                                    lcd.print(" V3: ");
                                    lcd.setCursor(15, 1);
                                    lcd.print(Cell3TV);
                                    lcd.setCursor(19, 1);
                                    lcd.print("V");
                                    //Cell4
                                    lcd.setCursor(11, 2);
                                    lcd.print("V4: ");
                                    lcd.setCursor(15, 2);
                                    lcd.print(Cell4TV);
                                    lcd.setCursor(19, 2);
                                    lcd.print("V");
                                    break;
                                case 6:
                                    //Print number of cells
                                    lcd.setCursor(1, 0);
                                    lcd.print("Cells");
                                    lcd.setCursor(0, 0);
                                    lcd.print(NumberOfCells);
                                    //Current
                                    lcd.setCursor(7, 0);
                                    lcd.print("Current");
                                    lcd.setCursor(19, 0);
                                    lcd.print("A");
                                    lcd.setCursor(14, 0);
                                    lcd.print(":");
                                    lcd.setCursor(15, 0);
                                    lcd.print(BattCurrent);
                                    //Cell0
                                    lcd.setCursor(0, 1);
                                    lcd.print("V0:  ");
                                    lcd.setCursor(5, 1);
                                    lcd.print(Cell0TV);
                                    lcd.setCursor(9, 1);
                                    lcd.print("V");
                                    //Cell1
                                    lcd.setCursor(0, 2);
                                    lcd.print("V1: ");
                                    lcd.setCursor(5, 2);
                                    lcd.print(Cell1TV);
                                    lcd.setCursor(9, 2);
                                    lcd.print("V ");
                                    //Cell2
                                    lcd.setCursor(0, 3);
                                    lcd.print("V2:  ");
                                    lcd.setCursor(5, 3);
                                    lcd.print(Cell2TV);
                                    lcd.setCursor(9, 3);
                                    lcd.print("V");
                                    //Cell3
                                    lcd.setCursor(10, 1);
                                    lcd.print(" V3: ");
                                    lcd.setCursor(15, 1);
                                    lcd.print(Cell3TV);
                                    lcd.setCursor(19, 1);
                                    lcd.print("V");
                                    //Cell4
                                    lcd.setCursor(11, 2);
                                    lcd.print("V4: ");
                                    lcd.setCursor(15, 2);
                                    lcd.print(Cell4TV);
                                    lcd.setCursor(19, 2);
                                    lcd.print("V");
                                    //Cell5
                                    lcd.setCursor(11, 3);
                                    lcd.print("V5: ");
                                    lcd.setCursor(15, 3);
                                    lcd.print(Cell5TV);
                                    lcd.setCursor(19, 3);
                                    lcd.print("V");
                                    break;
                                default:
                                    lcd.setCursor(19, 3);
                                    lcd.print("x");
                                }
                            }
                        }

                        if (testVal == 5)
                        {
                            lcd.setCursor(0, 1);
                            lcd.print("                    ");
                            lcd.setCursor(0, 2);
                            lcd.print("                    ");
                            lcd.setCursor(0, 3);
                            lcd.print("                    ");
                        }
                        if (NumberOfCells != 0)
                        {
                            if ((testVal >= 6) && (testVal < 12))
                            {
                                lcd.setCursor(5, 1);
                                lcd.print("Time:");
                                lcd.setCursor(11, 1);
                                lcd.print(hr);
                                lcd.setCursor(12, 1);
                                lcd.print(":");
                                if (min < 10)
                                {
                                    lcd.setCursor(14, 1);
                                    lcd.print(min);
                                }
                                else
                                {
                                    lcd.setCursor(13, 1);
                                    lcd.print(min);
                                }
                                lcd.setCursor(15, 1);
                                lcd.print(":");
                                lcd.setCursor(16, 1);
                                lcd.print(sec);

                                lcd.setCursor(0, 2);
                                lcd.print("Pack Temp:");
                                lcd.setCursor(16, 2);
                                lcd.print(char(0xDF));  //degree symbol
                                lcd.setCursor(17, 2);
                                lcd.print("C");
                                TrueTemp(ADC12MEM8, ADC12MEM9);
                                lcd.setCursor(11, 2);
                                lcd.print(AvgTemp);
                                TrueCellVoltage(ADC12MEM1, ADC12MEM2, ADC12MEM3,
                                                ADC12MEM4, ADC12MEM5,
                                                ADC12MEM6);
                                AvgPackCharge = (Cell0TV + Cell1TV + Cell2TV
                                        + Cell3TV + Cell4TV + Cell5TV) / 6;
                                lcd.setCursor(0, 3);
                                lcd.print("Average V:");
                                lcd.setCursor(11, 3);
                                lcd.print(AvgPackCharge);
                                lcd.setCursor(15, 3);
                                lcd.print("V");

                            }
                        }
                        // P9OUT ^= BIT7;

                        if (!BattFullCharge )         // Need it
                        {

                            //P9OUT ^= BIT7;
                            //Loading cell array with hex values of each cell
                            CellVoltageArray[0] = Cell0v;
                            CellVoltageArray[1] = Cell1v;
                            CellVoltageArray[2] = Cell2v;
                            CellVoltageArray[3] = Cell3v;
                            CellVoltageArray[4] = Cell4v;
                            CellVoltageArray[5] = Cell5v;

                            //Raw hex array of each respective cell voltage to
                            //pass to CellVoltageTrace function
                            CellArr[0] = Cell0v;
                            CellArr[1] = Cell1v;
                            CellArr[2] = Cell2v;
                            CellArr[3] = Cell3v;
                            CellArr[4] = Cell4v;
                            CellArr[5] = Cell5v;

                            //Function to determine the order of highest to lowest cell charger
                            selectionSort(CellVoltageArray, 6);

                            // If needed Max1 = highest voltage cell ... Max6 = lowest voltage cell
                            Max1ChargeCell = CellVoltageArray[0];
                            Max2ChargeCell = CellVoltageArray[1];
                            Max3ChargeCell = CellVoltageArray[2];
                            Max4ChargeCell = CellVoltageArray[3];
                            Max5ChargeCell = CellVoltageArray[4];
                            Max6ChargeCell = CellVoltageArray[5];

                            // Function to trace which cell has the which voltage
                            CellVoltageTrace(CellVoltageArray, 6, CellArr);
                            AvgPackCharge =  (ADC12MEM1 + ADC12MEM2+ ADC12MEM3+ ADC12MEM4 + ADC12MEM5 + ADC12MEM6 )/6;
                            // ADC12CTL0 = ADC12CTL0 | ADC12SC; // Start conversion

                            // If any of the cells reach 2.0V == 0x9B2 (raw input 4.0V), Start Constant Voltage
                                                //3.90V                     //4.0V
                            if((AvgPackCharge > 0x973) && (AvgPackCharge < 0x9B2))
                            {
                                P2OUT &= ~BIT4; //disable Constant Curr
                                // enable the constant voltage MOSFET P2.5
                                P2OUT |= BIT5;
                                P9OUT |= BIT7;// gree led flash when on
                                P1OUT &= ~BIT0;
                            }
                            if(AvgPackCharge < 0x973) //3.90v
                            {
                               P2OUT &= ~BIT5; // disable the constant voltage MOSFET P2.5
                               P2OUT |= BIT4; //enable Constant Curr
                               P1OUT |= BIT0;
                               P9OUT &= ~BIT7;// red led flash when on

                            }
                            // Else Enter Constant Current
                            if (AvgPackCharge > 0x9B2) //4.0
                            {

                                P2OUT &= ~ BIT4 | BIT5; //disable Constant Current/Voltage MOSFETs P2.4 P2.5
                                BattFullCharge = 1;

                            }

                        }
                        FinalTimer = Timer;

                    }

                    while(BattFullCharge )
                    {

                        while (FinalTimerLoop < 1)
                        {
                            sec = FinalTimer;
                            min = sec / 60;
                            hr = min / 60;
                            sec %= 60;
                            min %= 60;
                            //Print to screen Battery Fully charged
                            lcd.setCursor(0, 1);
                            lcd.print("Charging Completed");
                            lcd.setCursor(0, 2);
                            lcd.print("Disconnect Battery");
                            lcd.setCursor(0, 3);
                            lcd.print("                   ");
                            lcd.setCursor(5, 3);
                            lcd.print(hr);
                            lcd.setCursor(6, 3);
                            lcd.print(":");
                            if (min < 10)
                            {
                                lcd.setCursor(8, 3);
                                lcd.print(min);
                            }
                            else
                            {
                                lcd.setCursor(7, 3);
                                lcd.print(min);
                            }
                            lcd.setCursor(9, 3);
                            lcd.print(":");
                            lcd.setCursor(10, 3);
                            lcd.print(sec);
                            FinalTimerLoop++;

                        }
                    }

                }
                else
                {
                    //Print to screen the TempLimit Reached
                    lcd.setCursor(1, 2);
                    lcd.print("Time Limit Reached");
                    lcd.setCursor(1, 3);
                    lcd.print("CC & CV Disabled");

                    P2OUT &= ~ BIT4 | BIT5;  // disable the CC & CV

                }

            }
            else
            {
                //Print to screen the TempLimit Reached
                lcd.setCursor(1, 2);
                lcd.print("Temp Limit Reached");
                lcd.setCursor(1, 3);
                lcd.print("CC & CV Disabled");

                P2OUT &= ~ BIT4 | BIT5;  // disable the CC & CV

            }
        }
        P2OUT &= ~ BIT4 | BIT5;  //Disable CC & CV
        lcd.setCursor(0, 1);
        lcd.print("Over Current Fault");
        lcd.setCursor(0, 2);
        lcd.print("Disconnect Battery");
    }

}

/******************************************************************************
 // Function to trace which cell has the which voltage
 // CellVOrder[0] --> cell0
 // CellVOrder[1] --> cell1
 // CellVOrder[2] --> cell2
 // CellVOrder[3] --> cell3
 // CellVOrder[4] --> cell4
 // CellVOrder[5] --> cell5
 **************************************************************************** */
void CellVoltageTrace(int CellVoltageArray1[], int n, int CellArr[])
{
    int i, j;
    for (i = 0; i < n; i++)
    {
        for (j = 0; j < n + 1; j++)
        {
            if (CellVoltageArray1[j] == CellArr[i])
            {
                CellVOrder[i] = j;
            }
        }
    }
}

void TrueTemp(int a, int b)
{
    float temp = 33.07; // 29.045Divided the thermocouple ADC decimal value (639) by a
//known degree Celsius (22) ... 639/22 = 29.045
    RealTemp1 = a / temp; //Divide each thermocouple ADC value by known step size
    RealTemp2 = b / temp; //Divide each thermocouple ADC value by known step size

    AvgTemp = (RealTemp1 + RealTemp2) / (2); //Average of the two thermocouples

}

void TrueCellVoltage(int ADC12MEM1, int ADC12MEM2, int ADC12MEM3, int ADC12MEM4,
                     int ADC12MEM5, int ADC12MEM6)
{
    int c = 2;
    A = 1240.69; // reciprocal of 12-Bit ADC step size (.000806)^-1 = 1240.69
    Cell0TV = (c * ADC12MEM1) / A; //Generalized formula

    Cell1TV = (c * ADC12MEM2) / A;

    Cell2TV = (c * ADC12MEM3) / A;

    Cell3TV = (c * ADC12MEM4) / A;

    Cell4TV = (c * ADC12MEM5) / A;

    Cell5TV = (c * ADC12MEM6) / A;

// fault condition for if any cell below 3.0V
    if ((ADC12MEM1 < 0x745) || (ADC12MEM2 < 0x745) || (ADC12MEM3 < 0x745)
            || (ADC12MEM4 < 0x745) || (ADC12MEM5 < 0x745)
            || (ADC12MEM6 < 0x745))
    {
        faultFlag2 = 1;
    }
    else
    {
        faultFlag2 = 0;
    }


// Overvoltage Protection  4.25V == 0xA4C
    if ((ADC12MEM1 > 0xA4C) || (ADC12MEM2 > 0xA4C) || (ADC12MEM3 > 0xA4C)
            || (ADC12MEM4 > 0xA4C) || (ADC12MEM5 > 0xA4C)
            || (ADC12MEM6 > 0xA4C))
    {
        faultFlag3 = 1;
    }
    else
        faultFlag3 = 0;
}

void CellCurrent(int a)
{
    A = 1240.69; // reciprocal of 12-Bit ADC step size (.000806)^-1 = 1240.69
    F = a / A;
    BattCurrent = F * 2; // Multiply the voltage across the .005 Ohm by the inverse of the resistor
}

__attribute__((interrupt(TIMER0_A0_VECTOR)))
void myTimer_A(void)
{

//Check battery Temperature Thermocouple 1 and 2
//If battery temperature is too high
    if ((ADC12MEM8 >= 0x4FE) || (ADC12MEM9 >= 0x4FE)) //Change the value of according to thermocouple testing
    {                                               // 4FE = 45 C
        P2OUT &= ~BIT4 | BIT5;  //Disables the CV & CC MOSFETs
    }

    if (BattCurrent <= .130)
    {
//Battery has reached full charge
        P2OUT &= ~ BIT4 | BIT5; //disable Constant Current/Voltage MOSFETs P2.4 P2.5
//BattFullCharge = 1;
    }

    ADC12CTL0 = ADC12CTL0 | ADC12SC;

    testVal++;
    if (testVal > 11)
    {
        testVal = 0;
    }

//Increment the timer by 1seconds
    Timer++;

    sec = Timer;
    min = sec / 60;
    hr = min / 60;
    sec %= 60;
    min %= 60;

}

void Rot_CLK(void)
{
    static unsigned long refresh;
    if (testVal5 - refresh > 5)
    {
        if (digitalRead(RotData) == 1)
        {
            counter++;
        }
        refresh = testVal5;
    }
}

void Rot_Data(void)
{
    static unsigned long refresh;
    if (testVal5 - refresh > 5)
    {
        if (digitalRead(RotCLK) == 1)
        {
            if (counter > 0)
            {
                counter--;
            }
        }

        refresh = testVal5;
    }
}
__attribute__((interrupt(TIMER0_B0_VECTOR)))
void TimerX(void)
{
    testVal5++;

}

