
#ifndef __DELEGATE_DDR_H
#define __DELEGATE_DDR_H

#include "list.h"

#define MAX_CMD_LEN 256
struct shcmd_state {
	struct LIST exec_cmd;
};

struct shcmd_global_data {
    struct LIST shcmdTicketObj;
} _delegate_shcmd;

#endif

