#ifndef _SYSFS_DDR_H
#define _SYSFS_DDR_H

unsigned long sysfs_ddr_get_ddrinfo_curfreq(void);
struct ddrfreq_available_governors * sysfs_ddr_get_available_governors(void);
void sysfs_ddr_put_available_governors(struct ddrfreq_available_governors *any);
struct ddrfreq_available_frequencies * sysfs_ddr_get_available_frequencies(void);
void sysfs_ddr_put_available_frequencies(struct ddrfreq_available_frequencies *any);
int sysfs_ddr_access_scaling_governor(char *governor, int read);
int sysfs_ddr_access_scaling_maxfreq(unsigned long *max_freq, int read);
int sysfs_ddr_access_scaling_minfreq(unsigned long *min_freq, int read);
int sysfs_ddr_access_polling_interval(unsigned long *interval, int read);
int sysfs_ddr_get_scaling_policy(struct ddr_state *state);
int sysfs_ddr_set_scaling_policy(struct ddr_state *state);

#endif
