#include "event_sgnt.h"

event_sgnt::event_sgnt() {}
event_sgnt::event_sgnt(struct input_event &ev) {
  type = ev.type;
  code = ev.code;
  value = ev.value;
}
event_sgnt::event_sgnt(int t, int c, int v) {
  type = t;
  code = c;
  value = v;
}

void event_sgnt::set(struct input_event &ev) {
  type = ev.type;
  code = ev.code;
  value = ev.value;
}
void event_sgnt::set(int t, int c, int v) {
  type = t;
  code = c;
  value = v;
}

event_sgnt getSignature(struct input_event ev) {
  event_sgnt ret;
  if (ev.type == EV_ABS && (ev.code == ABS_Z && ev.code <= ABS_RZ)) {
    if (ev.value > 0) {
      ret.set(ev.type, ev.code, 1);
    } else {
      ret.set(ev.type, ev.code, 0);
    }
  } else {
    ret.set(ev);
  }

  return ret;
}
