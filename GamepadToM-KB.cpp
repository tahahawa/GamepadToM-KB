#include "input_dev.h"
#include "output_dev.h"
#include "virtual_dev.h"
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

using namespace std;

vector<string> splitCommands(string inString) {
  stringstream ss;
  string buf;
  vector<string> ret;
  ss.str(inString);
  while (ss >> buf) {
    ret.push_back(buf);
  }
  return ret;
}

void deviceOptionsParsing(string valString, virtual_dev &vd);

void modifierOptionsParsing(string valString, dev_mode &tempMode);

void pointer_stickOptionParsing(string valString, dev_mode &tempMode);

void setupBindings(
    Json::Value *modifier_mappings, vector<string> *mappingKeyNames,
    vector<string> *mappingValNames, vector<string> *mappingValSequence,
    int code, virtual_dev &vd, event_sgnt *tempUpSignature,
    event_sgnt *tempDownSignature, vector<struct input_event> *tempUpEvents,
    vector<struct input_event> *tempDownEvents, input_event *tempEvent,
    mode_modifier *tempModifier, dev_mode *tempMode, int, string);

void updateLoop(vector<struct input_event> &inEvents,
                vector<struct input_event> &outEvents, input_dev &id,
                virtual_dev &vd, output_dev &od, unsigned int &numEvs,
                int &count);

Json::Value root; // starts as "null"; will contain the root value after parsing
ifstream config_doc("gp2mkb.conf", ifstream::binary);

int main() {
  // config file
  config_doc >> root;
  // testing config access

  const Json::Value dev_options = root["options"];
  if (dev_options.isNull()) {
    cout << "There is no config file. Please make a config file called "
            "'gp2mkb.conf'"
         << endl;
    return 0;
  }

  input_dev id;
  output_dev od;

  virtual_dev vd;

  /* Begin device options parsing*/

  /* "off" to disable preset switching, otherwise specify the button code to
   * use*/
  deviceOptionsParsing(dev_options.get("modes", false).asString(), vd);

  /* End device options parsing*/

  /* Currently there is only support for a single mode and modifier,
      but it shouldn't be too difficult to extend further.
  */

  /* Begin mode parsing */

  int modeCount = 0;
  while (1) {

    dev_mode tempMode;
    string modeName = "mode_";
    modeName.append(to_string(modeCount));
    Json::Value mode = root[modeName];

    if (mode.isNull()) { /* error */
      break;
    }

    Json::Value mode_options = mode["options"];
    if (mode_options.isNull()) { /* error */
    }

    /* Setup whether this preset allows for trigger modifiers */
    modifierOptionsParsing(mode_options.get("modifiers", false).asString(),
                           tempMode);

    /* Setup which stick(s) are used for the mouse. Any stick not used
       could be used for directional-bound key events
    */
    pointer_stickOptionParsing(
        mode_options.get("pointer_stick", false).asString(), tempMode);

    /* End mode parsing */

    /* Begin modifier preset parsing */


    string modifierNameArray[4] = {"no_modifier", "left_modifier",
                                         "right_modifier", "both_modifier" };

    for (int i = 0; i < 4; ++i) {

       mode_modifier tempModifier;
       event_sgnt tempDownSignature;
       event_sgnt tempUpSignature;
       struct input_event tempEvent;
       vector<struct input_event> tempDownEvents;
       vector<struct input_event> tempUpEvents;

      Json::Value mode_modifier = mode[modifierNameArray[i]];
      if (mode_modifier.isNull()) {
        cout << "Your mode modifier is missing, please fix your config" << endl;
      }

      // setup button->key bindings first
      Json::Value modifier_mappings = mode_modifier["keys"];
      if (modifier_mappings.isNull()) { /* error */
      }
      vector<string> mappingKeyNames = modifier_mappings.getMemberNames();
      vector<string> mappingValNames;
      vector<string> mappingValSequence;
      int code = 0;

      setupBindings(&modifier_mappings, &mappingKeyNames, &mappingValNames,
                    &mappingValSequence, code, vd, &tempUpSignature,
                    &tempDownSignature, &tempUpEvents, &tempDownEvents, &tempEvent,
                    &tempModifier, &tempMode, i, modeName);
    }
    tempMode.curr_mod = 0;
    tempMode.curr_modifier = NULL;
    vd.mode_list.push_back(tempMode);
    ++modeCount;
  }
  vd.curr_mode = &(vd.mode_list[0]);
  vd.curr_mode->curr_modifier = &(vd.curr_mode->no_modifier);


  /* The send->translate->receive loop */
  unsigned int numEvs = 1;
  vector<struct input_event> inEvents;
  vector<struct input_event> outEvents;

  int count = 0;
  while (numEvs == 0 || numEvs > 0) {
    updateLoop(inEvents, outEvents, id, vd, od, numEvs, count);
  }
  return 0;
}

