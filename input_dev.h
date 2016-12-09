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
