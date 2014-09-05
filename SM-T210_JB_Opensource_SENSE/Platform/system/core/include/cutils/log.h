/*
 * Copyright (C) 2005 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//
// C/C++ logging functions.  See the logging documentation for API details.
//
// We'd like these to be available from C code (in case we import some from
// somewhere), so this has a C interface.
//
// The output will be correct when the log file is shared between multiple
// threads and/or multiple processes so long as the operating system
// supports O_APPEND.  These calls have mutex-protected data structures
// and so are NOT reentrant.  Do not use LOG in a signal handler.
//
#ifndef _LIBS_CUTILS_LOG_H
#define _LIBS_CUTILS_LOG_H

#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef HAVE_PTHREADS
#include <pthread.h>
#endif
#include <stdarg.h>

#include <cutils/uio.h>
#include <cutils/logd.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------

/*
 * Normally we strip ALOGV (VERBOSE messages) from release builds.
 * You can modify this (for example with "#define LOG_NDEBUG 0"
 * at the top of your source file) to change that behavior.
 */
#ifndef LOG_NDEBUG
#ifdef NDEBUG
#define LOG_NDEBUG 1
#else
#define LOG_NDEBUG 0
#endif
#endif

/*
 * This is the local tag used for the following simplified
 * logging macros.  You can change this preprocessor definition
 * before using the other macros to change the tag.
 */
#ifndef LOG_TAG
#define LOG_TAG NULL
#endif

// ---------------------------------------------------------------------

/*
 * Previous simplified macro to send a verbose log message using the current LOG_TAG.
 */
#ifndef LOGV
#if LOG_NDEBUG
#define LOGV(...)   ((void)0)
#else
#define LOGV ALOGV
#endif
#endif

#ifndef LOGV_IF
#if LOG_NDEBUG
#define LOGV_IF(cond, ...)   ((void)0)
#else
#define LOGV_IF ALOG_IF
#endif
#endif

/*
 * Simplified macro to send a debug log message using the current LOG_TAG.
 */
#ifndef LOGD
#define LOGD ALOGD
#endif

#ifndef LOGD_IF
#define LOGD_IF ALOGD_IF
#endif

/*
 * Simplified macro to send an info log message using the current LOG_TAG.
 */
#ifndef LOGI
#define LOGI ALOGI
#endif

#ifndef LOGI_IF
#define LOGI_IF ALOGI_IF
#endif

/*
 * Simplified macro to send a warning log message using the current LOG_TAG.
 */
#ifndef LOGW
#define LOGW ALOGW
#endif

#ifndef LOGW_IF
#define LOGW_IF ALOGW_IF
#endif

/*
 * Simplified macro to send an error log message using the current LOG_TAG.
 */
#ifndef LOGE
#define LOGE ALOGE
#endif

#ifndef LOGE_IF
#define LOGE_IF ALOGE_IF
#endif

// ---------------------------------------------------------------------

/*
 * Conditional based on whether the current LOG_TAG is enabled at
 * verbose priority.
 */
#ifndef IF_LOGV
#if LOG_NDEBUG
#define IF_LOGV() if (false)
#else
#define IF_LOGV IF_ALOGV
#endif
#endif

/*
 * Conditional based on whether the current LOG_TAG is enabled at
 * debug priority.
 */
#ifndef IF_LOGD
#define IF_LOGD IF_ALOGD
#endif

/*
 * Conditional based on whether the current LOG_TAG is enabled at
 * info priority.
 */
#ifndef IF_LOGI
#define IF_LOGI IF_ALOGI
#endif

/*
 * Conditional based on whether the current LOG_TAG is enabled at
 * warn priority.
 */
#ifndef IF_LOGW
#define IF_LOGW IF_ALOGW
#endif

/*
 * Conditional based on whether the current LOG_TAG is enabled at
 * error priority.
 */
#ifndef IF_LOGE
#define IF_LOGE IF_ALOGE
#endif


// ---------------------------------------------------------------------


// ---------------------------------------------------------------------

/*
 * Simplified macro to send a verbose log message using the current LOG_TAG.
 */
