#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <glib.h>
#include <iostream>
#include <json/json.h>
#include <json/value.h>
#include <libevdev/libevdev.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>

std::vector<std::string> splitCommands(std::string inString) {
  std::stringstream ss;
  std::string buf;
  std::vector<std::string> ret;
  ss.str(inString);
  while (ss >> buf) {
    ret.push_back(buf);
  }
  return ret;
}

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

struct compareSig : public std::binary_function<event_sgnt, event_sgnt, bool> {
  bool operator()(const event_sgnt &lhs, const event_sgnt &rhs) const {
    return (lhs.type < rhs.type) ||
           (lhs.type == rhs.type && lhs.code < rhs.code) ||
           (lhs.type == rhs.type && lhs.code == rhs.code &&
            lhs.value < rhs.value);
  }
};

class input_dev {
private:
  struct libevdev *dev;
  int fd;
  bool validInput(struct input_event *);
  int deadzone[4][2];
  const gchar *fname;
  gchar *gstr;
  GDir *dir;
  int rc;

public:
  input_dev();
  ~input_dev();

  void poll(std::vector<struct input_event> *events);
};

input_dev::input_dev() {
  rc = 1;

  // Detect the first joystick event file
  dir = g_dir_open("/dev/input/by-path/", 0, NULL);
  while ((fname = g_dir_read_name(dir))) {
    if (g_str_has_suffix(fname, "event-joystick"))
      break;
  }
  std::cout << "Opening event file /dev/input/by-path/" << fname << std::endl;
  gstr = g_strconcat("/dev/input/by-path/", fname, NULL);
  fd = open(gstr, O_RDONLY | O_NONBLOCK);
  rc = libevdev_new_from_fd(fd, &dev);
  if (rc < 0) {
    fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-rc));
    exit(1);
  }
  std::cout << "Input device name: " << libevdev_get_name(dev) << std::endl;
  std::cout << "Input device ID: bus " << libevdev_get_id_bustype(dev)
            << " vendor " << libevdev_get_id_vendor(dev) << " product "
            << libevdev_get_id_product(dev) << std::endl;
  std::cout << std::endl << std::endl << std::endl << std::endl;

  deadzone[0][0] = -7000;
  deadzone[0][1] = 7000;
  deadzone[1][0] = -7000;
  deadzone[1][1] = 7000;
  deadzone[2][0] = -7000;
  deadzone[2][1] = 7000;
  deadzone[3][0] = -7000;
  deadzone[3][1] = 7000;
}

input_dev::~input_dev() {
  if (!(fd < 0))
    close(fd);

  delete (fname);
  libevdev_free(dev);
  close(fd);
  g_free(gstr);
  g_dir_close(dir);
  g_free(dir);
}

void input_dev::poll(std::vector<struct input_event> *events) {
  struct input_event ev;
  events->clear();

  int rc = LIBEVDEV_READ_STATUS_SUCCESS;
  if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
    if (ev.type == EV_SYN)
      return;
  }

  if (ev.type == EV_ABS) {
    switch (ev.code) {
    case ABS_X:
    case ABS_Y:
    case ABS_RX:
    case ABS_RY:
      ev.type = 20;
      break;
    default:
      break;
    }
  }
  events->push_back(ev);

  while (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
    memset(&ev, 0, sizeof(struct input_event));
    rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);

    if (rc != LIBEVDEV_READ_STATUS_SUCCESS)
      break;
    if (ev.type == (*events).back().type && ev.code == (*events).back().code &&
        ev.value == (*events).back().value) {
      break;
    }
    if (ev.type == EV_SYN)
      break;
    if (ev.type == EV_ABS) {
      switch (ev.code) {
      case ABS_X:
      case ABS_Y:
      case ABS_RX:
      case ABS_RY:
        ev.type = 20;
        break;
      default:
        break;
      }
    }
    events->push_back(ev);
  }
}

