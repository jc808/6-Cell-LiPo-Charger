/************************************************************************************************************************************
 * File Name: Functions.h
 *
 * Original Creator: Jonathan Ciulla, Luis Camargo
 *
 * Date Created: 1/20/21
 *
 * Person Last Edited: Jonathan Ciulla
 *
 * Version: 2.0
 *
 * Description: Header file for
 *
 * the most of the functions used in the main (LiPo_Charger_Main.c).
 *              CellAutoDectect determines the number of cells in the attached battery pack based
 *              on the analog input voltages to the MSP430. The swap function is a sub-function to
 *              temporarily store and swap a value with another value within the array used in the
 *              selectionsort function. The selectionsort function reads the values of the cell
 *              voltages and reorders the cells in descending order from the cell with the highest
 *              to the lowest voltage.
 *
 *
 * Failure Mode Analysis: Hazards: Incorrect number of cells .
 *                        Mitigation: Set a voltage range to for the system to recognize a cell is
 *                        present( when the cell is at or above 3.0V). If a cell is reading from (2-2.99V)
 *                        , the system determines that there is a faulty cell present.
 *
 ***************************************************************************************************************************************/
#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_
int NumberOfCells = 0, testVal;
void CellAutoDetect(void);
void SettableAlarms(void);
void swap(int *xp, int *yp);
void selectionSort(int CellVoltageArray[], int n);

void CellAutoDetect(void)
{
    /***********************************************************************
     Autodetect Number of Cells Connected
     Inifintie loop if no cell voltage is above 2V
     ***********************************************************************/

    // if any cell voltages are above 2V Pack connected
    if ((ADC12MEM1 > 0x4D9) || (ADC12MEM2 > 0x4D9) || (ADC12MEM3 > 0x4D9)
            || (ADC12MEM4 > 0x4D9) || (ADC12MEM5 > 0x4D9)
            || (ADC12MEM6 > 0x4D9))
    {

        //if any of the cells are between 2V and 3V they are considered DAMAGED
        if (( ADC12MEM1 < 0x745) && (ADC12MEM2 < 0x745) && (ADC12MEM3 < 0x745) &&
           (ADC12MEM4 < 0x745) && (ADC12MEM5 < 0x745) && (ADC12MEM6 < 0x745))
        {

            NumberOfCells = 0;

        }

        //if cell0 is equal to 1.5V (0x745)--> raw cell voltage==3.0V and rest of cells are less than 2.0V
        if ((ADC12MEM1 >= 0x745) && (ADC12MEM2 < 0x4D9) && (ADC12MEM3 < 0x4D9)
                && (ADC12MEM4 < 0x4D9) && (ADC12MEM5 < 0x4D9)
                && (ADC12MEM6 < 0x4D9))
        {
            NumberOfCells = 1;

        }

        //if cell0 and cell1 is greater than or equal to 1.5V (0x745)--> raw cell voltage==3.0V and rest of cells are less than 2.0V
        if ((ADC12MEM1 >= 0x745) && (ADC12MEM2 >= 0x745) && (ADC12MEM3 < 0x4D9)
                && (ADC12MEM4 < 0x4D9) && (ADC12MEM5 < 0x4D9)
                && (ADC12MEM6 < 0x4D9))
        {
            NumberOfCells = 2;

        }

        //if cell0, cell1, and cell2 is greater than or equal to 1.5V (0x745)--> raw cell voltage==3.0V and rest of cells are less than 2.0V
        if ((ADC12MEM1 >= 0x745) && (ADC12MEM2 >= 0x745) && (ADC12MEM3 >= 0x745)
                && (ADC12MEM4 < 0x4D9) && (ADC12MEM5 < 0x4D9)
                && (ADC12MEM6 < 0x4D9))
        {
            NumberOfCells = 3;

        }

        //if cell0, cell1, cell2, and cell3 is greater than or equal to 1.5V (0x745)--> raw cell voltage==3.0V and rest of cells are less than 2.0V
        if ((ADC12MEM1 >= 0x745) && (ADC12MEM2 >= 0x745) && (ADC12MEM3 >= 0x745)
                && (ADC12MEM4 >= 0x745) && (ADC12MEM5 < 0x4D9)
                && (ADC12MEM6 < 0x4D9))
        {
            NumberOfCells = 4;

        }

        //if cell0, cell1, cell2, cell3, cell4, and cell5 is greater than or equal to 1.5V (0x745)--> raw cell voltage==3.0V and rest of cells are less than 2.0V
        if ((ADC12MEM1 >= 0x745) && (ADC12MEM2 >= 0x745) && (ADC12MEM3 >= 0x745)
                && (ADC12MEM4 >= 0x745) && (ADC12MEM5 >= 0x745)
                && (ADC12MEM6 < 0x4D9))
        {
            NumberOfCells = 5;

        }

        //if cell0, cell1, cell2, cell3, cell4, cell5, cell6 is greater than or equal to 1.5V (0x745)--> raw cell voltage==3.0V and rest of cells are less than 2.0V
        if ((ADC12MEM1 >= 0x745) && (ADC12MEM2 >= 0x745) && (ADC12MEM3 >= 0x745)
                && (ADC12MEM4 >= 0x745) && (ADC12MEM5 >= 0x745)
                && (ADC12MEM6 >= 0x745))
        {
            NumberOfCells = 6;

        }



    }

}


/******************************************************************************
 * User inputs settable alarms *Timer* *Temperature* *User Defined Thermal Cutoff
 ******************************************************************************/
void SettableAlarms(void)
{

}


/******************************************************************************
 * Function to determine the order of highest to lowest cell charger
 *****************************************************************************/

void swap(int *xp, int *yp)
{
    int temp = *xp;
    *xp = *yp;
    *yp = temp;
}

void selectionSort(int CellVoltageArray[], int n)
{
    int i, j, min_idx;

    // One by one move boundary of unsorted subarray
    for (i = 0; i < n - 1; i++)
    {

        // Find the minimum element in unsorted array
        min_idx = i;
        for (j = i + 1; j < n; j++)
            if (CellVoltageArray[j] < CellVoltageArray[min_idx])
                min_idx = j;

        // Swap the found minimum element
        // with the first element
        swap(&CellVoltageArray[min_idx], &CellVoltageArray[i]);

    }
}

#endif /* FUNCTIONS_H_ */
