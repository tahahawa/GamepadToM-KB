#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void setupAllowedEvents(int*);

int main(int argc, char const *argv[]) {
  struct uinput_user_dev uindev;
  struct input_event ev;
  int i, fd;

  fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
  if (fd < 0)
    {}// failed to open


  /* Set up the id information for the device */
  memset(&uindev, 0, sizeof(struct uinput_user_dev));
  snprintf(uindev.name, UINPUT_MAX_NAME_SIZE, "gp2mkb virtual device");
  uindev.id.bustype = BUS_USB;
  uindev.id.vendor = 0;
  uindev.id.product = 0;
  uindev.id.version = 1;
  if(write(fd, &uindev, sizeof(struct uinput_user_dev)) < 0)
    {}//write error

  setupAllowedEvents(&fd);

  /* Create the device */
  if (ioctl(fd, UI_DEV_CREATE) < 0)
    {}//ioctlerror

  /* Device doesn't always pick up first event
   * Adding an initial delay seems to help (min 25,000 microseconds)
   * Sync report improves eve more, but might just be the extra usleep
  */
  usleep(25000);
  memset(&ev, 0, sizeof(struct input_event));
  ev.type = EV_SYN;
  ev.code = SYN_REPORT;
  ev.value = 0;
  write(fd, &ev, sizeof(struct input_event));
  usleep(10000);


  /* Send events here */

  if (ioctl(fd, UI_DEV_DESTROY) < 0)
    {}//ioctlerror

  close(fd);

  return 0;
}


void setupAllowedEvents(int *fd) {
  int i;

  /* Allow types of events */
  for (i = EV_SYN; i <= EV_SND; ++i) {
    if (ioctl(*fd, UI_SET_EVBIT, i) < 0)
      {}//ioctlerror
  }

  /* Enable specific events */

  for (i = KEY_RESERVED; i <= KEY_KPDOT; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0)
      {}//ioctlerror
  }
  for (i = KEY_ZENKAKUHANKAKU; i <= KEY_F24; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0)
      {}//ioctlerror
  }
  for (i = KEY_PLAYCD; i <= KEY_MICMUTE ; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0)
      {}//ioctlerror
  }

  for (i = BTN_MISC; i <= BTN_9; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0)
      {}//ioctlerror
  }

  for (i = BTN_MOUSE; i <= BTN_TASK; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0)
      {}//ioctlerror
  }

  for (i = BTN_JOYSTICK; i <= BTN_THUMBR; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0)
      {}//ioctlerror
  }

  for (i = BTN_DIGI; i <= BTN_GEAR_UP; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0)
      {}//ioctlerror
  }

  for (i = KEY_OK; i <= KEY_IMAGES; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0)
      {}//ioctlerror
  }

  for (i = KEY_DEL_EOL; i <= KEY_DEL_LINE; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0)
      {}//ioctlerror
  }

  for (i = KEY_FN; i <= KEY_FN_B; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0)
      {}//ioctlerror
  }

  for (i = KEY_BRL_DOT1; i <= KEY_BRL_DOT10; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0)
      {}//ioctlerror
  }

  for (i = KEY_NUMERIC_0; i <= KEY_LIGHTS_TOGGLE; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0)
      {}//ioctlerror
  }

  for (i = BTN_DPAD_UP; i <= BTN_DPAD_RIGHT; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0)
      {}//ioctlerror
  }

  for (i = KEY_ALS_TOGGLE; i <= KEY_ALS_TOGGLE; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0)
      {}//ioctlerror
  }

  for (i = KEY_BUTTONCONFIG; i <= KEY_VOICECOMMAND; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0)
      {}//ioctlerror
  }

  for (i = KEY_BRIGHTNESS_MIN; i <= KEY_BRIGHTNESS_MAX; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0)
      {}//ioctlerror
  }

  /*
  for (i = KEY_KBDINPUTASSIST_PREV; i <= KEY_KBDINPUTASSIST_CANCEL; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0)
      {}//ioctlerror
  }*/
  for (i = KEY_KBDINPUTASSIST_PREV; i <= 0x276; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0)
      {}//ioctlerror
  }


  for (i = BTN_TRIGGER_HAPPY; i <= BTN_TRIGGER_HAPPY40; ++i) {
    if (ioctl(*fd, UI_SET_KEYBIT, i) < 0)
      {}//ioctlerror
  }



  for (i = BTN_TRIGGER_HAPPY; i <= REL_MISC; ++i) {
    if (ioctl(*fd, UI_SET_RELBIT, i) < 0)
      {}//ioctlerror
  }



  for (i = ABS_X; i <= ABS_BRAKE; ++i) {
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
  }



  for (i = SW_LID; i <= SW_MAX; ++i) {
    if (ioctl(*fd, UI_SET_SWBIT, i) < 0)
      {}//ioctlerror
  }


  for (i = MSC_SERIAL; i <= MSC_TIMESTAMP; ++i) {
    if (ioctl(*fd, UI_SET_MSCBIT, i) < 0)
      {}//ioctlerror
  }

  for (i = LED_NUML; i <= LED_CHARGING; ++i) {
    if (ioctl(*fd, UI_SET_LEDBIT, i) < 0)
      {}//ioctlerror
  }

  for (i = SND_CLICK; i <= SND_TONE; ++i) {
    if (ioctl(*fd, UI_SET_SNDBIT, i) < 0)
      {}//ioctlerror
  }
}