void deviceOptionsParsing(string valString, virtual_dev &vd) {
  const char *valCString = valString.c_str();
  if (valString.compare("off") == 0) {
    vd.mode_code = -1;
  } else {
    vd.mode_code = libevdev_event_code_from_name(EV_KEY, valCString);
    if (vd.mode_code < 0) {
      cout << "Something happened that should never happen. Could not set up "
              "virtual device."
           << endl;
      return;
    }
  }
}

void modifierOptionsParsing(string valString, dev_mode &tempMode) {
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
    cout << "Unexpected modifier option specified, please fix your config"
         << endl;
    return;
  }
}

void pointer_stickOptionParsing(string valString, dev_mode &tempMode) {
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
    cout << "Your pointer stick configuration is wrong. Please fix your config."
         << endl;
    return;
  }
}

void setupBindings(
    Json::Value *modifier_mappings, vector<string> *mappingKeyNames,
    vector<string> *mappingValNames, vector<string> *mappingValSequence,
    int code, virtual_dev &vd, event_sgnt *tempUpSignature,
    event_sgnt *tempDownSignature, vector<struct input_event> *tempUpEvents,
    vector<struct input_event> *tempDownEvents, input_event *tempEvent,
    mode_modifier *tempModifier, dev_mode *tempMode, int modifierNum,
    string modeName) {

  for (unsigned int i = 0; i < (*mappingKeyNames).size(); ++i) {
    // setup the button we will look for
    const char *valCString = (*mappingKeyNames)[i].c_str();
    code = libevdev_event_code_from_name(EV_KEY, valCString);
    if (code < 0) {
      // not a valid code
      continue;
    } else if (code == vd.mode_code) {
      // already used for mode switch. ignore it
      continue;
    }
    (*tempDownSignature).type = EV_KEY;
    (*tempDownSignature).code = code;
    (*tempDownSignature).value = 1;
    (*tempUpSignature).type = EV_KEY;
    (*tempUpSignature).code = code;
    (*tempUpSignature).value = 0;

    // setup the events we will send
    string valString =
    (*modifier_mappings).get((*mappingKeyNames)[i], true).asString();
    (*mappingValSequence).clear();
    (*tempUpEvents).clear();
    (*tempDownEvents).clear();
    (*mappingValSequence) = splitCommands(valString);
    for (unsigned int j = 0; j < (*mappingValSequence).size(); ++j) {
      const char *valCString = (*mappingValSequence)[j].c_str();
      code = libevdev_event_code_from_name(EV_KEY, valCString);
      if (code < 0) {
        // not a valid code
        continue;
      }
      (*tempEvent).type = EV_KEY;
      (*tempEvent).code = code;
      (*tempEvent).value = 1;
      (*tempDownEvents).push_back((*tempEvent));
      (*tempEvent).value = 0;
      (*tempUpEvents).push_back((*tempEvent));
    }
    // add the entries to the map (one for press and one for release)
    (*tempModifier).key_events[(*tempDownSignature)] = (*tempDownEvents);
    (*tempModifier).key_events[(*tempUpSignature)] = (*tempUpEvents);
  }

  // setup the trigger/d-pad->key bindings now
  (*modifier_mappings) = root[modeName]["abs"];
  if ((*modifier_mappings).isNull()) { /* error */
  }
  (*mappingKeyNames).clear();
  (*mappingKeyNames) = (*modifier_mappings).getMemberNames();
  (*mappingValNames).clear();
  (*mappingValSequence).clear();
  (*tempDownEvents).clear();
  (*tempUpEvents).clear();

  for (unsigned int i = 0; i < (*mappingKeyNames).size(); ++i) {
    const char *valCString = (*mappingKeyNames)[i].c_str();
    code = libevdev_event_code_from_name(EV_ABS, valCString);
    if (code < 0) {
      // not a valid code
      continue;
    } else if ((code >= ABS_HAT0X && code <= ABS_HAT3Y) ||
               (code == ABS_Z && !(*tempMode).lt_modifier) ||
               (code == ABS_RZ && !(*tempMode).rt_modifier)) {
      // we don't allow triggers to be bound if we're already using it as a
      // trigger modifier
      (*tempDownSignature).type = EV_ABS;
      (*tempDownSignature).code = code;
      (*tempDownSignature).value = 1;
      (*tempUpSignature).type = EV_ABS;
      (*tempUpSignature).code = code;
      (*tempUpSignature).value = 0;

      string valString =
      (*modifier_mappings).get((*mappingKeyNames)[i], true).asString();
      (*mappingValSequence).clear();
      (*mappingValSequence) = splitCommands(valString);

      for (unsigned int j = 0; j < (*mappingValSequence).size(); ++j) {
        const char *valCString = (*mappingValSequence)[j].c_str();
        code = libevdev_event_code_from_name(EV_KEY, valCString);
        if (code < 0) {
          // not a valid code
          continue;
        }
        (*tempEvent).type = EV_KEY;
        (*tempEvent).code = code;
        (*tempEvent).value = 1;
        (*tempDownEvents).push_back((*tempEvent));
        (*tempEvent).value = 0;
        (*tempUpEvents).push_back((*tempEvent));
      }
    } else {
      continue;
    }
    (*tempModifier).abs_events[(*tempDownSignature)] = (*tempDownEvents);
    (*tempModifier).abs_events[(*tempUpSignature)] = (*tempUpEvents);
  }
  // we'd want to set up the directional-bound key events here

  // add the modifier to the mode, then the mode to the virtual device
  switch (modifierNum) {
    case 0:
      (*tempMode).no_modifier = (*tempModifier);
      break;
    case 1:
      (*tempMode).left_modifier = (*tempModifier);
      break;
    case 2:
      (*tempMode).right_modifier = (*tempModifier);
      break;
    case 3:
      (*tempMode).both_modifier = (*tempModifier);
      break;
    default:
      break;
  }
  // check if mode_code > 0 before checking for more modes
  /* End modifier preset parsing */
}

