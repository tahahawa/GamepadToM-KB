#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <libevdev/libevdev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// A lot of the following code has been adapted from SDL

// Dependencies are glib and libevdev so compile with:
// gcc `pkg-config --cflags --libs libevdev glib-2.0`
// device-input-prototype.c

int main() {
  int i;
  struct libevdev *device = NULL;
  int fd;
  int rc = 1;

  // Detect the first joystick event file
  GDir *dir = g_dir_open("/dev/input/by-path/", 0, NULL);
  const gchar *fname;
  while ((fname = g_dir_read_name(dir))) {
    if (g_str_has_suffix(fname, "event-joystick"))
      break;
  }
  printf("Opening event file /dev/input/by-path/%s\n", fname);

  fd = open(g_strconcat("/dev/input/by-path/", fname, NULL),
            O_RDONLY | O_NONBLOCK);
  rc = libevdev_new_from_fd(fd, &device);
  if (rc < 0) {
    fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-rc));
    exit(1);
  }
  printf("Input device name: \"%s\"\n", libevdev_get_name(device));
  printf("Input device ID: bus %#x vendor %#x product %#x\n",
         libevdev_get_id_bustype(device), libevdev_get_id_vendor(device),
         libevdev_get_id_product(device));

  printf("\n\n\n\n");

  /* Poll Device */
  rc = -EAGAIN;
  while(rc == LIBEVDEV_READ_STATUS_SUCCESS || rc == -EAGAIN) {
    struct input_event event;
    rc = libevdev_next_event(device, LIBEVDEV_READ_FLAG_NORMAL, &event);
    if (rc == LIBEVDEV_READ_STATUS_SUCCESS ) {
      printf("%s, %s, %d\n", libevdev_event_type_get_name(event.type),
                           libevdev_event_code_get_name(event.type, event.code),
                            event.value);
    }
  }
}
