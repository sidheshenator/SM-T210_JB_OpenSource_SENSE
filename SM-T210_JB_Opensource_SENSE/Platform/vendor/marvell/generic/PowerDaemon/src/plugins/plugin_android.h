#ifndef _PLUGIN_ANDROID_H
#define _PLUGIN_ANDROID_H

#define MAX_DATA_LEN	64
#define EVENT_STATE		0
#define EVENT_EXTRA		1

union event_data {
	int state;
	char extra[MAX_DATA_LEN];
};

struct android_event {
	char keyword[MAX_DATA_LEN];
	int ev_type;
	union event_data ev_data;
};

struct plugin_event_state {
	const char *keyword;
	int state;
};

#endif
