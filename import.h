
#pragma once
#pragma uccm require(source) += [@inc]/~sudachen/uc_jprint/uc_jprint.c

#include <uccm/board.h>

/*

  API defined in uccm/uccm.h

  it's an backend implementation module only

void ucPutS(const char *text, bool complete);

*/

void ucSetup_jPrint();