#ifndef ALOGV
#if LOG_NDEBUG
#define ALOGV(...)   ((void)0)
#else
#define ALOGV(...) ((void)ALOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#endif
#endif

#ifndef SEC_ALOGV
#if LOG_NDEBUG
#define SEC_ALOGV(...)   ((void)0)
#else
#define SEC_ALOGV(...) ((void)SEC_ALOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#endif
#endif


#define CONDITION(cond)     (__builtin_expect((cond)!=0, 0))

#ifndef ALOGV_IF
#if LOG_NDEBUG
#define ALOGV_IF(cond, ...)   ((void)0)
#else
#define ALOGV_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)ALOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif
#endif

#ifndef SEC_ALOGV_IF
#if LOG_NDEBUG
#define SEC_ALOGV_IF(cond, ...)   ((void)0)
#else
#define SEC_ALOGV_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)SEC_ALOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif
#endif

/*
 * Simplified macro to send a debug log message using the current LOG_TAG.
 */
#ifndef ALOGD
#define ALOGD(...) ((void)ALOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#endif

#ifndef SEC_ALOGD
#define SEC_ALOGD(...) ((void)SEC_ALOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#endif


#ifndef ALOGD_IF
#define ALOGD_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)ALOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

#ifndef SEC_ALOGD_IF
#define SEC_ALOGD_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)SEC_ALOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif


/*
 * Simplified macro to send an info log message using the current LOG_TAG.
 */
#ifndef ALOGI
#define ALOGI(...) ((void)ALOG(LOG_INFO, LOG_TAG, __VA_ARGS__))
#endif

#ifndef ALOGI_IF
#define ALOGI_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)ALOG(LOG_INFO, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif


#ifndef SEC_ALOGI
#define SEC_ALOGI(...) ((void)SEC_ALOG(LOG_INFO, LOG_TAG, __VA_ARGS__))
#endif

#ifndef SEC_ALOGI_IF
#define SEC_ALOGI_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)SEC_ALOG(LOG_INFO, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif


/*
 * Simplified macro to send a warning log message using the current LOG_TAG.
 */
#ifndef ALOGW
#define ALOGW(...) ((void)ALOG(LOG_WARN, LOG_TAG, __VA_ARGS__))
#endif

#ifndef ALOGW_IF
#define ALOGW_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)ALOG(LOG_WARN, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

#ifndef SEC_ALOGW
#define SEC_ALOGW(...) ((void)SEC_ALOG(LOG_WARN, LOG_TAG, __VA_ARGS__))
#endif

#ifndef SEC_ALOGW_IF
#define SEC_ALOGW_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)SEC_ALOG(LOG_WARN, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif


/*
 * Simplified macro to send an error log message using the current LOG_TAG.
 */
#ifndef ALOGE
#define ALOGE(...) ((void)ALOG(LOG_ERROR, LOG_TAG, __VA_ARGS__))
#endif

#ifndef ALOGE_IF
#define ALOGE_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)ALOG(LOG_ERROR, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

#ifndef SEC_ALOGE
#define SEC_ALOGE(...) ((void)SEC_ALOG(LOG_ERROR, LOG_TAG, __VA_ARGS__))
#endif

#ifndef SEC_ALOGE_IF
#define SEC_ALOGE_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)SEC_ALOG(LOG_ERROR, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif


// ---------------------------------------------------------------------

/*
 * Conditional based on whether the current LOG_TAG is enabled at
 * verbose priority.
 */
#ifndef IF_ALOGV
#if LOG_NDEBUG
#define IF_ALOGV() if (false)
#else
#define IF_ALOGV() IF_ALOG(LOG_VERBOSE, LOG_TAG)
#endif
#endif

#ifndef SEC_IF_ALOGV
#if LOG_NDEBUG
#define SEC_IF_ALOGV() if (false)
#else
#define SEC_IF_ALOGV() SEC_IF_ALOG(LOG_VERBOSE, LOG_TAG)
#endif
#endif

/*
 * Conditional based on whether the current LOG_TAG is enabled at
 * debug priority.
 */
#ifndef IF_ALOGD
#define IF_ALOGD() IF_ALOG(LOG_DEBUG, LOG_TAG)
#endif

