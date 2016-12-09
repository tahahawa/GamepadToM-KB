#include <libevdev/libevdev.h>
#include <map>
#include <vector>

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
  bool operator<(const event_sgnt &rhs) const;
};

class mode_modifier {
public:
  std::map<event_sgnt, std::vector<input_event>> key_events;
  std::map<event_sgnt, std::vector<input_event>> abs_events;
};
