#include "output_dev.h"

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

void output_dev::send(std::vector<struct input_event> &seq) { send(seq, true); }

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
