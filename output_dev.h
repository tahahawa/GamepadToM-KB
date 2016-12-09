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
#include <linux/uinput.h>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <vector>

class output_dev {
private:
  struct uinput_user_dev uindev;
  int fd;
  void setupAllowedEvents(int *);

public:
  output_dev();
  ~output_dev();
  void send(std::vector<struct input_event>, bool);
  void send(std::vector<struct input_event> &);
};
