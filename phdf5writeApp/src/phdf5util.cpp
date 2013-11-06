/*
 * writeconfig.cpp
 *
 *  Created on: 11 Jan 2012
 *      Author: up45
 */
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
//#include <stdio.h>
//#include <stdlib.h>

#include <NDArray.h>
#include "dimension.h"
#include "writeconfig.h"

namespace phdf5 {

  /** find out whether or not the input is a prime number.
   * Returns true if number is a prime. False if not.
   */
  bool is_prime(unsigned int long number)
  {
    //0 and 1 are prime numbers
    if(number == 0 || number == 1) return true;

    //find divisor that divides without a remainder
    int divisor;
    for(divisor = (number/2); (number%divisor) != 0; --divisor){;}

    //if no divisor greater than 1 is found, it is a prime number
    return divisor == 1;
  }

} // phdf5

