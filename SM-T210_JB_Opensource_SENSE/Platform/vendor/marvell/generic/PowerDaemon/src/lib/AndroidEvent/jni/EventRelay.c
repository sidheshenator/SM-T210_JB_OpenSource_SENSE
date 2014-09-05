#include <stdlib.h>
#include <utils/Log.h>
#include <sys/time.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>


#include "Parser.h"
#include "ppd_socket_server.h"

#ifndef SS_DVFS_BOOSTER
#define SS_DVFS_BOOSTER
#endif

#define MAX_EXTRA_LEN 64
#define LOG_WITH_FLAG(...) \
do { \
	if (DEBUG_JNI)\
		LOGD(__VA_ARGS__); \
} while (0);

//FIXME: right now just aware these apis in Intent
struct method_table_t {
    jmethodID   get_action;         /* getAction:()Ljava/lang/String; */
    jmethodID   get_string_extra;   /* getStringExtra:(Ljava/lang/String;)Ljava/lang/String; */
    jmethodID   get_int_extra;      /* getIntExtra:(Ljava/lang/String;I)I */
    jmethodID   get_long_extra;     /* getLongExtra:(Ljava/lang/String;J)J */
    jmethodID   get_boolean_extra;  /* get_boolean_extra:(Ljava/lang/String;Z)Z */
};
typedef struct method_table_t method_table;

static int DEBUG_JNI = 0;
static method_table* meth_tbl = NULL;
static struct listnode* event_config_list = NULL;

int chip_rev = -1;

static void init_method_table(JNIEnv *env, jclass clazz)
{
	meth_tbl = (method_table*)calloc(1,sizeof(method_table));

	/* init the method */
	meth_tbl->get_action = (*env)->GetMethodID(env, clazz, "getAction", "()Ljava/lang/String;");
	meth_tbl->get_string_extra = (*env)->GetMethodID(env, clazz, "getStringExtra", "(Ljava/lang/String;)Ljava/lang/String;");
	meth_tbl->get_int_extra = (*env)->GetMethodID(env, clazz, "getIntExtra", "(Ljava/lang/String;I)I");
	meth_tbl->get_long_extra = (*env)->GetMethodID(env, clazz, "getLongExtra", "(Ljava/lang/String;J)J");
	meth_tbl->get_boolean_extra = (*env)->GetMethodID(env, clazz, "getBooleanExtra", "(Ljava/lang/String;Z)Z");
}

/*
  *  fill back the command structure by the info for each event
  *  @Parm cmd: the cmd of event
  *  @Parm event: the cmd of event
  */
static void fill_cmd_for_event(struct command_t* cmd, Event* event) {
	switch (cmd->tag) {
		case TAG_ANDROID:
			if (event->event_type == STATUS_EVENT) {
				cmd->plugin_data.android_data.ev_data.state = event->ev_data.state;
			} else if (event->event_type == CHAR_EVENT) {
				memcpy(cmd->plugin_data.android_data.ev_data.extra,
					event->ev_data.extra, strlen(event->ev_data.extra));
			} else {
				LOGW("Unsupport event type! New Type?\n");
			}
			break;

		case TAG_THERMAL:
			cmd->plugin_data.thermal_data.state = event->ev_data.state;
			break;

		case TAG_DDR_HOTPLUG:
			cmd->plugin_data.ddr_hotplug_data.state = event->ev_data.state;
			break;

		default:
			//TO DO: event without extra in other plugins
			LOGW("Hey, is this a legal event without extra in plugins: %d", cmd->tag);
	}

}

/*
  *  gen the command structure if don`t exist by the info for intent
  *  @Param cmd: the cmd of event
  *  @Param event: the cmd of event
  */
