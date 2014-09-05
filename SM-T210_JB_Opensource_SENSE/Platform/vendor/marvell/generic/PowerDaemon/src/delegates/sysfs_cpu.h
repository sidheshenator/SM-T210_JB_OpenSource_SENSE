#ifndef _SYSFS_CPU_H
#define _SYSFS_CPU_H

int sysfs_wait_ready(char *path);
int sysfs_cpu_exists(unsigned int cpu);
unsigned long sysfs_cpu_get_scaling_curfreq(unsigned int cpu);
unsigned long sysfs_cpu_get_cpuinfo_curfreq(unsigned int cpu);
unsigned long sysfs_cpu_get_transition_latency(unsigned int cpu);
unsigned long sysfs_cpu_get_transitions(unsigned int cpu);
int sysfs_cpu_get_hardware_limits(unsigned int cpu, unsigned long *min, unsigned long *max);
struct cpufreq_available_governors * sysfs_cpu_get_available_governors(unsigned int cpu);
void sysfs_cpu_put_available_governors(struct cpufreq_available_governors *any);
int sysfs_cpu_access_governor_upthreshold(char *gov, unsigned int *up_threshold, int read);
int sysfs_cpu_access_governor_downthreshold(char *gov, unsigned int *down_threshold, int read);
int sysfs_cpu_access_governor_samplingrate(char *gov, unsigned int *sampling_rate, int read);
struct cpufreq_available_frequencies * sysfs_cpu_get_available_frequencies(unsigned int cpu);
void sysfs_cpu_put_available_frequencies(struct cpufreq_available_frequencies *any);
int sysfs_cpu_access_scaling_governor(unsigned int cpu, char *governor, int read);
int sysfs_cpu_access_scaling_maxfreq(unsigned int cpu, unsigned long *max_freq, int read);
int sysfs_cpu_access_scaling_minfreq(unsigned int cpu, unsigned long *min_freq, int read);
int sysfs_cpu_access_online(unsigned int cpu, int *online, int read);
int sysfs_cpu_access_hotplug_lock(int *lock_status, int read);
int sysfs_cpu_modify_scaling_setspeed(unsigned int cpu, unsigned long target_frequency);
int sysfs_cpu_get_scaling_policy(struct cpu_state *state);
int sysfs_cpu_set_scaling_policy(struct cpu_state *state);
int sysfs_cpu_set_hotplug_status(int status);
int sysfs_cpu_get_hotplug_status(int *status);
int sysfs_cpu_set_online_status(struct cpu_state *state);
int sysfs_cpu_get_online_status(struct cpu_state *state, int *status);

#endif
