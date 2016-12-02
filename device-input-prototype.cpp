#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <libevdev/libevdev.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <sstream>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <json/value.h>
#include <json/json.h>



// A lot of the following code has been adapted from SDL

// Dependencies are glib and libevdev so compile with:
// gcc `pkg-config --cflags --libs libevdev glib-2.0`
// device-input-prototype.c

using namespace std;

int handleAbsEvent(struct input_event*);
Json::Value root;   // starts as "null"; will contain the root value after parsing
std::ifstream config_doc("config.json", std::ifstream::binary);




int deadzone[4][2] = {
        { -7000, 7000 }, { -7000, 7000 },
        { -7000, 7000 }, { -7000, 7000 }
};

int main() {
        //config file
        config_doc >> root;
        //testing config access
        string s = "BTN_SOUTH";
        string t = root["mappings"].get(s, true).asString();
        std::cout << t;

        int i;
        struct libevdev *dev = NULL;
        int fd;
        int rc = 1;

        // Detect the first joystick event file
        GDir *dir = g_dir_open("/dev/input/by-path/", 0, NULL);
        const gchar *fname;
        while ((fname = g_dir_read_name(dir))) {
                if (g_str_has_suffix(fname, "event-joystick"))
                        break;
        }
        std::cout << "Opening event file /dev/input/by-path/" << fname << endl;

        fd = open(g_strconcat("/dev/input/by-path/", fname, NULL),
                  O_RDONLY | O_NONBLOCK);
        rc = libevdev_new_from_fd(fd, &dev);
        if (rc < 0) {
                fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-rc));
                exit(1);
        }
        std::cout << "Input device name: " << libevdev_get_name(dev) << endl;
        std::cout << "Input device ID: bus " << libevdev_get_id_bustype(dev) << " vendor " << libevdev_get_id_vendor(dev) << " product " << libevdev_get_id_product(dev) << endl;
        std::cout << endl << endl << endl << endl;

        /* Poll Device */
        rc = -EAGAIN;
        while(rc == LIBEVDEV_READ_STATUS_SUCCESS || rc == -EAGAIN) {
                struct input_event ev;
                rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);

                if (rc == LIBEVDEV_READ_STATUS_SUCCESS ) {
                        int absNoise = 1;
                        if (ev.type == EV_ABS) {
                                if (handleAbsEvent(&ev)) {
                                        std::cout << libevdev_event_type_get_name(ev.type) << ", " << libevdev_event_code_get_name(ev.type, ev.code) << ", " << ev.value << endl;
                                }
                        } else {
                                if (ev.type != EV_SYN) {
                                        std::cout << libevdev_event_type_get_name(ev.type) << ", " << libevdev_event_code_get_name(ev.type, ev.code) << ", " << ev.value << endl;

                                }
                        }
                }

        }

        close(fd);

        return 0;
}

/* Takes an input_event* with type EV_ABS.
   Returns 1 if the input is valid, 0 if the input should be ignored
   (ie: value lies within deadzone).
   Currently uses a global 2D array called 'deadzone'.
 */
int handleAbsEvent(struct input_event *ev) {
        int ret = 0;
        switch (ev->code) {
        case ABS_X:
        case ABS_Y:
                if (ev->value < deadzone[ev->code][0])
                        ret = 1;
                else if (ev->value > deadzone[ev->code][1])
                        ret = 1;
                break;
        case ABS_RX:
        case ABS_RY:
                if (ev->value < deadzone[ev->code-1][0])
                        ret = 1;
                else if (ev->value > deadzone[ev->code-1][1])
                        ret = 1;
                break;
        default:
                ret = 1;
                break;
        }
        return ret;
}
