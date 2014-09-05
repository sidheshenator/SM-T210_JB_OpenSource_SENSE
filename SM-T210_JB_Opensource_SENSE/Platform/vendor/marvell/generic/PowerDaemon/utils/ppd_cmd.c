#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include "ppd_remote.h"
#include "ppd_log.h"

void make_command_by_type(Command *, int, int, char** );
Command* gen_command (int , char**);


static int ppd_dirs(const struct dirent *d)
{
	return (strncmp(d->d_name, "ppd-", 4) == 0);
}

static void usage(char *argv[])
{
	fprintf(stdout, "\n%s: Test the command socket of ppd\n", argv[0]);
	fprintf(stdout, "Usage: %s [commandID] [args]\n", argv[0]);
	fprintf(stdout, "Commands: \n");
	fprintf(stdout, "  0:  LIST_RULE:\n");
	fprintf(stdout, "  1:  CUR_RULE: \n");
	fprintf(stdout, "  2:  SET_RULE:				[Rule Name]\n");
	fprintf(stdout, "  3:  CLEAR_RULE:			[Rule Name]\n");
	fprintf(stdout, "  4:  SET_LOG_LEVEL:			[Level(0-7)]\n");
	fprintf(stdout, "  5:  SET_PPD_MODE:			[PPD MODE(auto/manual)]\n");
	fprintf(stdout, "  6:  GET_SCALING_AVAIL_FREQS: 		[Delegate(cpu[0/1]/vpu/gpu/ddr)]\n");
	fprintf(stdout, "  7:  CUR_PROFILE: 			[Delegate(cpu[0/1]/vpu/gpu/ddr)]\n");
	fprintf(stdout, "  8:  SET_FREQ:				[Delegate(cpu[0/1]/vpu/gpu/ddr)][Freq]\n");
}

void make_command_by_type(Command *c,int type, int argc, char* args[])
{
	c->type = type;
	c->argc = argc;
	if (args != NULL) { /* cmd with args */
		if (type == CMD_SET_FREQ) { /* <delegates type><delegates Id><freq> */
			if ((strncmp(args[2],"cpu",3) == 0) || (strncmp(args[2],"gpu",3) == 0)) {
				char* ch = args[2];
				if (strncmp(args[2],"cpu",3) == 0)
					strncat(c->args,"cpu ",4);
				else
					strncat(c->args,"gpu ",4);

				if (strlen(args[2])==3)
					strncat(c->args,"0",1);     /*add default delegate id-0*/
				else
					strncat(c->args,ch+3,1);    /*add the delegate id*/

				strncat(c->args," ",1);
				strncat(c->args,args[3],strlen(args[3]));
			} else {
				strncat(c->args,args[2],strlen(args[2]));
				strncat(c->args," 0 ",3);    /* add the delegate id-default 0 */
				strncat(c->args,args[3],strlen(args[3]));
			}
		} else if ((type == CMD_CUR_PROFILE)
				|| (type == CMD_GET_SCALING_AVAIL_FREQS)) { /* <delegates type><delegates Id> */
			if ((strncmp(args[2],"cpu",3) == 0) || (strncmp(args[2],"gpu",3) == 0)) {
				char* ch = args[2];
				if (strncmp(args[2],"cpu",3) == 0)
					strncat(c->args,"cpu ",4);
				else
					strncat(c->args,"gpu ",4);

				if (strlen(args[2]) == 3)
					strncat(c->args,"0",1);     /* add default delegate id-0 */
				else
					strncat(c->args,ch+3,1);    /* add the delegate id */
				strncat(c->args," ",1);

			} else {
				strncat(c->args,args[2],strlen(args[2]));
				strncat(c->args," 0",2);    /* add the delegate id-default 0 */
			}
		} else {
			for(int i=0; i < argc; i++) {
				printf("%s,len: %d\n",args[i+2],strlen(args[i+2]));
				strncat(c->args,args[i+2],strlen(args[i+2]));
				if (i != argc-1)
					strncat(c->args," ",1);
			}
		}
	}
}