static Command* make_command_for_intent( JNIEnv *env, jobject intent,const char* action_name)
{
	LOG_WITH_FLAG("make_command_for_intent %s!", action_name);
	struct command_t* cmd = NULL;
	Event* ev = NULL;
	struct listnode *node = NULL;

	list_for_each(node, event_config_list)
	{
		ev = node_to_item(node, Event, link);
		if (strcmp(ev->intent_info, action_name) == 0) {//found
			/* event without extra*/
			if (ev->extra_info == NULL) {
				LOG_WITH_FLAG("CMD without extra!");
				cmd = (struct command_t*)ev->cmd;
				fill_cmd_for_event(cmd, ev);
				return (Command *)cmd;
			}

			/* event with extra */
			char value[MAX_EXTRA_LEN];
			memset(value,0,MAX_EXTRA_LEN);
			jstring key = (*env)->NewStringUTF(env, ev->extra_info->extra_name);

			/* FIXME: some more extra types should be added */
			switch (ev->extra_info->extra_type) {
				case EXTRA_NONE:
					LOGE("How could EXTRA_NONE appear here!!!");
					break;

				case EXTRA_STRINGS:
					LOG_WITH_FLAG("String Extra!!!");
					jstring sstate = (*env)->CallObjectMethod(env, intent,
							meth_tbl->get_string_extra, key);
					const char *state_utf = (*env)->GetStringUTFChars(env, sstate, NULL);
					strncpy(value, state_utf, MAX_EXTRA_LEN);
					(*env)->ReleaseStringUTFChars(env, sstate, state_utf);
					break;

				case EXTRA_INT:
					LOG_WITH_FLAG("Int Extra!!!");
					jint istate = (*env)->CallObjectMethod(env, intent,
							meth_tbl->get_int_extra, key, (jint)-1);
					snprintf(value, MAX_EXTRA_LEN, "%d", istate);
					break;

				case EXTRA_LONG:
					LOG_WITH_FLAG("Long Extra!!!");
					jlong lstate = (*env)->CallObjectMethod(env, intent,
							meth_tbl->get_long_extra, key, (jlong)-1);
					snprintf(value, MAX_EXTRA_LEN, "%lld", (long long)lstate);
					break;

				case EXTRA_BOOLEAN:
					LOG_WITH_FLAG("Boolean Extra!!!");
					jboolean bstate = (*env)->CallObjectMethod(env, intent,
							meth_tbl->get_boolean_extra, key, (jboolean)0);
					snprintf(value, MAX_EXTRA_LEN, "%s", bstate?"true":"false");
					break;

				default:
					LOGE("Unsupported extra type!!!");
			}

            /*  NOTICE: Case like monitor task,which has no value for extra
             *  in config. That means we concern all the extras if it has.
             */
			if (strcmp(ev->extra_info->extra_name, "taskName") == 0) {
				cmd = (struct command_t*)ev->cmd;
				/*Should only be TAG_ANDROID*/
				memset(cmd->plugin_data.android_data.ev_data.extra, 0, MAX_ATTR_LEN);
				strncpy(cmd->plugin_data.android_data.ev_data.extra, value, MAX_ATTR_LEN);
				return (Command *)cmd;
			}

			/* General case: found */
			if (strcmp(value, ev->extra_info->extra_value) == 0) {
				cmd = (struct command_t*)ev->cmd;
				fill_cmd_for_event(cmd, ev);
				return (Command *)cmd;
			}
		}
	}
    /* Not found! */
	return NULL;
}

/*
 * Class:     com_marvell_ppdgadget_EventRelay
 * Method:    openConnectionToSocketSrv
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_marvell_ppdgadget_EventRelay_openConnectionToSocketSrv
  (JNIEnv * env, jobject thiz)
{
	/* open the socket */
	int srvSocket_fd = ppd_socket_open();
	if (srvSocket_fd < 0) {
		LOGE("Failed to open the server socket!");
		return JNI_ERR;
	}

	/* init the event config */
	event_config_list = (struct listnode *)calloc(1,sizeof(struct listnode));
	list_init(event_config_list);

	int ret = init_event_config(event_config_list);
	if (ret < 0) {
		LOGE("Failed to init event config!!!");
		return JNI_ERR;
	}

	/* init the method table */
	jclass intent = (*env)->FindClass(env,"android/content/Intent");
	if (intent == NULL)
		LOGE("Failed to find Class Intent!!!");

	init_method_table(env,intent);
	return (jint)srvSocket_fd;
}

/*
 * Class:     com_marvell_ppdgadget_EventRelay
 * Method:    closeConnectionToSocketSrv
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_marvell_ppdgadget_EventRelay_closeConnectionToSocketSrv
  (JNIEnv * env, jobject thiz, jint srvSocket_fd)
{
	int ret = ppd_socket_close((int)srvSocket_fd);
	if (ret < 0) {
		LOGE("Failed to close the connection!");
		free(meth_tbl);
		return JNI_ERR;
	}

	free(meth_tbl);
	free_config_list(event_config_list);
	return JNI_OK;
}

/*DEBUG*/
static void dump_command(Command* c)
{
	struct command_t* cmd = &(c->cmd);
	LOGD("=============================\n");
	LOGD("CMD: len %d, tag: %d", cmd->len, cmd->tag);
	if (cmd->tag == TAG_ANDROID) { //Android
		LOGD("%s", cmd->plugin_data.android_data.keyword);
		if (cmd->plugin_data.android_data.ev_type== STATUS_EVENT) {
			LOGD("Status: %d", cmd->plugin_data.android_data.ev_data.state);
		} else {
			LOGD("Extra: %s", cmd->plugin_data.android_data.ev_data.extra);
		}
	} else if (cmd->tag == TAG_THERMAL) {  //Thermal
		LOGD("Thermal state: %d", cmd->plugin_data.thermal_data.state);
		LOGD("Thermal temp: %d", cmd->plugin_data.thermal_data.temperature);
	} else if (cmd->tag == TAG_DDR_HOTPLUG) { //DDR Hotplug
		LOGD("Hotplug state: %d", cmd->plugin_data.ddr_hotplug_data.state);
	}
	LOGD("=============================\n");
}

/*
 * Class:     com_marvell_ppdgadget_EventRelay
 * Method:    sendCmdToSocketSrv
 * Signature: (Landroid/content/Intent;I)V
 */
