/* $Id: ms3_user.c,v 1.7 2012/05/25 14:13:46 jsmcortina Exp $
 * Copyright 2007, 2008, 2009, 2010, 2011, 2012 James Murray and Kenneth Culver
 *
 * This file is a part of Megasquirt-3.
 *
 * You should have received a copy of the code LICENSE along with this source, please
 * ask on the www.msextra.com forum if you did not.
 *
*/

#include "ms3.h"

void user_defined()
{
/* 'user defined'
 *
 * So here is a place to put your code. The three variables are there ready
 * for you.
 * If you want to get data out to tuning software you can use outpc.user0 which is set
 * aside specially  (or outpc.status4 or outpc.istatus5 which are both free at time of
 * writing this note.)
 * Then those gauges can be enabled in Tunerstudio.
 *
 * Other uses:
 * Make custom comparisons to turn outputs on/off. One way to simplify this somewhat
 * could be to use this code to change the value in status4 and then let the
 * existing "outputs" code that you configure in the tuning software actually
 * enable the output and turn it on or off - that might save a lot of customisation
 * and digging around in the code. Once you have status4 changing value, you are
 * nearly done.
 */

    RPAGE = tables[8].rpg;

    if (ram_window.pg8.user_conf & 0x01) {        // is our user defined feature enabled
        //user_ulong = ????;  variables for your use
        //user_uint = ????;
        //user_uchar = ????;

        // ram4.user_conf   } These are the data 
        // ram4.user_value1 } you can prog to
        // ram4.user_value2 } flash from tuning software

        /*  if (user_uchar > 4) {
           outpc.user0++;
           } else {
           outpc.user0--;
           }
         */
    }

    return;
}
