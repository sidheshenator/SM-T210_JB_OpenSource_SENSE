
#ifndef __DELEGATE_CPU_H
#define __DELEGATE_CPU_H

#include "list.h"

#define MAX_GOVERNOR_LEN 32
struct governor_parameter {
	unsigned int up_threshold;
	unsigned int down_threshold;
	unsigned int sampling_rate;
	//may add more here
};

struct cpu_state {
	unsigned int id;
	unsigned long minfreq;
	unsigned long maxfreq;
	unsigned long setspeed;
	unsigned int online;
	char governor[MAX_GOVERNOR_LEN];
	struct governor_parameter gov_para;
};

enum cpufreq_governor_prio {
	PRIO_CPUFREQ_ONDEMAND,
	PRIO_CPUFREQ_INTERACTIVE,
	PRIO_CPUFREQ_POWERSAVE,
	PRIO_CPUFREQ_CONSERVATIVE,
	PRIO_CPUFREQ_PERFORMANCE,
	PRIO_CPUFREQ_USERSPACE,
	PRIO_CPUFREQ_UNKNOWN,
	PRIO_CPUFREQ_MAX,
};

static const char *supported_governors[PRIO_CPUFREQ_MAX] = {
	[PRIO_CPUFREQ_POWERSAVE] = "powersave",
	[PRIO_CPUFREQ_CONSERVATIVE] = "conservative",
	[PRIO_CPUFREQ_ONDEMAND] = "ondemand",
	[PRIO_CPUFREQ_INTERACTIVE] = "interactive",
	[PRIO_CPUFREQ_PERFORMANCE] = "performance",
	[PRIO_CPUFREQ_USERSPACE] = "userspace",
	[PRIO_CPUFREQ_UNKNOWN] = "unknown",
};

//////////////////////////////////////
struct cpufreq_available_governors {
	char *governor;
	struct cpufreq_available_governors *next;
	struct cpufreq_available_governors *first;
};

struct cpufreq_available_frequencies {
	unsigned long frequency;
	struct cpufreq_available_frequencies *next;
	struct cpufreq_available_frequencies *first;
};

struct cpufreq_sys_info {
	unsigned long min;
	unsigned long max;
	struct cpufreq_available_governors *governors;
	struct cpufreq_available_frequencies *frequencies;
};

struct cpu_hardware_info {
	unsigned int cpus;
	struct cpufreq_sys_info *sys_info;
};

struct cpu_global_data {
	struct cpu_hardware_info *cpu_info;
	struct LIST cpuTicketObj;
	struct cpu_state *result;
} _delegate_cpu;

#endif

