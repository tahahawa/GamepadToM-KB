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
#include <linux/input.h>
#include <linux/uinput.h>
#include <json/value.h>
#include <json/json.h>


class output_dev {
private:
struct uinput_user_dev uindev;
struct input_event ev;
int i;
int fd;
void setupAllowedEvents(int*);

public:
output_dev();
~output_dev();
void send(int*);

};

output_dev::output_dev(){
        fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
        if (fd < 0)
                delete this; // failed to open

        memset(&uindev, 0, sizeof(struct uinput_user_dev));
        snprintf(uindev.name, UINPUT_MAX_NAME_SIZE, "gp2mkb virtual device");
        uindev.id.bustype = BUS_USB;
        uindev.id.vendor = 0;
        uindev.id.product = 0;
        uindev.id.version = 1;
        if(write(fd, &uindev, sizeof(struct uinput_user_dev)) < 0)
                delete this;  //write error

        output_dev::setupAllowedEvents(&fd);
}

output_dev::~output_dev() {
        if (!(fd < 0))
                close(fd);
}

void output_dev::send(int*){
        //take event as param, parse it, and send it to the fd. idk how
        //TODO

        /* possible useful code block
           usleep(25000);
           memset(&ev, 0, sizeof(struct input_event));
           ev.type = EV_SYN;
           ev.code = SYN_REPORT;
           ev.value = 0;
           write(fd, &ev, sizeof(struct input_event));
           usleep(10000);*/
}
// A lot of the following code has been adapted from SDL

// Dependencies are glib and libevdev so compile with:
// gcc `pkg-config --cflags --libs libevdev glib-2.0`
// device-input-prototype.c
// amended to: g++ device-input-prototype.cpp -Wall `pkg-config --cflags --libs libevdev glib-2.0 jsoncpp`


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
        output_dev od;

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
//TODO, send events from this loop to output_dev object send function, using example getting of mapping from JSON file
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

void output_dev::setupAllowedEvents(int *fd) {
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
        for (i = KEY_PLAYCD; i <= KEY_MICMUTE; ++i) {
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