bool input_dev::validInput(struct input_event *ev) {
  bool isValid = false;
  switch (ev->code) {
  case ABS_X:
  case ABS_Y:
    if (ev->value < deadzone[ev->code][0])
      isValid = true;
    else if (ev->value > deadzone[ev->code][1])
      isValid = true;
    break;
  case ABS_RX:
  case ABS_RY:
    if (ev->value < deadzone[ev->code - 1][0])
      isValid = true;
    else if (ev->value > deadzone[ev->code - 1][1])
      isValid = true;
    break;
  default:
    isValid = true;
    break;
  }
  return isValid;
}

class output_dev {
private:
  struct uinput_user_dev uindev;
  int fd;
  void setupAllowedEvents(int *);

public:
  output_dev();
  ~output_dev();
  void send(std::vector<struct input_event>, bool);
};

output_dev::output_dev() {
  fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK | O_SYNC);
  if (fd < 0)
    delete this; // failed to open

  memset(&uindev, 0, sizeof(struct uinput_user_dev));
  snprintf(uindev.name, UINPUT_MAX_NAME_SIZE, "gp2mkb virtual device");
  uindev.id.bustype = BUS_USB;
  uindev.id.vendor = 0;
  uindev.id.product = 0;
  uindev.id.version = 1;
  if (write(fd, &uindev, sizeof(struct uinput_user_dev)) < 0)
    delete this; // write error

  output_dev::setupAllowedEvents(&fd);

  if (ioctl(fd, UI_DEV_CREATE) < 0)
    delete this; // creation error
  usleep(50000);
}

output_dev::~output_dev() {
  if (!(fd < 0))
    close(fd);
}

void output_dev::send(std::vector<struct input_event> seq,
                      bool autoSync = true) {
  // take event as param, parse it, and send it to the fd. idk how

  for (unsigned int i = 0; i < seq.size(); ++i) {
    write(fd, &(seq[i]), sizeof(struct input_event));
  }

  if (autoSync) {
    struct input_event ev;
    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    ev.value = 0;
    write(fd, &ev, sizeof(struct input_event));
  }

  usleep(10000);
  fsync(fd);
}
/* possible useful code block
   usleep(25000);
   memset(&ev, 0, sizeof(struct input_event));
   ev.type = EV_SYN;
   ev.code = SYN_REPORT;
   ev.value = 0;
   write(fd, &ev, sizeof(struct input_event));
   usleep(10000);*/

// A lot of the following code has been adapted from SDL

// Dependencies are glib and libevdev so compile with:
// gcc `pkg-config --cflags --libs libevdev glib-2.0`
// device-input-prototype.c
// amended to: g++ device-input-prototype.cpp -Wall `pkg-config --cflags --libs
// libevdev glib-2.0 jsoncpp`

class mode_modifier {
public:
  std::map<event_sgnt, std::vector<input_event>, compareSig> key_events;
  std::map<event_sgnt, std::vector<input_event>, compareSig> abs_events;
};

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

class virtual_dev {
public:
  int axis_type;
  int mode_code;
  int X, Y, RX, RY, LT, RT;
  bool swap_mode;

  dev_mode *curr_mode;
  std::vector<dev_mode> mode_list;
  virtual_dev();
};

virtual_dev::virtual_dev() {
  X = 0;
  Y = 0;
  RX = 0;
  RY = 0;
  LT = 0;
  RT = 0;
  swap_mode = false;
}

using namespace std;

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

Json::Value root; // starts as "null"; will contain the root value after parsing
std::ifstream config_doc("gp2mkb.conf", std::ifstream::binary);

