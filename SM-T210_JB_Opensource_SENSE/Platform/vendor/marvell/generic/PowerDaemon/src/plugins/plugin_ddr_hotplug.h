#ifndef _PLUGIN_DDR_HOTPLUG_H
#define _PLUGIN_DDR_HOTPLUG_H

enum ddr_hotplug_state {
	DDR_ONLINE,
	DDR_OFFLINE,
};

struct ddr_hotplug_info {
	int state;
};

#endif
