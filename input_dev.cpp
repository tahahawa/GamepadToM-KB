#include "input_dev.h"

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