int main() {
  // config file
  config_doc >> root;
  // testing config access

  input_dev id;
  output_dev od;

  virtual_dev vd;

  const Json::Value dev_options = root["options"];
  if (dev_options.isNull()) { /* error */
  }

  /* Begin device options parsing*/

  /* "off" to disable preset switching, otherwise specify the button code to
   * use*/
  std::string valString = dev_options.get("modes", false).asString();
  const char *valCString = valString.c_str();
  if (valString.compare("off") == 0) {
    vd.mode_code = -1;
  } else {
    vd.mode_code = libevdev_event_code_from_name(EV_KEY, valCString);
    if (vd.mode_code < 0) {
      // error
    }
  }

  /* End device options parsing*/

  /* Currently there is only support for a single mode and modifier,
      but it shouldn't be too difficult to extend further.
  */

  /* Begin mode parsing */

  dev_mode tempMode;

  const Json::Value mode = root["mode_0"];
  if (mode.isNull()) { /* error */
  }

  const Json::Value mode_options = mode["options"];
  if (mode_options.isNull()) { /* error */
  }

  /* Setup whether this preset allows for trigger modifiers */
  valString = mode_options.get("modifiers", false).asString();
  if (valString.compare("left") == 0) {
    tempMode.lt_modifier = true;
    tempMode.rt_modifier = false;
  } else if (valString.compare("right") == 0) {
    tempMode.lt_modifier = false;
    tempMode.rt_modifier = true;
  } else if (valString.compare("both") == 0) {
    tempMode.lt_modifier = true;
    tempMode.rt_modifier = true;
  } else if (valString.compare("off") == 0) {
    tempMode.lt_modifier = false;
    tempMode.rt_modifier = false;
  } else {
    // error
  }

  /* Setup which stick(s) are used for the mouse. Any stick not used
     could be used for directional-bound key events
  */
  valString = mode_options.get("pointer_stick", false).asString();
  if (valString.compare("left") == 0) {
    tempMode.la_radial = false;
    tempMode.ra_radial = true;
  } else if (valString.compare("right") == 0) {
    tempMode.la_radial = true;
    tempMode.ra_radial = false;
  } else if (valString.compare("both") == 0) {
    tempMode.la_radial = true;
    tempMode.ra_radial = true;
  } else if (valString.compare("off") == 0) {
    tempMode.la_radial = false;
    tempMode.ra_radial = false;
  } else {
    // error
  }

  /* End mode parsing */

  /* Begin modifier preset parsing */
  mode_modifier tempModifier;
  event_sgnt tempDownSignature;
  event_sgnt tempUpSignature;
  struct input_event tempEvent;
  std::vector<struct input_event> tempDownEvents;
  std::vector<struct input_event> tempUpEvents;

  Json::Value mode_modifier = mode["no_modifier"];
  if (mode_modifier.isNull()) { /* error */
  }

  // setup button->key bindings first
  Json::Value modifier_mappings = mode_modifier["keys"];
  if (modifier_mappings.isNull()) { /* error */
  }
  std::vector<std::string> mappingKeyNames = modifier_mappings.getMemberNames();
  std::vector<std::string> mappingValNames;
  std::vector<std::string> mappingValSequence;
  int code;

  for (unsigned int i = 0; i < mappingKeyNames.size(); ++i) {
    // setup the button we will look for
    valCString = mappingKeyNames[i].c_str();
    code = libevdev_event_code_from_name(EV_KEY, valCString);
    if (code < 0) {
      // not a valid code
      continue;
    } else if (code == vd.mode_code) {
      // already used for mode switch. ignore it
      continue;
    }
    tempDownSignature.type = EV_KEY;
    tempDownSignature.code = code;
    tempDownSignature.value = 1;
    tempUpSignature.type = EV_KEY;
    tempUpSignature.code = code;
    tempUpSignature.value = 0;

    // setup the events we will send
    valString = modifier_mappings.get(mappingKeyNames[i], true).asString();
    mappingValSequence.clear();
    tempDownEvents.clear();
    tempUpEvents.clear();
    mappingValSequence = splitCommands(valString);
    for (unsigned int j = 0; j < mappingValSequence.size(); ++j) {
      valCString = mappingValSequence[j].c_str();
      code = libevdev_event_code_from_name(EV_KEY, valCString);
      if (code < 0) {
        // not a valid code
        continue;
      }
      tempEvent.type = EV_KEY;
      tempEvent.code = code;
      tempEvent.value = 1;
      tempDownEvents.push_back(tempEvent);
      tempEvent.value = 0;
      tempUpEvents.push_back(tempEvent);
    }
    // add the entries to the map (one for press and one for release)
    tempModifier.key_events[tempDownSignature] = tempDownEvents;
    tempModifier.key_events[tempUpSignature] = tempUpEvents;
  }

  // setup the trigger/d-pad->key bindings now
  modifier_mappings = mode_modifier["abs"];
  if (modifier_mappings.isNull()) { /* error */
  }
  mappingKeyNames.clear();
  mappingKeyNames = modifier_mappings.getMemberNames();
  mappingValNames.clear();
  mappingValSequence.clear();
  tempDownEvents.clear();
  tempUpEvents.clear();

  for (unsigned int i = 0; i < mappingKeyNames.size(); ++i) {
    valCString = mappingKeyNames[i].c_str();
    code = libevdev_event_code_from_name(EV_ABS, valCString);
    if (code < 0) {
      // not a valid code
      continue;
    } else if ((code >= ABS_HAT0X && code <= ABS_HAT3Y) ||
               (code == ABS_Z && !tempMode.lt_modifier) ||
               (code == ABS_RZ && !tempMode.rt_modifier)) {
      // we don't allow triggers to be bound if we're already using it as a
      // trigger modifier
      tempDownSignature.type = EV_ABS;
      tempDownSignature.code = code;
      tempDownSignature.value = 1;
      tempUpSignature.type = EV_ABS;
      tempUpSignature.code = code;
      tempUpSignature.value = 0;

      valString = modifier_mappings.get(mappingKeyNames[i], true).asString();
      mappingValSequence.clear();
      mappingValSequence = splitCommands(valString);

      for (unsigned int j = 0; j < mappingValSequence.size(); ++j) {
        valCString = mappingValSequence[j].c_str();
        code = libevdev_event_code_from_name(EV_KEY, valCString);
        if (code < 0) {
          // not a valid code
          continue;
        }
        tempEvent.type = EV_KEY;
        tempEvent.code = code;
        tempEvent.value = 1;
        tempDownEvents.push_back(tempEvent);
        tempEvent.value = 0;
        tempUpEvents.push_back(tempEvent);
      }
    } else {
      continue;
    }
    tempModifier.abs_events[tempDownSignature] = tempDownEvents;
    tempModifier.abs_events[tempUpSignature] = tempUpEvents;
  }
  // we'd want to set up the directional-bound key events here

  // add the modifier to the mode, then the mode to the virtual device
  tempMode.no_modifier = tempModifier;
  tempMode.curr_mod = 0;
  vd.mode_list.push_back(tempMode);
  vd.curr_mode = &(vd.mode_list[0]);
  vd.curr_mode->curr_modifier = &(vd.curr_mode->no_modifier);

  // check if mode_code > 0 before checking for more modes
  /* End modifier preset parsing */

  /* The send->translate->receive loop */
  unsigned int numEvs = 1;
  std::vector<struct input_event> inEvents;
  std::vector<struct input_event> outEvents;

  int count = 0;
  while (numEvs == 0 || numEvs > 0) {
    inEvents.clear();
    outEvents.clear();
    id.poll(&inEvents); // get all the events up to the next sync event
    numEvs = inEvents.size();
    for (unsigned int i = 0; i < numEvs; ++i) {
      outEvents.clear();
      // should never be EV_SYN, but I left it in here while testing just to be
      // safe
      if (inEvents[i].type == EV_SYN) {
        continue;
      }
      if (inEvents[i].type == 20) { // if it's analog stick event we handle them
                                    // differently. update the devie state
                                    // instead
        if (inEvents[i].code == REL_X) {
          vd.X = inEvents[i].value;
          if (!vd.curr_mode->la_radial) // this might be useful depending on
                                        // how/if we do directional-bound inputs
            continue;
        } else if (inEvents[i].code == REL_Y) {
          vd.Y = inEvents[i].value;
          if (!vd.curr_mode->la_radial)
            continue;
        } else if (inEvents[i].code == REL_RX) {
          vd.RX = inEvents[i].value;
          if (!vd.curr_mode->ra_radial)
            continue;
        } else if (inEvents[i].code == REL_RY) {
          vd.RY = inEvents[i].value;
          if (!vd.curr_mode->ra_radial)
            continue;
        }
        continue; // might need to remove for directional-bound input stuff
      }
      // an event signature is basically just an input_event minus the time
      // component. we could probably just use input_events now that the design
      // has changed
      event_sgnt signature = getSignature(inEvents[i]);
      if (signature.type == EV_KEY) {
        if (vd.mode_code ==
            signature
                .code) { // check if we need to swap our mode after this input
          vd.swap_mode = true;
          continue;
        }
        // use our event signature key in the right map and get the event
        // sequence we should send
        if (vd.curr_mode->curr_modifier->key_events.find(signature) !=
            vd.curr_mode->curr_modifier->key_events.end()) {
          outEvents = vd.curr_mode->curr_modifier->key_events[signature];
        } else {
          continue;
        }
      } else if (signature.type == EV_ABS) {
        if (signature.code == ABS_Z) { // keep track of the trigger states, but
                                       // still allow them to be pressed if
                                       // they're not acting as a modifier
          vd.LT = signature.type;
        } else if (signature.code == ABS_RZ) {
          vd.RT = signature.type;
        }
        if (vd.curr_mode->curr_modifier->abs_events.find(signature) !=
            vd.curr_mode->curr_modifier->abs_events.end()) {
          outEvents = vd.curr_mode->curr_modifier->abs_events[signature];
        } else {
          continue;
        }
      } else {
        // invalid input_event
        continue;
      }
      od.send(outEvents); // send the output events to the device
    }

    ++count;
    /* We can mess around with this threshold value to update how often we move
       the mouse.

       At lower values we build up a queue of stick input events and so the
       mouse
       will lag behind, continuing to complete its queue for a bit after stick
       input
       has stopped (if you want to see this move the stick in large circles
       rapidly).

       At higher values we avoid the lag issue, and the mouse moves as you move
       the stick
       (but there will be a slight decrease in response/accuracy)

    */

    /*
      Currently only the left stick is supported, and we don't take into account
      whether
      it is actually supposed to be controlled with the left stick
    */
    if (count > 5) {
      outEvents.clear();
      struct input_event tempEvent;
      tempEvent.type = EV_REL;
      tempEvent.code = REL_X;
      tempEvent.value = (int)vd.X / 4000; // 4000 seems to be a good deadzone on
                                          // my controller, scales mouse
                                          // movement somewhat according to how
                                          // far you move stick

      outEvents.push_back(tempEvent);
      struct tempEvent;
      tempEvent.type = EV_REL;
      tempEvent.code = REL_Y;
      tempEvent.value = (int)vd.Y / 4000;

      outEvents.push_back(tempEvent);
      od.send(outEvents);
      count = 0;
    }
    vd.curr_mode->updateModifier(
        vd.LT, vd.RT); // update what trigger modifiers are pressed
    if (vd.swap_mode) {
      // this should increment to the next mode, or go to the original mode if
      // at the end of the list
    }
  }
  return 0;
}

