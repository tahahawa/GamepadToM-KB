#include "event_sgnt.h"

class dev_mode {
public:
  bool lt_modifier;
  bool rt_modifier;
  bool la_radial;
  bool ra_radial;

  int curr_mod;
  mode_modifier *curr_modifier;
  mode_modifier no_modifier;
  mode_modifier left_modifier;
  mode_modifier right_modifier;
  mode_modifier both_modifier;

  void updateModifier(int, int);
  dev_mode();
};
