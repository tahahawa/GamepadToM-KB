#include "compareSig.h"
#include <libevdev/libevdev.h>

class event_sgnt {
public:
  int type;
  int code;
  int value;
  event_sgnt();
  event_sgnt(struct input_event &);
  event_sgnt(int, int, int);
  void set(struct input_event &);
  void set(int, int, int);
};