void output_dev::setupAllowedEvents(int *fd) {
  int i;

  /* Allow types of events */
  for (i = EV_SYN; i <= EV_SND; ++i) {
    if (ioctl(*fd, UI_SET_EVBIT, i) < 0) {
    } // ioctlerror
  }

  /* Enable specific events */

  for (i = KEY_RESERVED; i <= KEY_KPDOT; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0) {
    } // ioctlerror
  }
  for (i = KEY_ZENKAKUHANKAKU; i <= KEY_F24; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0) {
    } // ioctlerror
  }
  for (i = KEY_PLAYCD; i <= KEY_MICMUTE; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0) {
    } // ioctlerror
  }

  for (i = BTN_MISC; i <= BTN_9; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0) {
    } // ioctlerror
  }

  for (i = BTN_MOUSE; i <= BTN_TASK; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0) {
    } // ioctlerror
  }

  for (i = BTN_JOYSTICK; i <= BTN_THUMBR; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0) {
    } // ioctlerror
  }

  for (i = BTN_DIGI; i <= BTN_GEAR_UP; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0) {
    } // ioctlerror
  }

  for (i = KEY_OK; i <= KEY_IMAGES; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0) {
    } // ioctlerror
  }

  for (i = KEY_DEL_EOL; i <= KEY_DEL_LINE; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0) {
    } // ioctlerror
  }

  for (i = KEY_FN; i <= KEY_FN_B; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0) {
    } // ioctlerror
  }

  for (i = KEY_BRL_DOT1; i <= KEY_BRL_DOT10; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0) {
    } // ioctlerror
  }

  for (i = KEY_NUMERIC_0; i <= KEY_LIGHTS_TOGGLE; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0) {
    } // ioctlerror
  }

  for (i = BTN_DPAD_UP; i <= BTN_DPAD_RIGHT; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0) {
    } // ioctlerror
  }

  for (i = KEY_ALS_TOGGLE; i <= KEY_ALS_TOGGLE; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0) {
    } // ioctlerror
  }

  for (i = KEY_BUTTONCONFIG; i <= KEY_VOICECOMMAND; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0) {
    } // ioctlerror
  }

  for (i = KEY_BRIGHTNESS_MIN; i <= KEY_BRIGHTNESS_MAX; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0) {
    } // ioctlerror
  }

  /*
     for (i = KEY_KBDINPUTASSIST_PREV; i <= KEY_KBDINPUTASSIST_CANCEL; ++i) {
     if (ioctl(*fd, UI_SET_KEYBIT, i) < 0)
      {}//ioctlerror
     }*/
  for (i = KEY_KBDINPUTASSIST_PREV; i <= 0x276; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0) {
    } // ioctlerror
  }

  for (i = BTN_TRIGGER_HAPPY; i <= BTN_TRIGGER_HAPPY40; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0) {
    } // ioctlerror
  }

  for (i = REL_X; i <= REL_MISC; ++i) {
    if (ioctl(*fd, UI_SET_RELBIT, i) < 0) {
    } // ioctlerror
  }

  /* Disabled due to conflicts with relative events

  for (i = ABS_X; i <= ABS_RZ; ++i) {
    if (ioctl(*fd, UI_SET_ABSBIT, i) < 0)
      {}//ioctlerror
  }

  for (i = ABS_THROTTLE; i <= ABS_BRAKE; ++i) {
    if (ioctl(*fd, UI_SET_ABSBIT, i) < 0)
      {}//ioctlerror
  }

  for (i = ABS_HAT0X; i <= ABS_TOOL_WIDTH; ++i) {
    if (ioctl(*fd, UI_SET_ABSBIT, i) < 0)
      {}//ioctlerror
  }

  for (i = ABS_VOLUME; i <= ABS_VOLUME; ++i) {
    if (ioctl(*fd, UI_SET_ABSBIT, i) < 0)
      {}//ioctlerror
  }

  for (i = ABS_MISC; i <= ABS_MISC; ++i) {
    if (ioctl(*fd, UI_SET_ABSBIT, i) < 0)
      {}//ioctlerror
  }

  for (i = ABS_MT_SLOT; i <= ABS_MT_TOOL_Y; ++i) {
    if (ioctl(*fd, UI_SET_ABSBIT, i) < 0)
      {}//ioctlerror
  }*/

  for (i = SW_LID; i <= SW_MAX; ++i) {
    if (ioctl(*fd, UI_SET_SWBIT, i) < 0) {
    } // ioctlerror
  }

  for (i = MSC_SERIAL; i <= MSC_TIMESTAMP; ++i) {
    if (ioctl(*fd, UI_SET_MSCBIT, i) < 0) {
    } // ioctlerror
  }

  for (i = LED_NUML; i <= LED_CHARGING; ++i) {
    if (ioctl(*fd, UI_SET_LEDBIT, i) < 0) {
    } // ioctlerror
  }

  for (i = SND_CLICK; i <= SND_TONE; ++i) {
    if (ioctl(*fd, UI_SET_SNDBIT, i) < 0) {
    } // ioctlerror
  }
}