JNIEXPORT void JNICALL Java_com_marvell_ppdgadget_EventRelay_sendCmdToSocketSrv
  (JNIEnv * env, jobject thiz, jobject intent, jint srvSocket_fd)
{
	if (meth_tbl == NULL) {
		LOGE("Method table init failed?");
		return;
	}

	jstring action = (*env)->CallObjectMethod(env, intent, meth_tbl->get_action);
	if (action == NULL) {
		LOGE("Call getAction of Intent return NULL!!!");
		return;
	}

	const char *intent_action = (*env)->GetStringUTFChars(env, action, NULL);
	Command *cmd = make_command_for_intent(env, intent,intent_action);

	if (DEBUG_JNI)
		dump_command(cmd);

	if (cmd == NULL) {
		LOGE("Could find suitable cmd for intent, Any problem in config???");
		(*env)->ReleaseStringUTFChars(env, action, intent_action);
		return;
	}

	int ret = ppd_socket_write(srvSocket_fd, cmd);
	if (ret < 0) {
		LOGE("Failed to write the cmd to socket!");
	}

	(*env)->ReleaseStringUTFChars(env, action, intent_action);
}

JNIEXPORT jint JNICALL Java_com_marvell_ppdgadget_EventRelay_getChipRevison
  (JNIEnv * env, jobject thiz)
{
	FILE *fp;
	char line[256];
	const char *split = ": ";
	unsigned int rev_num = 0;

	fp = fopen("/proc/cpuinfo", "r");
	if(!fp) {
		LOGE("Cannot open cpuinfo file: %s\n", strerror(errno));
		return JNI_ERR;
	}

	while(!feof(fp)) {
		fgets(line, 255, fp);
		if(!strncmp(line, "Revision", 8)) {
			char *rev = strtok(line,split);
			if(rev != NULL) {
				rev = strtok(NULL, split);
			}

			if(rev != NULL) {
				rev_num = atoi(rev);
			}
			break;
		}
	}
	fclose(fp);

	LOGD("chip revision number is %i \n", rev_num);
	chip_rev = rev_num;
	return rev_num;
}

/*
 * SS API
 * FIXME: now just DVFS_BOOSTER.
 */
#ifdef SS_DVFS_BOOSTER
JNIEXPORT void JNICALL Java_com_marvell_ppdgadget_EventRelay_sendStateCmdToSocketSrv
  (JNIEnv *env, jobject thiz, jint status, jint srvSocket_fd)
{
	struct command_t* cmd = (struct command_t*)calloc(1, sizeof(struct command_t));
	cmd->tag = TAG_ANDROID;
	cmd->len = sizeof(struct android_event);
	strncpy(cmd->plugin_data.android_data.keyword, "booster", 7);
	cmd->plugin_data.android_data.ev_type = STATUS_EVENT;
	cmd->plugin_data.android_data.ev_data.state = status;

	int ret = ppd_socket_write(srvSocket_fd, cmd);
	if (ret < 0) {
		LOGE("Failed to write the cmd to socket!");
	}
}
#endif

/*
 * Table of methods associated with the EventRelay class.
 */
static JNINativeMethod gEventRelayMethods[] = {
    /* name, signature, funcPtr */
	{"sendCmdToSocketSrv", "(Landroid/content/Intent;I)V",
		(void*)Java_com_marvell_ppdgadget_EventRelay_sendCmdToSocketSrv},
#ifdef SS_DVFS_BOOSTER
	{"sendStateCmdToSocketSrv", "(II)V",
		(void*)Java_com_marvell_ppdgadget_EventRelay_sendStateCmdToSocketSrv},
#endif
	{"openConnectionToSocketSrv", "()I",
		(void*)Java_com_marvell_ppdgadget_EventRelay_openConnectionToSocketSrv},
	{"closeConnectionToSocketSrv", "(I)I",
		(void*)Java_com_marvell_ppdgadget_EventRelay_closeConnectionToSocketSrv},
	{"getChipRevison", "()I",
		(void*)Java_com_marvell_ppdgadget_EventRelay_getChipRevison},
};

/*
 * Register several native methods for EventRelay.
 */
static int registerNativeMethods(JNIEnv* env, const char* className,
    JNINativeMethod* gMethods, int numMethods)
{
	jclass clazz;

	clazz = (*env)->FindClass(env, className);
	if (clazz == NULL)
		return JNI_FALSE;

	if ((*env)->RegisterNatives(env, clazz, gMethods, numMethods) < 0)
		return JNI_FALSE;

	return JNI_TRUE;
}

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
	JNIEnv* env = NULL;
	jint result = -1;

	if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4) != JNI_OK)
		goto bail;

	assert(env != NULL);
	if (!registerNativeMethods(env, "com/marvell/ppdgadget/EventRelay",
			gEventRelayMethods, sizeof(gEventRelayMethods) / sizeof(gEventRelayMethods[0])))
		goto bail;

    /* success -- return valid version number */
	result = JNI_VERSION_1_4;

bail:
	return result;
}

