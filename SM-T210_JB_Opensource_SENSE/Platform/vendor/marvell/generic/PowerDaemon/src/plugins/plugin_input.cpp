/*
 *  Copyright (C) 2002-2008  Mattia Dongili <malattia@linux.it>
 *                           George Staikos <staikos@0wned.org>
 *
 *  Copyright (C) 2011 Marvell, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <EventHub.h>

extern "C"
{
#include "ppd_component.h"
#include "plugin_tags.h"
}


#define SIGCANCEL SIGUSR1
#define INPUTEVENT_EXPIRE_TIME 800
#define INPUT_EVENT_CLASS (INPUT_DEVICE_CLASS_KEYBOARD | INPUT_DEVICE_CLASS_ALPHAKEY | INPUT_DEVICE_CLASS_DPAD | INPUT_DEVICE_CLASS_CURSOR | INPUT_DEVICE_CLASS_TOUCH_MT | INPUT_DEVICE_CLASS_TOUCH)


using namespace android;

static pthread_t event_thread;
static struct sigaction prev_action;


struct __input_event{
	int value;
	time_t time;
};
struct __input_event inputEvent;


static void sigcancel_handle(int signum)
{
	if (signum == SIGCANCEL)
		pthread_exit(NULL);
}

void timerHandle(sigval_t v)
{
	clog(LOG_DEBUG, "input event timed out!");
	inputEvent.value = 0;
	wakeup_ppd();

	return;
}


int startTimer(long milliseconds, timer_t tid)
{
	struct itimerspec ts, ots;
	long seconds, nseconds;

	seconds = milliseconds/1000;
	nseconds = (milliseconds)*1000*1000;

	ts.it_value.tv_sec = seconds;
	ts.it_value.tv_nsec = nseconds;
	ts.it_interval.tv_sec = 0;
	ts.it_interval.tv_nsec = 0;
	if (timer_settime (tid, 0, &ts, &ots) < 0)
	{
		clog(LOG_ERR, "timer_settime");
		return -1;
	}
	return 0;

}


timer_t createTimer(int id)
{
	timer_t tid;
	struct sigevent se;

	memset(&se, 0, sizeof (se));
	se.sigev_notify = SIGEV_THREAD;
	se.sigev_notify_function = timerHandle;
	se.sigev_value.sival_int = id;

	if (timer_create (CLOCK_REALTIME, &se, &tid) < 0)
	{
		clog(LOG_ERR, "timer_create failed");
		return -1;
	}
	clog(LOG_INFO, "timer_create successfully.");

	return tid;
}

/*  Waits for android events on the file descriptor opened previously.
 *  This function uses poll(2) to wait for readable data.
 */
static void *event_wait (void __UNUSED__ *arg) {

	sp<EventHub> eventHub = new EventHub();
	static const int EVENT_BUFFER_SIZE = 256;
	RawEvent mEventBuffer[EVENT_BUFFER_SIZE];

	clog(LOG_INFO, "input event thread running.");
	timer_t tid = createTimer(1);
	while (1) {
		size_t count = eventHub->getEvents(-1, mEventBuffer, EVENT_BUFFER_SIZE);
		clog(LOG_DEBUG, "get %d input event.", count);
		int found = 0;
		while(count){
			RawEvent* rawEvent = mEventBuffer;
			int32_t type = rawEvent->type;
			if (type < EventHubInterface::FIRST_SYNTHETIC_EVENT) {
				int32_t deviceId = rawEvent->deviceId;
				uint32_t classes = eventHub->getDeviceClasses(deviceId);
				if(classes & INPUT_EVENT_CLASS){
					clog(LOG_DEBUG, "Input event: device=0x%x type=0x%x code=%d value=%d",
							rawEvent->deviceId, rawEvent->type, rawEvent->code, rawEvent->value);
					found = 1;
					break;
				}
			}
			count--;
			rawEvent++;
		}

		if(found) {
			clog(LOG_DEBUG, "start/refresh timer");
			startTimer(INPUTEVENT_EXPIRE_TIME, tid);
			if(inputEvent.value == 0) {
				inputEvent.value = 1;
				clog(LOG_NOTICE, "input thread wakeup ppd");
				wakeup_ppd();
			}
		}
	}
	return NULL;
}
/*  static int input_event_init(void)
 *
 *  create a thread to read input event
 */
static int input_event_init(void) {

	struct sigaction act;
	int ret;

	memset(&inputEvent, 0, sizeof(inputEvent));
	act.sa_handler = sigcancel_handle;
	act.sa_flags = SA_ONESHOT;
	sigemptyset(&act.sa_mask);
	sigaction(SIGCANCEL, &act, &prev_action);

	/* launch event thread */
	if ((ret = pthread_create(&event_thread, NULL, &event_wait, NULL))) {
		clog(LOG_ERR, "Unable to launch thread: %s\n", strerror(ret));
		sigaction(SIGCANCEL, &prev_action, NULL);
		return -1;
	}
	sigaddset(&act.sa_mask, SIGCANCEL);
	return pthread_sigmask(SIG_BLOCK, &act.sa_mask, NULL);

}

static int input_event_exit(void) {
	if (event_thread) {
		clog(LOG_DEBUG, "killing event thread.\n");
		int ret = pthread_kill(event_thread, SIGCANCEL);
		if (ret != 0)
			clog(LOG_ERR, "Couldn't cancel event thread (%s).\n",
					strerror(ret));
		/* cleanup */
		ret = pthread_join(event_thread, NULL);
		if (ret != 0)
			clog(LOG_ERR, "Couldn't join event thread (%s).\n",
					strerror(ret));
		event_thread = 0;
	}

	sigaction(SIGCANCEL, &prev_action, NULL);
	clog(LOG_INFO, "android_event exited.\n");
	return 0;

}


/*
 *  parse the 'input_event' keywork
 */
static int input_event_parse(const char *ev, void **obj) {
	int *ret = (int*)malloc(sizeof(int));
	if (ret == NULL) {
		clog(LOG_ERR, "couldn't make enough room for input_event (%s)\n",
				strerror(errno));
		return -1;
	}

	*ret = 0;

	if (strncmp(ev, "1", 1) == 0) {
		*ret = 1;
	} else if (strncmp(ev, "0", 1) == 0) {
		*ret = 0;
	} else {
		clog(LOG_ERR, "couldn't parse %s\n", ev);
		free(ret);
		return -1;
	}

	clog(LOG_DEBUG, "parsed: %s\n", *ret==1 ? "1" : "0");

	*obj = ret;
	return 0;
}

/*
 *  evaluate the 'input_event' keyword
 */
static int input_event_evaluate(const void *s) {
	const int *input = (const int *)s;

	clog(LOG_DEBUG, "called: rule_val=%d [input=%d]\n", *input, inputEvent.value);

	return (*input == inputEvent.value) ? MATCH : DONT_MATCH;
}

static PowerPolicyDaemonKeyword kw[] = {
	{ "input_event", 0, 1, &input_event_parse, &input_event_evaluate, NULL, NULL, NULL, NULL, NULL},
	{ NULL, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};

static PowerPolicyDaemonPlugin input_event = {
	"input",
	TAG_INPUT,
	kw,
	&input_event_init,
	&input_event_exit,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

PowerPolicyDaemonPlugin *create_plugin (void) {
	return &input_event;
}
