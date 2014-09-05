#ifndef _PLUGIN_CODA_H
#define _PLUGIN_CODA_H

#define MAX_DATA_LEN	128
#define CODA_START	0
#define CODA_UPDATE 1
#define CODA_STOP	2

enum video_resolution {
    VGA,
    HD720P,
    HD1080P,
};

struct coda_video_profile {
    int codec_type;     //0: dec, 1: enc
    int resolution;     //0: vga, 1:720p, 2:1080p
    int fps;            //only valid for encoder currently, just set it as 0 for decoder
    int strm_fmt;       //stream format, not used currently, just set it as 0
    int sideinfo;       //only for debug purpose currently, just set it as 0
};

struct coda_info {
    int tid;
    int event;  /* 0:start, 1:update, 2:stop */
    struct coda_video_profile profile;
};

struct command_t {
    int tag;
    int len;
    struct coda_info info;
};

#endif
