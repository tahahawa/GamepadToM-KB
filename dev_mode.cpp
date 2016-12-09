#include "mode_modifier.h"

dev_mode::dev_mode() { curr_mod = 0; }

void dev_mode::updateModifier(int LT, int RT) {
  if (lt_modifier && !rt_modifier) {
    if (LT == 1 && curr_mod != 1) {
      curr_mod = 1;
      curr_modifier = &left_modifier;
    } else if (LT == 0 && curr_mod == 1) {
      curr_mod = 0;
      curr_modifier = &no_modifier;
    }
  } else if (!lt_modifier && rt_modifier) {
    if (RT == 1 && curr_mod != 2) {
      curr_mod = 2;
      curr_modifier = &right_modifier;
    } else if (RT == 0 && curr_mod == 2) {
      curr_mod = 0;
      curr_modifier = &no_modifier;
    }
  } else if (lt_modifier && rt_modifier) {
    if (LT == 1 && RT == 1) {
      if (curr_mod == 0) {
        curr_mod = 3;
        curr_modifier = &both_modifier;
      } else if (curr_mod == 1) {
        curr_mod = 3;
        curr_modifier = &both_modifier;
      } else if (curr_mod == 2) {
        curr_mod = 3;
        curr_modifier = &both_modifier;
      }
    } else if (LT == 1) {
      if (curr_mod == 0) {
        curr_mod = 1;
        curr_modifier = &left_modifier;
      } else if (curr_mod == 2) {
        curr_mod = 1;
        curr_modifier = &left_modifier;
      } else if (curr_mod == 3) {
        curr_mod = 1;
        curr_modifier = &left_modifier;
      }
    } else if (RT == 1) {
      if (curr_mod == 0) {
        curr_mod = 2;
        curr_modifier = &right_modifier;
      } else if (curr_mod == 1) {
        curr_mod = 2;
        curr_modifier = &right_modifier;
      } else if (curr_mod == 3) {
        curr_mod = 2;
        curr_modifier = &right_modifier;
      }
    } else {
      curr_mod = 0;
      curr_modifier = &no_modifier;
    }
  }
}