Command* gen_command (int argc, char* argv[])
{
	Command* c = (Command *)calloc(1,sizeof(Command));
	int type = atoi(argv[1]);
	int cmd;
	switch (type) {
		case 0: /* CMD_LIST_RULES */
			cmd = CMD_LIST_RULES;
			make_command_by_type(c,cmd,0,NULL);
			break;

		case 1: /* CMD_CUR_RULE */
			cmd = CMD_CUR_RULE;
			make_command_by_type(c,cmd,0,NULL);
			break;

		case 2: /* CMD_SET_RULE */
			if (argc < 3) {
				cmd = INVALID_CMD;
				break;
			}
			cmd = CMD_SET_RULE;
			make_command_by_type(c,cmd,1,argv);
			break;

		case 3: /* CMD_CLEAR_RULE */
			if (argc < 3) {
				cmd = INVALID_CMD;
				break;
			}
			cmd = CMD_CLEAR_RULE;
			make_command_by_type(c,cmd,1,argv);
			break;

		case 4: /* CMD_SET_LOG_LEVEL */
			if (argc < 3) {
				cmd = INVALID_CMD;
				break;
			}
			cmd = CMD_SET_LOG_LEVEL;
			int level = atoi(argv[2]);
			if (level < LOG_EMERG || level > LOG_DEBUG)
			   cmd = INVALID_CMD;
			else
			   make_command_by_type(c,cmd,1,argv);
			break;

			case 5: /* CMD_SET_PPD_MODE */
			if (argc < 3) {
				cmd = INVALID_CMD;
				break;
			}
			cmd = CMD_SET_PPD_MODE;
			if (!(strcmp("auto",argv[2]) == 0 || strcmp("manual",argv[2]) == 0)) {
				cmd = INVALID_CMD;
			} else
				make_command_by_type(c,cmd,1,argv);
			break;

		case 6: /* CMD_GET_SCALING_AVAIL_FREQS */
			if (argc < 3) {
				cmd = INVALID_CMD;
				break;
			}
			cmd = CMD_GET_SCALING_AVAIL_FREQS;
			if (!(strncmp("cpu",argv[2],3) == 0 || strncmp("ddr",argv[2],3) == 0
				|| strncmp("vpu",argv[2],3) == 0  || strncmp("gpu",argv[2],3) == 0)) {
				cmd = INVALID_CMD;
			} else
				make_command_by_type(c,cmd,2,argv);
			break;

		case 7: /* CMD_CUR_PROFILE */
			if (argc < 3) {
				cmd = INVALID_CMD;
				break;
			}
			cmd = CMD_CUR_PROFILE;
			if (argc == 3) {
				if (!(strncmp("cpu",argv[2],3) == 0 || strncmp("ddr",argv[2],3) == 0
					|| strncmp("vpu",argv[2],3) == 0  || strncmp("gpu",argv[2],3) == 0)) {
					cmd = INVALID_CMD;
				} else
					make_command_by_type(c,cmd,2,argv);
			}
			break;

		case 8: /* CMD_SET_FREQ */
			if (argc < 4) {
				cmd = INVALID_CMD;
				break;
			}
			cmd = CMD_SET_FREQ;
			if (!(strncmp("cpu",argv[2],3) == 0 || strncmp("ddr",argv[2],3) == 0
				|| strncmp("vpu",argv[2],3) == 0  || strncmp("gpu",argv[2],3) == 0)) {
				cmd = INVALID_CMD;
			} else
				make_command_by_type(c,cmd,3,argv);
			printf("%s,len %d\n",c->args,strlen(c->args));
			break;

		default:
			cmd = INVALID_CMD;
	}

	if (cmd == INVALID_CMD) {
		fprintf(stdout, "Error args for %s\n", argv[0]);
		free(c);
		usage(argv);
		return NULL;
	}
	return c;
}


int main(int argc, char *argv[])
{
	int sock;
	struct dirent **namelist = NULL;
	struct sockaddr_un sck;
	struct stat st;
	time_t last_mtime = 0;
	int cmd = 0;
	int n = -1;
	char buf[4096] = {0};
	Command* c = NULL;
	if (argc < 2 ) {
		cmd = INVALID_CMD;
		fprintf(stdout, "Error args for %s\n", argv[0]);
		usage(argv);
		return 0;
	} else {
		c = gen_command(argc,argv);
	}

	if (c == NULL) {
		return 0;
	}

	printf("Command: %d,%s\n",c->argc,c->args);

	sck.sun_family = AF_UNIX;
	sck.sun_path[0] = '\0';
	/* get path */
	n = scandir("/tmp", &namelist, &ppd_dirs, NULL);
	if (n > 0) {
		while (n--) {
			snprintf(buf, 108, "/tmp/%s", namelist[n]->d_name);
			free(namelist[n]);
			if (stat(buf, &st) != 0) {
				fprintf(stderr, "%s: %s\n", buf, strerror(errno));
				continue;
			}

			if (last_mtime == 0 || last_mtime < (time_t)st.st_mtime) {
				last_mtime = (time_t)st.st_mtime;
				snprintf(sck.sun_path, 108,"%s/PowerDaemon", buf);
			}
		}
		free(namelist);
	} else {
		fprintf(stderr, "No ppd socket found\n");
		return ENOENT;
	}
	if (!sck.sun_path[0]) {
		fprintf(stderr, "No ppd socket found\n");
		return ENOENT;
	}

	if ((sock = socket(PF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket()");
		return 1;
	}

	if (connect(sock, (struct sockaddr *)&sck, sizeof(sck)) == -1) {
		perror("connect()");
		close(sock);
		return 1;
	}

	if (write(sock, c, sizeof(Command)) != sizeof(Command))
		perror("write()");

	while (read(sock, buf, 4096)) {
		printf("%s\n",buf);
	}
	printf("Done\n");
	free(c);
	close(sock);

	return 0;
}
