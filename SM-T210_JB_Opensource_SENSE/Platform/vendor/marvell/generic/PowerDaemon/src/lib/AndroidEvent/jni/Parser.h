#ifndef _JNI_PARSER_h
#define _JNI_PARSER_h

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <cutils/list.h>

#include "EventRelay.h"
#include "plugin_tags.h"
#include "plugin_android.h"
#include "plugin_thermal.h"
#include "plugin_ddr_hotplug.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "EventRelay_JNI"

#define MAX_ATTR_LEN 64

/* EVENT TYPES */
#define STATUS_EVENT 0
#define CHAR_EVENT 1

/* EXTRA TYPES */
#define EXTRA_NONE 0
#define EXTRA_INT 1
#define EXTRA_LONG 2
#define EXTRA_STRINGS 3
#define EXTRA_BOOLEAN 4

/* chip revision */
enum {
	PXA988_Z1,
	PXA988_Z2,
	PXA988_Z3,
};

/*
Template of powerdaemon.xml
  <Plugins>
    <XXX_PLUGIN enable="1">
      <!--XXX event: status event-->
      <event keyword="XXX" intent="android.intent.action.BOOT_COMPLETED" status="1" />
        <extra type="String" name="state" value="RINGING" status="1"/>
        <extra type="String" name="state" value="OFFHOOK" status="1"/>
        <extra type="String" name="state" value="IDLE" status="0"/>
      </event>
      <!--fm state event-->
      <event keyword="fm" intent="com.marvell.fmradio.ENABLE" status="1" />
      <event keyword="fm" intent="com.marvell.fmradio.DISABLE" status="0" />
      <event keyword="fm" intent="com.marvell.fmradio.RECORD" status="0" />
      <event keyword="fm" intent="com.marvell.fmradio.STOPRECORD" status="1" />
      <!--XXX event: char event-->
      <event keyword="XXX" intent="com.marvell.cpugadget.TASK_MONITOR">
        <extra type="String" name="taskName" />
      </event>
    </XXX_PLUGIN>
  </Plugins>
  <Rules>
    <Bootup level="0" constraint="Booting" enable="1">
      <android_state>boot=0</android_state>
    </Bootup>
    ...
  </Rules>
  <Constraints>
    ...
    <Booting>
      <cpu>
        <minfreq>100%</minfreq>
        <maxfreq>100%</maxfreq>
        <governor>performance</governor>
      </cpu>
    </Booting>
    ...
  </Constraints>

Android Level(EventRelay)            JNI              ppd
Intent(with/without extra) ==> Event ==> Command ==> socket

1.  When ppd startup, EventRelay would trigger parser to parse the configure
file. Parser would init event_list, cmd_list.

2. When a target intent coming, EventRelay would parse the intent, and get
the event from the list, and achieve the cmd in event, fill back the cmd if
necessary.
*/

union plugin_data_info {
	struct android_event android_data;
	struct thermal_info thermal_data;
	struct ddr_hotplug_info ddr_hotplug_data;
};

/* structure of command for socket */
struct command_t {
	int tag; // tag for specific plugin
	int len; //the len of REAL data size, NOT the size of android_data
	union plugin_data_info plugin_data;
};

/* structure for cmd_list */
struct command_link_t {
	struct command_t cmd;
	struct listnode link;
};
typedef struct command_link_t Command;

struct extra_t {
	int extra_type; // FIXME: type: long/string/int/float/double
	char extra_name[MAX_ATTR_LEN]; // the extra name for getXXXExtra
	char extra_value[MAX_ATTR_LEN];// the value for the extra_name
};

/* structure of event */
struct event_config_t {
	// the action name of intent for the event
	char intent_info[MAX_ATTR_LEN];

	// the type of event, right now there are two event
	// type: STATUS_EVENT/CHAR_EVENT
	int event_type;

	// the data of event, used to fill in the cmd struct.
	// STATUS_EVENT: the state for the event
	// CHAR_EVENT: the arguments shipped in extra;
	union event_data ev_data;

	// struct to store all the info of extra, such as extra type,
	// extra key/value, the value would store into ev_data
	struct extra_t *extra_info;

	// the specific cmd for the event, would fill back it if neccessary
	// when send into the socket.
	Command* cmd;

	// link node
	struct listnode link;
};
typedef struct event_config_t Event;

void dump_config(struct listnode*);
int init_event_config(struct listnode*);
void free_config_list(struct listnode*);

#endif
