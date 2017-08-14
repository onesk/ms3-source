/* $Id: sigs.c,v 1.1275.4.64 2015/07/09 19:27:17 jsmcortina Exp $
 * Copyright 2007, 2008, 2009, 2010, 2011, 2012 James Murray and Kenneth Culver
 *
 * This file is a part of Megasquirt-3.
 *
 * You should have received a copy of the code LICENSE along with this source, please
 * ask on the www.msextra.com forum if you did not.
 *
*/
#include "ms3.h"
const char TEXT_ATTR RevNum[SIZE_OF_REVNUM] = {     // revision no:
// only change for major rev and/or interface change.
// Change the numeric prefix if the raw data becomes incompatible.
// Change the dotted suffix if the ini changes, but data remains compatible
#ifdef MS3PRO
    "MS3 Format 0523.15P"
#else
    "MS3 Format 0523.15 "
#endif
  //"1234567890123456789"
}, Signature[SIZE_OF_SIGNATURE] = {            // program title.
    // Change this every time you tweak a feature.
    "MS3 1.4.0 release     20150709 19:51BST (c) JSM/KC ********"
  //"12345678901234567890123456789012345678901234567890123456789"
};
