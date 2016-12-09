#include "event_sgnt.h"
#include <map>
#include <vector>

class mode_modifier {
public:
  std::map<event_sgnt, std::vector<input_event>, compareSig> key_events;
  std::map<event_sgnt, std::vector<input_event>, compareSig> abs_events;
};
