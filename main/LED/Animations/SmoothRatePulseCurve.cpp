//******************************************************************************
/**
 * @file SmoothRatePulseCurve.cpp
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief SmoothRatePulseCurve class implementation
 * @version 0.1
 * @date 2023-09-19
 * 
 * @copyright Copyright MN8 (c) 2023
 */
//******************************************************************************

#include "SmoothRatePulseCurve.h"

// The refresh rates for the smooth pulse curve
SmoothRatePulseCurve::SmoothRatePulseCurve(void) {
    refresh_rates[0]=20;
    refresh_rates[1]=20;
    refresh_rates[2]=20;
    refresh_rates[3]=20;
    refresh_rates[4]=20;
    refresh_rates[5]=20;
    refresh_rates[6]=20;
    refresh_rates[7]=20;
    refresh_rates[8]=20;
    refresh_rates[9]=20;
    refresh_rates[10]=20;
    refresh_rates[11]=20;
    refresh_rates[12]=19;
    refresh_rates[13]=19;
    refresh_rates[14]=19;
    refresh_rates[15]=19;
    refresh_rates[16]=19;
    refresh_rates[17]=19;
    refresh_rates[18]=19;
    refresh_rates[19]=18;
    refresh_rates[20]=18;
    refresh_rates[21]=18;
    refresh_rates[22]=17;
    refresh_rates[23]=17;
    refresh_rates[24]=16;
    refresh_rates[25]=16;
    refresh_rates[26]=15;
    refresh_rates[27]=15;
    refresh_rates[28]=14;
    refresh_rates[29]=14;
    refresh_rates[30]=13;
    refresh_rates[31]=12;
    refresh_rates[32]=11;
    refresh_rates[33]=10;
    refresh_rates[34]=10;
    refresh_rates[35]=9;
    refresh_rates[36]=8;
    refresh_rates[37]=7;
    refresh_rates[38]=6;
    refresh_rates[39]=5;
    refresh_rates[40]=5;
    refresh_rates[41]=4;
    refresh_rates[42]=3;
    refresh_rates[43]=2;
    refresh_rates[44]=2;
    refresh_rates[45]=1;
    refresh_rates[46]=1;
    refresh_rates[47]=0;
    refresh_rates[48]=0;
    refresh_rates[49]=0;
    refresh_rates[50]=0;
    refresh_rates[51]=0;
    refresh_rates[52]=0;
    refresh_rates[53]=0;
    refresh_rates[54]=1;
    refresh_rates[55]=1;
    refresh_rates[56]=2;
    refresh_rates[57]=2;
    refresh_rates[58]=3;
    refresh_rates[59]=4;
    refresh_rates[60]=5;
    refresh_rates[61]=5;
    refresh_rates[62]=6;
    refresh_rates[63]=7;
    refresh_rates[64]=8;
    refresh_rates[65]=9;
    refresh_rates[66]=10;
    refresh_rates[67]=10;
    refresh_rates[68]=11;
    refresh_rates[69]=12;
    refresh_rates[70]=13;
    refresh_rates[71]=14;
    refresh_rates[72]=14;
    refresh_rates[73]=15;
    refresh_rates[74]=15;
    refresh_rates[75]=16;
    refresh_rates[76]=16;
    refresh_rates[77]=17;
    refresh_rates[78]=17;
    refresh_rates[79]=18;
    refresh_rates[80]=18;
    refresh_rates[81]=18;
    refresh_rates[82]=19;
    refresh_rates[83]=19;
    refresh_rates[84]=19;
    refresh_rates[85]=19;
    refresh_rates[86]=19;
    refresh_rates[87]=19;
    refresh_rates[88]=19;
    refresh_rates[89]=20;
    refresh_rates[90]=20;
    refresh_rates[91]=20;
    refresh_rates[92]=20;
    refresh_rates[93]=20;
    refresh_rates[94]=20;
    refresh_rates[95]=20;
    refresh_rates[96]=20;
    refresh_rates[97]=20;
    refresh_rates[98]=20;
    refresh_rates[99]=20;
    refresh_rates[100]=20;
}