#ifndef SEC_IF_ALOGD
#define SEC_IF_ALOGD() SEC_IF_ALOG(LOG_DEBUG, LOG_TAG)
#endif


/*
 * Conditional based on whether the current LOG_TAG is enabled at
 * info priority.
 */
#ifndef IF_ALOGI
#define IF_ALOGI() IF_ALOG(LOG_INFO, LOG_TAG)
#endif

#ifndef SEC_IF_ALOGI
#define SEC_IF_ALOGI() SEC_IF_ALOG(LOG_INFO, LOG_TAG)
#endif


/*
 * Conditional based on whether the current LOG_TAG is enabled at
 * warn priority.
 */
#ifndef IF_ALOGW
#define IF_ALOGW() IF_ALOG(LOG_WARN, LOG_TAG)
#endif

#ifndef SEC_IF_ALOGW
#define SEC_IF_ALOGW() SEC_IF_ALOG(LOG_WARN, LOG_TAG)
#endif

/*
 * Conditional based on whether the current LOG_TAG is enabled at
 * error priority.
 */
#ifndef IF_ALOGE
#define IF_ALOGE() IF_ALOG(LOG_ERROR, LOG_TAG)
#endif

#ifndef SEC_IF_ALOGE
#define SEC_IF_ALOGE() SEC_IF_ALOG(LOG_ERROR, LOG_TAG)
#endif


#define SEC_ALOG_BUF_PRINT   __android_log_switchable_buf_print  

#if USE_SWITCHABLE_LOG
#define ALOG_BUF_PRINT	SEC_ALOG_BUF_PRINT
#else
#define ALOG_BUF_PRINT	__android_log_buf_print
#endif

// ---------------------------------------------------------------------

/*
 * Simplified macro to send a verbose system log message using the current LOG_TAG.
 */
#ifndef SLOGV
#if LOG_NDEBUG
#define SLOGV(...)   ((void)0)
#else
#define SLOGV(...) ((void)ALOG_BUF_PRINT(LOG_ID_SYSTEM, ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#endif
#endif

#ifndef SEC_SLOGV
#if LOG_NDEBUG
#define SEC_SLOGV(...)   ((void)0)
#else
#define SEC_SLOGV(...) ((void)SEC_ALOG_BUF_PRINT(LOG_ID_SYSTEM, ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#endif
#endif


#define CONDITION(cond)     (__builtin_expect((cond)!=0, 0))

#ifndef SLOGV_IF
#if LOG_NDEBUG
#define SLOGV_IF(cond, ...)   ((void)0)
#else
#define SLOGV_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)ALOG_BUF_PRINT(LOG_ID_SYSTEM, ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif
#endif

#ifndef SEC_SLOGV_IF
#if LOG_NDEBUG
#define SEC_SLOGV_IF(cond, ...)   ((void)0)
#else
#define SEC_SLOGV_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)SEC_ALOG_BUF_PRINT(LOG_ID_SYSTEM, ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif
#endif

/*
 * Simplified macro to send a debug system log message using the current LOG_TAG.
 */
#ifndef SLOGD
#define SLOGD(...) ((void)ALOG_BUF_PRINT(LOG_ID_SYSTEM, ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#endif

#ifndef SLOGD_IF
#define SLOGD_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)ALOG_BUF_PRINT(LOG_ID_SYSTEM, ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif


#ifndef SEC_SLOGD
#define SEC_SLOGD(...) ((void)SEC_ALOG_BUF_PRINT(LOG_ID_SYSTEM, ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#endif

#ifndef SEC_SLOGD_IF
#define SEC_SLOGD_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)SEC_ALOG_BUF_PRINT(LOG_ID_SYSTEM, ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif


/*
 * Simplified macro to send an info system log message using the current LOG_TAG.
 */
#ifndef SLOGI
#define SLOGI(...) ((void)ALOG_BUF_PRINT(LOG_ID_SYSTEM, ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#endif

#ifndef SLOGI_IF
#define SLOGI_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)ALOG_BUF_PRINT(LOG_ID_SYSTEM, ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

