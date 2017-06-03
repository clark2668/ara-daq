/****************************************************************************/
/* IceCalConsole - a console front end to interact with IceCalAPI           */
/*                                                                          */
/* Author: Jonathan Wonders                                                 */
/* University of Maryland                                                   */
/*                                                                          */
/****************************************************************************/

#include <string>
#include <iostream>
#include "icecalapi.h"

void showMenu(void);

int main(int argc, char** argv)
{
    showMenu();

    char serialPort[] = "/dev/ttyS0";

    if(ICECAL_Open(serialPort) == ICECAL_FAILURE)
        return -1;

    char input;
    std::cin >> input;

    char output[20] = {0};

    switch(input)
    {
    case '1':
        ICECAL_GetSurfaceLineVoltage(output);
        break;
    case '2':
        ICECAL_GetSurfaceProcessorTemperature(output);
        break;
    case '3':
        ICECAL_GetRemoteLineVoltage(output);
        break;
    case '4':
        ICECAL_GetRemoteProcessorTemperature(output);
        break;
    default:
        std::cerr << "Invalid Input" << std::endl;
        break;
    };

    std::cout << output << std::endl;
    ICECAL_Close();

    return 0;
}

void showMenu()
{
    std::cout << "Welcome to the IceCalConsole\n\n"
              << "############## Menu ################\n"
              << "# 1. Surface Line Voltage          #\n"
              << "# 2. Surface Processor Temperture  #\n"
              << "# 3. Remote Line Voltage           #\n"
              << "# 4. Remote Processor Temperture   #\n"
              << "####################################\n"
              << std::endl;
}
