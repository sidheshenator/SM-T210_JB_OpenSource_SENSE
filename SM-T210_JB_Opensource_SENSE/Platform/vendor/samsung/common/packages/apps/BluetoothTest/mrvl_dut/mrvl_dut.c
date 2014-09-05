/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2000-2001  Qualcomm Incorporated
 *  Copyright (C) 2002-2003  Maxim Krasnyansky <maxk@qualcomm.com>
 *  Copyright (C) 2002-2009  Marcel Holtmann <marcel@holtmann.org>
 *
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>  
#include <sys/un.h>
#include <utils/Log.h>


#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

//+ KJH 20100810 :: for android_set_aid_and_cap & log
/* init.rc
service bt_dut_cmd /system/bin/bcm_dut
    group bluetooth net_bt_admin
    disabled
    oneshot
*/

#include <private/android_filesystem_config.h>
#include <sys/prctl.h>
#include <linux/capability.h>
#include "utils/Log.h"


#define SOCKERR_IO          -1
#define SOCKERR_CLOSED      -2


static int wifi_enable(void);
static int wifi_disable(void);
static int wireless_send_command(const char *cmd);
static int cli_conn (const char *name);


static const char* WIFI_PS_MODE_PATH = "/data/.psm.info";
static const char *local_socket_dir = "/data/misc/wifi/sockets/socket_daemon";

//P1TF_BEGIN GA_BT(+neo. 2010.06.13.)
//to support DUT mode

//+ KJH 20100810 :: from android_bluez.c
/* Set UID to bluetooth w/ CAP_NET_RAW, CAP_NET_ADMIN and CAP_NET_BIND_SERVICE
 * (Android's init.rc does not yet support applying linux capabilities) */
void android_set_aid_and_cap() {
    ALOGI("[GABT] %s + ",__FUNCTION__);

    int ret = -1;
    prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0);

    gid_t groups[] = {AID_BLUETOOTH, AID_WIFI, AID_NET_BT_ADMIN, AID_NET_BT, AID_INET, AID_NET_RAW, AID_NET_ADMIN};
    if ((ret = setgroups(sizeof(groups)/sizeof(groups[0]), groups)) != 0){
        ALOGE("setgroups failed, ret:%d, strerror:%s", ret, strerror(errno));
        return;
    }

    if(setgid(AID_SYSTEM) != 0){
        ALOGE("setgid failed, ret:%d, strerror:%s", ret, strerror(errno));
        return;
    }

    if ((ret = setuid(AID_SYSTEM)) != 0){
        ALOGE("setuid failed, ret:%d, strerror:%s", ret, strerror(errno));
        return;
    }

    struct __user_cap_header_struct header;
    struct __user_cap_data_struct cap;
    header.version = _LINUX_CAPABILITY_VERSION;
    header.pid = 0;

    cap.effective = cap.permitted = 1 << CAP_NET_RAW |
    1 << CAP_NET_ADMIN |
    1 << CAP_NET_BIND_SERVICE |
    1 << CAP_SYS_MODULE |
    1 << CAP_IPC_LOCK;

    cap.inheritable = 0;
    if ((ret = capset(&header, &cap)) != 0){
        ALOGE("capset failed, ret:%d, strerror:%s", ret, strerror(errno));
        return;
    }

    ALOGI("[GABT] %s - ",__FUNCTION__);
    return;
}


void write_ps_mode_for_dut(char arg)
{

    char data = 0;
    int fd = 0;
    int sz = 0;
    int ch = 0;
    int ret = 0;

    ALOGI("[sLan] %s(%d) ++++ arg: %c", __FUNCTION__, __LINE__, arg);
    fd = open(WIFI_PS_MODE_PATH, O_RDWR | O_CREAT);
    if (fd < 0)
    {
      ALOGE("Creat %s, Fail, %s", WIFI_PS_MODE_PATH, strerror(errno));
      goto out;
    }
    ch = chmod(WIFI_PS_MODE_PATH, 0666);
    if (ch < 0)
    {
      ALOGE("Chmod %s, Fail, %s", WIFI_PS_MODE_PATH, strerror(errno));
      goto out;
    }
    data = arg;
    sz = write(fd, &data, 1);
    if (sz < 0)
    {
      ALOGE("Write %s, Fail, %s", WIFI_PS_MODE_PATH, strerror(errno));
      goto out;
    }

out:
    if( fd > 0)
       close(fd);
       ALOGI("[sLan] %s(%d) ----", __FUNCTION__, __LINE__);
    return ;
}


