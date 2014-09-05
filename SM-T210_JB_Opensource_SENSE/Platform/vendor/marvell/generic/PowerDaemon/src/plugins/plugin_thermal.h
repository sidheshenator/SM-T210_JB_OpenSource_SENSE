#ifndef _PLUGIN_THERMAL_H
#define _PLUGIN_THERMAL_H

enum thermal_state {
    STATE_SAFE,
    STATE_NOTICEABLE,
    STATE_WARNING,
    STATE_CRITICAL,
};

struct thermal_info {
	int state;
	int temperature;
};

#endif
