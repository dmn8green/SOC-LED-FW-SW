//******************************************************************************
/**
 * @file BasePulseCurve.h
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief BasePulseCurve class definition
 * @version 0.1
 * @date 2023-09-19
 * 
 * @copyright Copyright MN8 (c) 2023
 */
//******************************************************************************
#pragma once

//******************************************************************************
/**
 * @brief Base class for all pulsing curves
 * 
 * This class is used to define the interface for all pulsing curves.  It is used
 * by the PulsingAnimation class to get the the next rate value.
 */
class BasePulseCurve {
public:
    virtual int get_value(int index) = 0;
    virtual int get_length(void) = 0;
};