int main(int argc, char *argv[])
{
    int dev_id = -1;

    ALOGI("[GABT] %s :: before : getuid=%d",__FUNCTION__, getuid());
    android_set_aid_and_cap();
    ALOGI("[GABT] %s :: after : getuid=%d",__FUNCTION__, getuid());

    unsigned char buf[HCI_MAX_EVENT_SIZE];
    struct hci_filter flt;
    int len = 0;
    int dd = -1;
    uint16_t ocf;
    uint8_t ogf;

	ALOGI("[sLan] %s(%d) START: wlan for sLan", __FUNCTION__, __LINE__);
	write_ps_mode_for_dut('0');
	sleep(1);
	wifi_enable();
	sleep(1);
	system("/system/xbin/iwlist wlan0 scan");
        write_ps_mode_for_dut('1');
	ALOGI("[sLan] %s(%d) END: wlan for sLan", __FUNCTION__, __LINE__);


    if (dev_id < 0)
        dev_id = hci_get_route(NULL);

    errno = 0;

    dd = hci_open_dev(dev_id);
    if (dd < 0) {
        ALOGE("[GABT] %s :: Hci Device open failed :: dev_id=%d, dd=%d",__FUNCTION__, dev_id, dd);
        perror("Hci Device open failed");
        exit(EXIT_FAILURE);
    }


    /* Setup filter */
    hci_filter_clear(&flt);
    hci_filter_set_ptype(HCI_EVENT_PKT, &flt);
    hci_filter_all_events(&flt);
    if (setsockopt(dd, SOL_HCI, HCI_FILTER, &flt, sizeof(flt)) < 0) {
        ALOGE("[GABT] %s :: HCI filter setup failed ",__FUNCTION__);
        perror("HCI filter setup failed");
        exit(EXIT_FAILURE);
    }


    /* sleep mode disable */
    ogf = 0x03;
    ocf = 0x0001;

    memset(buf, 0, sizeof(buf));
    buf[0] = 0x3f;
    buf[1] = 0x27;
    buf[2] = 0x00;
    buf[3] = 0x00;
    buf[4] = 0x00;
    buf[5] = 0x00;
    buf[6] = 0x00;
    buf[7] = 0x00;
    buf[8] = 0x00;
    buf[9] = 0x00;
    buf[10] = 0x00;
    buf[11] = 0x00;
    buf[12] = 0x00;
    buf[13] = 0x00;

    len = 14;

    if (hci_send_cmd(dd, ogf, ocf, len, buf) < 0) {
        ALOGE("[GABT] %s :: Send failed 1",__FUNCTION__);
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    len = read(dd, buf, sizeof(buf));
    if (len < 0) {
        ALOGE("[GABT] %s :: Read failed 1",__FUNCTION__);
        perror("Read failed");
        exit(EXIT_FAILURE);
    }
    ALOGI("[GABT] sleep mode disable -");


    /* Set Event Filter */
    ogf = 0x03;
    ocf = 0x0005;

    memset(buf, 0, sizeof(buf));
    buf[0] = 0x02; //Filter_Type          , 0x02 : Connection Setup.
    buf[1] = 0x00; //Filter_Condition_Type, 0x00 : Allow Connections from all devices.
    buf[2] = 0x02; //Condition
    len = 3;

    if (hci_send_cmd(dd, ogf, ocf, len, buf) < 0) {
        ALOGE("[GABT] %s :: Send failed 2 ",__FUNCTION__);
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    len = read(dd, buf, sizeof(buf));
    if (len < 0) {
        ALOGE("[GABT] %s :: Read failed 2 ",__FUNCTION__);
        perror("Read failed");
        exit(EXIT_FAILURE);
    }
    ALOGI("[GABT] Set Event Filter -");


    /* Write Scan Enable */
    ogf = 0x03;
    ocf = 0x001a;
    memset(buf, 0, sizeof(buf));
    buf[0] = 0x03; //Scan_Enable, 0x03 : Inquiry Scan enabled.
                   //                    Page Scan enabled.
    len = 1;

    if (hci_send_cmd(dd, ogf, ocf, len, buf) < 0) {
        ALOGE("[GABT] %s :: Send failed 3 ",__FUNCTION__);
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    len = read(dd, buf, sizeof(buf));
    if (len < 0) {
        ALOGE("[GABT] %s :: Read failed 3 ",__FUNCTION__);
        perror("Read failed");
        exit(EXIT_FAILURE);
    }
    ALOGI("[GABT] Write Scan Enable -");

    /* Enable Device Under Test Mode */
    ogf = 0x06;
    ocf = 0x0003;
    memset(buf, 0, sizeof(buf));
    //buf[0] = 0x00;
    len = 0;//1;

    if (hci_send_cmd(dd, ogf, ocf, len, buf) < 0) {
        ALOGE("[GABT] %s :: Send failed 4 ",__FUNCTION__);
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    len = read(dd, buf, sizeof(buf));
    if (len < 0) {
        ALOGE("[GABT] %s :: Read failed 4 ",__FUNCTION__);
        perror("Read failed");
        exit(EXIT_FAILURE);
    }
    ALOGE("[GABT] Enable Device Under Test Mode -");

    hci_close_dev(dd);

    ALOGE("[GABT] %s :: EXIT ",__FUNCTION__);
    return 0;
}


static int wifi_enable(void)
{
    return wireless_send_command("WIFI_ENABLE");
}

static int wifi_disable(void)
{
    return wireless_send_command("WIFI_DISABLE");
}

/*send wireless command:wifi,uAP,Bluetooth,FM enable/disable commands to marvell wireless daemon*/
static int wireless_send_command(const char *cmd)
{
    int conn_fd = -1;
    int n = 0;
    int i = 0;
    int data = 0;
    char buffer[256];
    int len = 0;
    int ret = 1;

    conn_fd = cli_conn (local_socket_dir);
    if (conn_fd < 0) {
        ALOGE("cli_conn error.\n");
        ret = 1;
        goto out1;
    }
    len = strlen(cmd);
    strncpy(buffer, cmd, len);
    buffer[len++] = '\0';
    n = write(conn_fd, buffer, len);

    if (n == SOCKERR_IO) 
    {
        ALOGE("write error on fd %d\n", conn_fd);
        ret = 1;
        goto out;
    }
    else if (n == SOCKERR_CLOSED) 
    {
        ALOGE("fd %d has been closed.\n", conn_fd);
        ret = 1;
        goto out;
    }
    else 
        ALOGI("Wrote %s to server. \n", buffer);

    n = read(conn_fd, buffer, sizeof (buffer));
    if (n == SOCKERR_IO) 
    {
        ALOGE("read error on fd %d\n", conn_fd);
        ret = 1;
        goto out;
    }
    else if (n == SOCKERR_CLOSED) 
    {
        ALOGE("fd %d has been closed.\n", conn_fd);
        ret = 1;
        goto out;
    }
    else 
    {
        if(!strncmp(buffer,"0,OK", strlen("0,OK")))
            ret = 0;
        else
            ret = 1;
    }
out:
    close(conn_fd);
out1:
    return ret; 
}

/* returns fd if all OK, -1 on error */
static int cli_conn (const char *name)
{
    int                fd, len;
    int ret = 0;
    struct sockaddr_un unix_addr;
    int value = 0x1;
    /* create a Unix domain stream socket */
    if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
    {
        ALOGE("create socket failed, ret:%d, strerror:%s", fd, strerror(errno));
        return(-1);
    }

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void*)&value, sizeof(value));
    /* fill socket address structure w/server's addr */
    memset(&unix_addr, 0, sizeof(unix_addr));
    unix_addr.sun_family = AF_UNIX;
    strcpy(unix_addr.sun_path, name);
    len = sizeof(unix_addr.sun_family) + strlen(unix_addr.sun_path);

    if ((ret = connect (fd, (struct sockaddr *) &unix_addr, len)) < 0)
    {
        ALOGE("connect failed, ret:%d, strerror:%s", ret, strerror(errno));
        goto error;
    }
    return (fd);

error:
    close (fd);
    return ret;
}