#ifndef SEC_SLOGI
#define SEC_SLOGI(...) ((void)SEC_ALOG_BUF_PRINT(LOG_ID_SYSTEM, ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#endif

#ifndef SEC_SLOGI_IF
#define SEC_SLOGI_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)SEC_ALOG_BUF_PRINT(LOG_ID_SYSTEM, ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif


/*
 * Simplified macro to send a warning system log message using the current LOG_TAG.
 */
#ifndef SLOGW
#define SLOGW(...) ((void)ALOG_BUF_PRINT(LOG_ID_SYSTEM, ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__))
#endif

#ifndef SLOGW_IF
#define SLOGW_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)ALOG_BUF_PRINT(LOG_ID_SYSTEM, ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

#ifndef SEC_SLOGW
#define SEC_SLOGW(...) ((void)SEC_ALOG_BUF_PRINT(LOG_ID_SYSTEM, ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__))
#endif

#ifndef SEC_SLOGW_IF
#define SEC_SLOGW_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)SEC_ALOG_BUF_PRINT(LOG_ID_SYSTEM, ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif


/*
 * Simplified macro to send an error system log message using the current LOG_TAG.
 */
#ifndef SLOGE
#define SLOGE(...) ((void)ALOG_BUF_PRINT(LOG_ID_SYSTEM, ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))
#endif

#ifndef SLOGE_IF
#define SLOGE_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)ALOG_BUF_PRINT(LOG_ID_SYSTEM, ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif


#ifndef SEC_SLOGE
#define SEC_SLOGE(...) ((void)SEC_ALOG_BUF_PRINT(LOG_ID_SYSTEM, ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))
#endif

#ifndef SEC_SLOGE_IF
#define SEC_SLOGE_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)SEC_ALOG_BUF_PRINT(LOG_ID_SYSTEM, ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

    

// ---------------------------------------------------------------------

/*
  * Simplified macro to send a verbose surfaceflinger log message using the current LOG_TAG.
  */
 #ifndef SFLOGV
 #if LOG_NDEBUG
 #define SFLOGV(...)   ((void)0)
 #else
 #define SFLOGV(...) ((void)__android_log_buf_print(LOG_ID_SF, ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
 #endif
 #endif
 
 #define CONDITION(cond)	 (__builtin_expect((cond)!=0, 0))
 
 #ifndef SFLOGV_IF
 #if LOG_NDEBUG
 #define SFLOGV_IF(cond, ...)	((void)0)
 #else
 #define SFLOGV_IF(cond, ...) \
	 ( (CONDITION(cond)) \
	 ? ((void)__android_log_buf_print(LOG_ID_SF, ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)) \
	 : (void)0 )
 #endif
 #endif
 
 /*
  * Simplified macro to send a debug surfaceflinger log message using the current LOG_TAG.
  */
 #ifndef SFLOGD
 #define SFLOGD(...) ((void)__android_log_buf_print(LOG_ID_SF, ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
 #endif
 
 #ifndef SFLOGD_IF
 #define SFLOGD_IF(cond, ...) \
	 ( (CONDITION(cond)) \
	 ? ((void)__android_log_buf_print(LOG_ID_SF, ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)) \
	 : (void)0 )
 #endif
 
 /*
  * Simplified macro to send an info surfaceflinger log message using the current LOG_TAG.
  */
 #ifndef SFLOGI
 #define SFLOGI(...) ((void)__android_log_buf_print(LOG_ID_SF, ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
 #endif
 
 #ifndef SFLOGI_IF
 #define SFLOGI_IF(cond, ...) \
	 ( (CONDITION(cond)) \
	 ? ((void)__android_log_buf_print(LOG_ID_SF, ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)) \
	 : (void)0 )
 #endif
 
 /*
  * Simplified macro to send a warning surfaceflinger log message using the current LOG_TAG.
  */
 #ifndef SFLOGW
 #define SFLOGW(...) ((void)__android_log_buf_print(LOG_ID_SF, ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__))
 #endif
 
 #ifndef SFLOGW_IF
 #define SFLOGW_IF(cond, ...) \
	 ( (CONDITION(cond)) \
	 ? ((void)__android_log_buf_print(LOG_ID_SF, ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)) \
	 : (void)0 )
 #endif
 
 /*
  * Simplified macro to send an error surfaceflinger log message using the current LOG_TAG.
  */
 #ifndef SFLOGE
 #define SFLOGE(...) ((void)__android_log_buf_print(LOG_ID_SF, ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))
 #endif
 
 #ifndef SFLOGE_IF
 #define SFLOGE_IF(cond, ...) \
	 ( (CONDITION(cond)) \
	 ? ((void)__android_log_buf_print(LOG_ID_SF, ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)) \
	 : (void)0 )
 #endif
 
 // ---------------------------------------------------------------------
 

