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

void event_sgnt::getSignature(struct input_event &ev) {
  if (ev.type == EV_ABS && (ev.code == ABS_Z || ev.code == ABS_RZ)) {
    if (ev.value > 0) {
      this->set(ev.type, ev.code, 1);
    } else {
      this->set(ev.type, ev.code, 0);
    }
  } else {
    this->set(ev);
  }
}

bool event_sgnt::operator<(const event_sgnt &rhs) const {
  return (type < rhs.type) || (type == rhs.type && code < rhs.code) ||
         (type == rhs.type && code == rhs.code && value < rhs.value);
}
