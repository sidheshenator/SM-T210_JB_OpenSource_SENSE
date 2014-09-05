#ifndef _PLUGIN_TAG_H
#define _PLUGIN_TAG_H


enum plugin_tags {
    TAG_ANDROID,
    TAG_CODA,
    TAG_THERMAL,
    TAG_INPUT,
    TAG_DDR_HOTPLUG,
    TAG_MAX
};

struct plugin_info {
	const char *name;
	int tags;
};

static struct plugin_info plugin_infos[] = {
	{
		"android",
		TAG_ANDROID,
	},
	{
		"coda",
		TAG_CODA,
	},
	{
		"thermal",
		TAG_THERMAL,
	},
	{
		"ddr_hotplug",
		TAG_DDR_HOTPLUG,
	},
	{
		NULL,
		TAG_MAX,
	}
};

#endif
