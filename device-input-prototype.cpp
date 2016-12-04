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


class input_dev {
private:
struct libevdev* dev;
int fd;
bool validInput(struct input_event*);
int deadzone[4][2];

public:
input_dev();
~input_dev();
int poll(struct input_event*, int);

};

input_dev::input_dev() {
	int rc = 1;

	// Detect the first joystick event file
	GDir *dir = g_dir_open("/dev/input/by-path/", 0, NULL);
	const gchar *fname;
	while ((fname = g_dir_read_name(dir))) {
		if (g_str_has_suffix(fname, "event-joystick"))
		break;
	}
	std::cout << "Opening event file /dev/input/by-path/" << fname << std::endl;

	fd = open(g_strconcat("/dev/input/by-path/", fname, NULL), O_RDONLY | O_NONBLOCK);
	rc = libevdev_new_from_fd(fd, &dev);
	if (rc < 0) {
		fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-rc));
		exit(1);
	}
	std::cout << "Input device name: " << libevdev_get_name(dev) << std::endl;
	std::cout << "Input device ID: bus " << libevdev_get_id_bustype(dev) << " vendor " << libevdev_get_id_vendor(dev) << " product " << libevdev_get_id_product(dev) << std::endl;
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
}

int input_dev::poll(struct input_event* events, int size) {
	int count = 0;
	struct input_event ev;

	memset(events, 0, size * sizeof(struct input_event));


	int rc = LIBEVDEV_READ_STATUS_SUCCESS;
	while(rc == LIBEVDEV_READ_STATUS_SUCCESS) {
		memset(&ev, 0, sizeof(struct input_event));
		rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);

		if (rc != LIBEVDEV_READ_STATUS_SUCCESS)
			break;
		if (ev.type == EV_SYN)
			break;

		if (count == size-1)
			return -1;

		if (validInput(&ev) && ev.type != EV_SYN) {
			events[count] = ev;
			++count;
		}
	}
	return count;
}

bool input_dev::validInput(struct input_event* ev) {
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
		if (ev->value < deadzone[ev->code-1][0])
			isValid = true;
		else if (ev->value > deadzone[ev->code-1][1])
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
void setupAllowedEvents(int*);

public:
output_dev();
~output_dev();
void send(struct input_event*, int, bool);

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
        if(write(fd, &uindev, sizeof(struct uinput_user_dev)) < 0)
                delete this;  //write error

        output_dev::setupAllowedEvents(&fd);

        if (ioctl(fd, UI_DEV_CREATE) < 0)
                delete this; // creation error
        usleep(50000);

}

output_dev::~output_dev() {
        if (!(fd < 0))
                close(fd);
}

void output_dev::send(struct input_event* events, int size, bool autoSync=true){
        //take event as param, parse it, and send it to the fd. idk how

        for (int i = 0; i < size; ++i) {
                write(fd, &(events[i]), sizeof(struct input_event));
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
// amended to: g++ device-input-prototype.cpp -Wall `pkg-config --cflags --libs libevdev glib-2.0 jsoncpp`


using namespace std;

Json::Value root;   // starts as "null"; will contain the root value after parsing
std::ifstream config_doc("config.json", std::ifstream::binary);


int main() {
        //config file
        config_doc >> root;
        //testing config access
        output_dev od;

        string s = "BTN_SOUTH";
        string t = root["mappings"].get(s, true).asString();
        std::cout << t;

        input_dev id;

	int numEvs = 1;
	struct input_event events[8];

	while (numEvs == -1 || numEvs == 0 || numEvs > 0) {
		numEvs = id.poll(events, 8);
		//TODO, send events from this loop to output_dev object send function, using example getting of mapping from JSON file
		if (numEvs > 0){
			for (int i = 0; i < numEvs; ++i) {
				std::cout << libevdev_event_type_get_name(events[i].type) << ", " << libevdev_event_code_get_name(events[i].type, events[i].code) << ", " << events[i].value << std::endl;
			}
		}
	}

        return 0;
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



        for (i = REL_X; i <= REL_MISC; ++i) {
                if (ioctl(*fd, UI_SET_RELBIT, i) < 0)
                {}//ioctlerror
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
