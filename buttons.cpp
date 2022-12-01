#include "buttons.h"

const unsigned long buttonBitMasks[5][4] = {
  {btn00BitMask, btn01BitMask, btn02BitMask, btn03BitMask},
  {btn10BitMask, btn11BitMask, btn12BitMask, btn13BitMask},
  {btn20BitMask, btn21BitMask, btn22BitMask, btn23BitMask},
  {btn30BitMask, btn31BitMask, btn32BitMask, btn33BitMask},
  {btn40BitMask, btn41BitMask, btn42BitMask, btn43BitMask}
};

const unsigned long buttonShiftLeft[5][4] = {
  {btn00ShiftLeft, btn01ShiftLeft, btn02ShiftLeft, btn03ShiftLeft},
  {btn10ShiftLeft, btn11ShiftLeft, btn12ShiftLeft, btn13ShiftLeft},
  {btn20ShiftLeft, btn21ShiftLeft, btn22ShiftLeft, btn23ShiftLeft},
  {btn30ShiftLeft, btn31ShiftLeft, btn32ShiftLeft, btn33ShiftLeft},
  {btn40ShiftLeft, btn41ShiftLeft, btn42ShiftLeft, btn43ShiftLeft}
};