/*
 * Log a fatal error.  If the given condition fails, this stops program
 * execution like a normal assertion, but also generating the given message.
 * It is NOT stripped from release builds.  Note that the condition test
 * is -inverted- from the normal assert() semantics.
 */
#ifndef LOG_ALWAYS_FATAL_IF
#define LOG_ALWAYS_FATAL_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)android_printAssert(#cond, LOG_TAG, ## __VA_ARGS__)) \
    : (void)0 )
#endif

#ifndef LOG_ALWAYS_FATAL
#define LOG_ALWAYS_FATAL(...) \
    ( ((void)android_printAssert(NULL, LOG_TAG, ## __VA_ARGS__)) )
#endif

/*
 * Versions of LOG_ALWAYS_FATAL_IF and LOG_ALWAYS_FATAL that
 * are stripped out of release builds.
 */
#if LOG_NDEBUG

#ifndef LOG_FATAL_IF
#define LOG_FATAL_IF(cond, ...) ((void)0)
#endif
#ifndef LOG_FATAL
#define LOG_FATAL(...) ((void)0)
#endif

#else

#ifndef LOG_FATAL_IF
#define LOG_FATAL_IF(cond, ...) LOG_ALWAYS_FATAL_IF(cond, ## __VA_ARGS__)
#endif
#ifndef LOG_FATAL
#define LOG_FATAL(...) LOG_ALWAYS_FATAL(__VA_ARGS__)
#endif

#endif

/*
 * Assertion that generates a log message when the assertion fails.
 * Stripped out of release builds.  Uses the current LOG_TAG.
 */
#ifndef ALOG_ASSERT
#define ALOG_ASSERT(cond, ...) LOG_FATAL_IF(!(cond), ## __VA_ARGS__)
//#define ALOG_ASSERT(cond) LOG_FATAL_IF(!(cond), "Assertion failed: " #cond)
#endif

// ---------------------------------------------------------------------

/*
 * Basic log message macro.
 *
 * Example:
 *  ALOG(LOG_WARN, NULL, "Failed with error %d", errno);
 *
 * The second argument may be NULL or "" to indicate the "global" tag.
 */
#ifndef ALOG
#define ALOG(priority, tag, ...) \
    LOG_PRI(ANDROID_##priority, tag, __VA_ARGS__)
#endif

#ifndef SEC_ALOG
#define SEC_ALOG(priority, tag, ...) \
    SEC_ALOG_PRI(ANDROID_##priority, tag, __VA_ARGS__)
#endif


/*
 * Log macro that allows you to specify a number for the priority.
 */
#ifndef LOG_PRI
#define LOG_PRI(priority, tag, ...) \
    android_printLog(priority, tag, __VA_ARGS__)
#endif

#ifndef SEC_LOG_PRI
#define SEC_LOG_PRI(priority, tag, ...) \
    __android_log_switchable_print(priority, tag, __VA_ARGS__)
#endif

/*
 * Log macro that allows you to pass in a varargs ("args" is a va_list).
 */
#ifndef LOG_PRI_VA
#define LOG_PRI_VA(priority, tag, fmt, args) \
    android_vprintLog(priority, NULL, tag, fmt, args)
#endif

/*
 * Conditional given a desired logging priority and tag.
 */
#ifndef IF_ALOG
#define IF_ALOG(priority, tag) \
    if (android_testLog(ANDROID_##priority, tag))
#endif

#ifndef SEC_IF_ALOG
#define SEC_IF_ALOG(priority, tag) \
    if (android_testLog(ANDROID_##priority, tag))
#endif


// ---------------------------------------------------------------------

/*
 * Event logging.
 */

/*
 * Event log entry types.  These must match up with the declarations in
 * java/android/android/util/EventLog.java.
 */
typedef enum {
    EVENT_TYPE_INT      = 0,
    EVENT_TYPE_LONG     = 1,
    EVENT_TYPE_STRING   = 2,
    EVENT_TYPE_LIST     = 3,
} AndroidEventLogType;


#ifndef LOG_EVENT_INT
#define LOG_EVENT_INT(_tag, _value) {                                       \
        int intBuf = _value;                                                \
        (void) android_btWriteLog(_tag, EVENT_TYPE_INT, &intBuf,            \
            sizeof(intBuf));                                                \
    }
#endif
#ifndef LOG_EVENT_LONG
#define LOG_EVENT_LONG(_tag, _value) {                                      \
        long long longBuf = _value;                                         \
        (void) android_btWriteLog(_tag, EVENT_TYPE_LONG, &longBuf,          \
            sizeof(longBuf));                                               \
    }
#endif
#ifndef LOG_EVENT_STRING
#define LOG_EVENT_STRING(_tag, _value)                                      \
    ((void) 0)  /* not implemented -- must combine len with string */
#endif
/* TODO: something for LIST */

/*
 * ===========================================================================
 *
 * The stuff in the rest of this file should not be used directly.
 */

#if USE_SWITCHABLE_LOG
#define android_printLog(prio, tag, fmt...) \
    __android_log_switchable_print(prio, tag, fmt)
#else
#define android_printLog(prio, tag, fmt...) \
    __android_log_print(prio, tag, fmt)
#endif    

#define android_vprintLog(prio, cond, tag, fmt...) \
    __android_log_vprint(prio, tag, fmt)

/* XXX Macros to work around syntax errors in places where format string
 * arg is not passed to ALOG_ASSERT, LOG_ALWAYS_FATAL or LOG_ALWAYS_FATAL_IF
 * (happens only in debug builds).
 */

/* Returns 2nd arg.  Used to substitute default value if caller's vararg list
 * is empty.
 */
#define __android_second(dummy, second, ...)     second

/* If passed multiple args, returns ',' followed by all but 1st arg, otherwise
 * returns nothing.
 */
#define __android_rest(first, ...)               , ## __VA_ARGS__

#define android_printAssert(cond, tag, fmt...) \
    __android_log_assert(cond, tag, \
        __android_second(0, ## fmt, NULL) __android_rest(fmt))

#define android_writeLog(prio, tag, text) \
    __android_log_write(prio, tag, text)

#define android_bWriteLog(tag, payload, len) \
    __android_log_bwrite(tag, payload, len)
#define android_btWriteLog(tag, type, payload, len) \
    __android_log_btwrite(tag, type, payload, len)

// TODO: remove these prototypes and their users
#define android_testLog(prio, tag) (1)
#define android_writevLog(vec,num) do{}while(0)
#define android_write1Log(str,len) do{}while (0)
#define android_setMinPriority(tag, prio) do{}while(0)
//#define android_logToCallback(func) do{}while(0)
#define android_logToFile(tag, file) (0)
#define android_logToFd(tag, fd) (0)

typedef enum {
    LOG_ID_MAIN = 0,
    LOG_ID_RADIO = 1,
    LOG_ID_EVENTS = 2,
    LOG_ID_SYSTEM = 3,
    LOG_ID_SF = 4,

    LOG_ID_MAX
} log_id_t;

/*
 * Send a simple string to the log.
 */
int __android_log_buf_write(int bufID, int prio, const char *tag, const char *text);
int __android_log_buf_print(int bufID, int prio, const char *tag, const char *fmt, ...);
int __android_log_switchable_print(int prio, const char *tag, const char *fmt, ...);
int __android_log_switchable_buf_print(int bufID, int prio, const char *tag, const char *fmt, ...);


#ifdef __cplusplus
}
#endif

#endif // _LIBS_CUTILS_LOG_H
