#include <stdlib.h>
#include <utils/Log.h>
#include <sys/time.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "ddr_hotplug.h"

JNIEXPORT jint JNICALL Java_com_marvell_ddrhotplug_DDRHotplugService_dumpMemInfo
  (JNIEnv * env, jobject thiz)
{
	char buffer[1024];
	int numFound = 0;

	int fd = open("/proc/meminfo", O_RDONLY);

	if (fd < 0) {
		LOGE("Unable to open /proc/meminfo: %s\n", strerror(errno));
		return JNI_ERR;
	}

	const int len = read(fd, buffer, sizeof(buffer)-1);
	close(fd);

	if (len < 0) {
		LOGE("Empty /proc/meminfo");
		return JNI_ERR;
	}
	buffer[len] = 0;

	static const char* const tags[] = {
			"MemTotal:",
			"MemFree:",
			"Buffers:",
			"Cached:",
			"Shmem:",
			"Slab:",
			NULL
	};
	static const int tagsLen[] = {
			9,
			8,
			8,
			7,
			6,
			5,
			0
	};
	long mem[] = { 0, 0, 0, 0, 0, 0 };

	char* p = buffer;
	while (*p && numFound < 6) {
		int i = 0;
		while (tags[i]) {
			if (strncmp(p, tags[i], tagsLen[i]) == 0) {
				p += tagsLen[i];
				while (*p == ' ') p++;
				char* num = p;
				while (*p >= '0' && *p <= '9') p++;
				if (*p != 0) {
					*p = 0;
					p++;
				}
				mem[i] = atoll(num);
				numFound++;
				break;
			}
			i++;
		}
		while (*p && *p != '\n') {
			p++;
		}
		if (*p) p++;
	}

	LOGD("RAM: %ldK total, %ldK free, %ldK buffers, %ldK cached, %ldK shmem, %ldK slab\n",
			mem[0], mem[1], mem[2], mem[3], mem[4], mem[5]);

	return  mem[1] +  mem[3];
}

/*
 * Table of methods associated with the EventRelay class.
 */
static JNINativeMethod gEventRelayMethods[] = {
    /* name, signature, funcPtr */
	{"dumpMemInfo", "()I",
		(void*)Java_com_marvell_ddrhotplug_DDRHotplugService_dumpMemInfo},
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
	if (!registerNativeMethods(env, "com/marvell/ddrhotplug/DDRHotplugService",
			gEventRelayMethods, sizeof(gEventRelayMethods) / sizeof(gEventRelayMethods[0])))
		goto bail;

    /* success -- return valid version number */
	result = JNI_VERSION_1_4;

bail:
	return result;
}