void updateLoop(vector<struct input_event> &inEvents,
                vector<struct input_event> &outEvents, input_dev &id,
                virtual_dev &vd, output_dev &od, unsigned int &numEvs,
                int &count) {
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
    event_sgnt signature;
    signature.getSignature(inEvents[i]);
    if (signature.type == EV_KEY) {
      // check if we need to swap our mode after this input
      if (vd.mode_code == signature.code && signature.value == 1) {
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
        vd.LT = signature.value;
      } else if (signature.code == ABS_RZ) {
        vd.RT = signature.value;
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

  // update what trigger modifiers are pressed
  vd.curr_mode->updateModifier(vd.LT, vd.RT);

  if (vd.swap_mode) {
    // this should increment to the next mode, or go to the original mode if
    // at the end of the list
    int gdb = 1;
    if (vd.modeNum < (vd.mode_list.size()-1)) {
      ++vd.modeNum;
    } else {
      vd.modeNum = 0;
    }
    // make sure we leave the mode in the default state before switching
    vd.curr_mode->curr_modifier = &(vd.curr_mode->no_modifier);
    vd.curr_mode = &(vd.mode_list[vd.modeNum]);
    vd.curr_mode->curr_modifier = &(vd.curr_mode->no_modifier);
    vd.swap_mode = false;
  }
}
