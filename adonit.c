/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2011  Nokia Corporation
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
#include "config.h"
#endif

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>
#include <glib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>

#include "lib/uuid.h"
#include <btio/btio.h>
#include "att.h"
#include "gattrib.h"
#include "gatt.h"
#include "gatttool.h"

#include "log.h"
#include "uinput.h"

static GIOChannel *iochannel = NULL;
static GAttrib *attrib = NULL;
static GMainLoop *event_loop;

static gchar *opt_src = NULL;
static gchar *opt_dst = NULL;
static gchar *opt_dst_type = NULL;
static gchar *opt_sec_level = NULL;
static const int opt_psm = 0;
static int opt_mtu = 0;
static int start;
static int end;

static enum state {
    STATE_DISCONNECTED=0,
    STATE_CONNECTING=1,
    STATE_CONNECTED=2
} conn_state;

struct characteristic_data {
    uint16_t orig_start;
    uint16_t start;
    uint16_t end;
    bt_uuid_t uuid;
};

static const char
*tag_RESPONSE  = "rsp",
    *tag_ERRCODE   = "code",
    *tag_HANDLE    = "hnd",
    *tag_UUID      = "uuid",
    *tag_DATA      = "d",
    *tag_CONNSTATE = "state",
    *tag_SEC_LEVEL = "sec",
    *tag_MTU       = "mtu",
    *tag_DEVICE    = "dst",
    *tag_RANGE_START = "hstart",
    *tag_RANGE_END = "hend",
    *tag_PROPERTIES= "props",
    *tag_VALUE_HANDLE = "vhnd";

static const char
*rsp_ERROR     = "err",
    *rsp_STATUS    = "stat",
    *rsp_NOTIFY    = "ntfy",
    *rsp_IND       = "ind",
    *rsp_DISCOVERY = "find",
    *rsp_DESCRIPTORS = "desc",
    *rsp_READ      = "rd",
    *rsp_WRITE     = "wr";

static const char
*err_CONN_FAIL = "connfail",
    *err_COMM_ERR  = "comerr",
    *err_PROTO_ERR = "protoerr",
    *err_NOT_FOUND = "notfound",
    *err_BAD_CMD   = "badcmd",
    *err_BAD_PARAM = "badparam",
    *err_BAD_STATE = "badstate";

static const char
*st_DISCONNECTED = "disc",
    *st_CONNECTING   = "tryconn",
    *st_CONNECTED    = "conn";

struct uinput_info uinfo;
struct uinput_user_dev dev;

int fd;
struct input_event event;

int prev_btn_0;
int prev_btn_1;

void pen_update(uint16_t p)
{
    struct input_event ev;

    gettimeofday(&ev.time, 0);

    int btn_0 = (p & 0x1);
    int btn_1 = (p & 0x2) >> 1;

    if (btn_0 != prev_btn_0) {
        ev.type = EV_KEY;
        ev.code = BTN_0;
        ev.value = btn_0;

        uinput_write_event(&uinfo, &ev);

        printf("Button 0\n");
    }
    prev_btn_0 = btn_0;

    if (btn_1 != prev_btn_1) {
        ev.type = EV_KEY;
        ev.code = BTN_1;
        ev.value = btn_1;

        uinput_write_event(&uinfo, &ev);

        printf("Button 1\n");
    }
    prev_btn_1 = btn_1;

    ev.type = EV_ABS;
    ev.code = ABS_PRESSURE;
    ev.value = (p >> 5) & 0x7ff;
    uinput_write_event(&uinfo, &ev);

    printf("Pressure: %d\n", ev.value);

    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    ev.value = 0;

    uinput_write_event(&uinfo, &ev);

}

gboolean touchscreen_update(GIOChannel *chan, GIOCondition cond, gpointer user_data) {
    struct input_event *ev;
    gsize size;
    gchar buffer[24];
    GError *error = NULL;

    if (error) {
        printf("%s", error->message);
    }

    while(G_IO_STATUS_NORMAL == g_io_channel_read_chars(chan, buffer, sizeof(struct input_event), &size, &error)) {
        ev = (struct input_event *)buffer;

        if (ev->type == EV_SYN) {
            printf("PACK %i %i\n", ev->code, ev->value);
        } else if (ev->type == EV_KEY) {
            //if (ev->code == BTN_TOUCH) {
            //    ev->code = BTN_TOOL_PEN;
            //}
            printf("EV_KEY code=%i value=%i\n", ev->code, ev->value);

            uinput_write_event(&uinfo, ev);
        } else if (ev->type == EV_ABS) {
            printf("EV_ABS code=%i value=%i\n", ev->code, ev->value);

            if (ev->code == ABS_X || ev->code == ABS_Y) {
                uinput_write_event(&uinfo, ev);
            }
        }
    }

    ev->type = EV_SYN;
    ev->code = SYN_REPORT;
    ev->value = 0;

    uinput_write_event(&uinfo, ev);

    return TRUE;
}

static void resp_begin(const char *rsptype)
{
    printf("%s=$%s", tag_RESPONSE, rsptype);
}

static void send_sym(const char *tag, const char *val)
{
    printf(" %s=$%s", tag, val);
}

static void send_uint(const char *tag, unsigned int val)
{
    printf(" %s=h%X", tag, val);
}

static void send_str(const char *tag, const char *val)
{
    //!!FIXME
    printf(" %s='%s", tag, val);
}

static void send_data(const unsigned char *val, size_t len)
{
    printf(" %s=b", tag_DATA);
    while ( len-- > 0 )
        printf("%02X", *val++);
}

static void resp_end()
{
    printf("\n");
    fflush(stdout);
}

static void resp_error(const char *errcode)
{
    resp_begin(rsp_ERROR);
    send_sym(tag_ERRCODE, errcode);
    resp_end();
}

static void set_state(enum state st)
{
    conn_state = st;
}

static void events_handler(const uint8_t *pdu, uint16_t len, gpointer user_data)
{
    uint8_t *opdu;
    uint8_t evt;
    uint16_t handle, i, olen;
    size_t plen;

    evt = pdu[0];

    if ( evt != ATT_OP_HANDLE_NOTIFY && evt != ATT_OP_HANDLE_IND )
    {
        printf("#Invalid opcode %02X in event handler??\n", evt);
        return;
    }

    assert( len >= 3 );
    handle = att_get_u16(&pdu[1]);

    uint16_t value = pdu[3] << 8 | pdu[4];

    pen_update(value);

    if (evt == ATT_OP_HANDLE_NOTIFY)
        return;

    opdu = g_attrib_get_buffer(attrib, &plen);
    olen = enc_confirmation(opdu, plen);

    if (olen > 0)
        g_attrib_send(attrib, 0, opdu, olen, NULL, NULL, NULL);
}

static void connect_cb(GIOChannel *io, GError *err, gpointer user_data)
{
    printf("Connected.");
    if (err) {
        set_state(STATE_DISCONNECTED);
        resp_error(err_CONN_FAIL);
        printf("# Connect error: %s\n", err->message);
        return;
    }


    attrib = g_attrib_new(iochannel);
    g_attrib_register(attrib, ATT_OP_HANDLE_NOTIFY, GATTRIB_ALL_HANDLES,
            events_handler, attrib, NULL);
    g_attrib_register(attrib, ATT_OP_HANDLE_IND, GATTRIB_ALL_HANDLES,
            events_handler, attrib, NULL);
    set_state(STATE_CONNECTED);

    uint8_t value = 0x1;
    gatt_write_char(attrib, 0x000c, &value, 1, NULL, NULL);
}

static void disconnect_io()
{
    if (conn_state == STATE_DISCONNECTED)
        return;

    uint8_t value = 0x0;
    gatt_write_char(attrib, 0x000c, &value, 1, NULL, NULL);

    g_attrib_unref(attrib);
    attrib = NULL;
    opt_mtu = 0;

    g_io_channel_shutdown(iochannel, FALSE, NULL);
    g_io_channel_unref(iochannel);
    iochannel = NULL;

    set_state(STATE_DISCONNECTED);
}

static void primary_all_cb(GSList *services, guint8 status, gpointer user_data)
{
    GSList *l;

    if (status) {
        resp_error(err_COMM_ERR); // Todo: status
        return;
    }

    resp_begin(rsp_DISCOVERY);
    for (l = services; l; l = l->next) {
        struct gatt_primary *prim = l->data;
        send_uint(tag_RANGE_START, prim->range.start);
        send_uint(tag_RANGE_END, prim->range.end);
        send_str(tag_UUID, prim->uuid);
    }
    resp_end();

}

static void primary_by_uuid_cb(GSList *ranges, guint8 status,
        gpointer user_data)
{
    GSList *l;

    if (status) {
        resp_error(err_COMM_ERR); // Todo: status
        return;
    }

    resp_begin(rsp_DISCOVERY);
    for (l = ranges; l; l = l->next) {
        struct att_range *range = l->data;
        send_uint(tag_RANGE_START, range->start);
        send_uint(tag_RANGE_END, range->end);
    }
    resp_end();
}

static void included_cb(GSList *includes, guint8 status, gpointer user_data)
{
    GSList *l;

    if (status) {
        resp_error(err_COMM_ERR); // Todo: status
        return;
    }

    resp_begin(rsp_DISCOVERY);
    for (l = includes; l; l = l->next) {
        struct gatt_included *incl = l->data;
        send_uint(tag_HANDLE, incl->handle);
        send_uint(tag_RANGE_START, incl->range.start);
        send_uint(tag_RANGE_END,   incl->range.end);
        send_str(tag_UUID, incl->uuid);
    }
    resp_end();
}

static void char_cb(GSList *characteristics, guint8 status, gpointer user_data)
{
    GSList *l;

    if (status) {
        resp_error(err_COMM_ERR); // Todo: status
        return;
    }

    resp_begin(rsp_DISCOVERY);
    for (l = characteristics; l; l = l->next) {
        struct gatt_char *chars = l->data;
        send_uint(tag_HANDLE, chars->handle);
        send_uint(tag_PROPERTIES, chars->properties);
        send_uint(tag_VALUE_HANDLE, chars->value_handle);
        send_str(tag_UUID, chars->uuid);
    }
    resp_end();
}

static void char_desc_cb(guint8 status, const guint8 *pdu, guint16 plen,
        gpointer user_data)
{
    struct att_data_list *list;
    guint8 format;
    uint16_t handle = 0xffff;
    int i;

    if (status != 0) {
        resp_error(err_COMM_ERR); // Todo: status
        return;
    }

    list = dec_find_info_resp(pdu, plen, &format);
    if (list == NULL) {
        resp_error(err_NOT_FOUND); // Todo: what does this mean?
        return;
    }

    resp_begin(rsp_DESCRIPTORS);
    for (i = 0; i < list->num; i++) {
        char uuidstr[MAX_LEN_UUID_STR];
        uint8_t *value;
        bt_uuid_t uuid;

        value = list->data[i];
        handle = att_get_u16(value);

        if (format == 0x01)
            uuid = att_get_uuid16(&value[2]);
        else
            uuid = att_get_uuid128(&value[2]);

        bt_uuid_to_string(&uuid, uuidstr, MAX_LEN_UUID_STR);
        send_uint(tag_HANDLE, handle);
        send_str (tag_UUID, uuidstr);
    }
    resp_end();

    att_data_list_free(list);

    if (handle != 0xffff && handle < end)
        gatt_find_info(attrib, handle + 1, end, char_desc_cb, NULL);
}

static void char_read_cb(guint8 status, const guint8 *pdu, guint16 plen,
        gpointer user_data)
{
    uint8_t value[plen];
    ssize_t vlen;

    if (status != 0) {
        resp_error(err_COMM_ERR); // Todo: status
        return;
    }

    vlen = dec_read_resp(pdu, plen, value, sizeof(value));
    if (vlen < 0) {
        resp_error(err_COMM_ERR);
        return;
    }

    resp_begin(rsp_READ);
    send_data(value, vlen);
    resp_end();
}

static void char_read_by_uuid_cb(guint8 status, const guint8 *pdu,
        guint16 plen, gpointer user_data)
{
    struct characteristic_data *char_data = user_data;
    struct att_data_list *list;
    int i;

    if (status == ATT_ECODE_ATTR_NOT_FOUND &&
            char_data->start != char_data->orig_start)
    {
        printf("# TODO case in char_read_by_uuid_cb\n");
        goto done;
    }

    if (status != 0) {
        resp_error(err_COMM_ERR); // Todo: status
        goto done;
    }

    list = dec_read_by_type_resp(pdu, plen);

    resp_begin(rsp_READ);
    if (list == NULL)
        goto nolist;

    for (i = 0; i < list->num; i++) {
        uint8_t *value = list->data[i];
        int j;

        char_data->start = att_get_u16(value) + 1;

        send_uint(tag_HANDLE, att_get_u16(value));
        send_data(value+2, list->len-2); // All the same length??
    }

    att_data_list_free(list);
nolist:
    resp_end();

done:
    g_free(char_data);
}

static gboolean channel_watcher(GIOChannel *chan, GIOCondition cond,
        gpointer user_data)
{
    disconnect_io();

    return FALSE;
}

static void cmd_connect(int argcp, char **argvp)
{
    if (conn_state != STATE_DISCONNECTED)
        return;

    if (argcp > 1) {
        g_free(opt_dst);
        opt_dst = g_strdup(argvp[1]);

        g_free(opt_dst_type);
        if (argcp > 2)
            opt_dst_type = g_strdup(argvp[2]);
        else
            opt_dst_type = g_strdup("public");
    }

    if (opt_dst == NULL) {
        resp_error(err_BAD_PARAM);
        return;
    }

    set_state(STATE_CONNECTING);
    iochannel = gatt_connect(opt_src, opt_dst, opt_dst_type, opt_sec_level,
            opt_psm, opt_mtu, connect_cb);

    if (iochannel == NULL)
        set_state(STATE_DISCONNECTED);
    else
        g_io_add_watch(iochannel, G_IO_HUP, channel_watcher, NULL);
}

static void cmd_primary(int argcp, char **argvp)
{
    bt_uuid_t uuid;

    if (conn_state != STATE_CONNECTED) {
        resp_error(err_BAD_STATE);
        return;
    }

    if (argcp == 1) {
        gatt_discover_primary(attrib, NULL, primary_all_cb, NULL);
        return;
    }

    if (bt_string_to_uuid(&uuid, argvp[1]) < 0) {
        resp_error(err_BAD_PARAM);
        return;
    }

    gatt_discover_primary(attrib, &uuid, primary_by_uuid_cb, NULL);
}

static int strtohandle(const char *src)
{
    char *e;
    int dst;

    errno = 0;
    dst = strtoll(src, &e, 16);
    if (errno != 0 || *e != '\0')
        return -EINVAL;

    return dst;
}

static void cmd_included(int argcp, char **argvp)
{
    int start = 0x0001;
    int end = 0xffff;

    if (conn_state != STATE_CONNECTED) {
        resp_error(err_BAD_STATE);
        return;
    }

    if (argcp > 1) {
        start = strtohandle(argvp[1]);
        if (start < 0) {
            resp_error(err_BAD_PARAM);
            return;
        }
        end = start;
    }

    if (argcp > 2) {
        end = strtohandle(argvp[2]);
        if (end < 0) {
            resp_error(err_BAD_PARAM);
            return;
        }
    }

    gatt_find_included(attrib, start, end, included_cb, NULL);
}

static void cmd_char(int argcp, char **argvp)
{
    int start = 0x0001;
    int end = 0xffff;

    if (conn_state != STATE_CONNECTED) {
        resp_error(err_BAD_STATE);
        return;
    }

    if (argcp > 1) {
        start = strtohandle(argvp[1]);
        if (start < 0) {
            resp_error(err_BAD_PARAM);
            return;
        }
    }

    if (argcp > 2) {
        end = strtohandle(argvp[2]);
        if (end < 0) {
            resp_error(err_BAD_PARAM);
            return;
        }
    }

    if (argcp > 3) {
        bt_uuid_t uuid;

        if (bt_string_to_uuid(&uuid, argvp[3]) < 0) {
            resp_error(err_BAD_PARAM);
            return;
        }

        gatt_discover_char(attrib, start, end, &uuid, char_cb, NULL);
        return;
    }

    gatt_discover_char(attrib, start, end, NULL, char_cb, NULL);
}

static void cmd_char_desc(int argcp, char **argvp)
{
    if (conn_state != STATE_CONNECTED) {
        resp_error(err_BAD_STATE);
        return;
    }

    if (argcp > 1) {
        start = strtohandle(argvp[1]);
        if (start < 0) {
            resp_error(err_BAD_PARAM);
            return;
        }
    } else
        start = 0x0001;

    if (argcp > 2) {
        end = strtohandle(argvp[2]);
        if (end < 0) {
            resp_error(err_BAD_PARAM);
            return;
        }
    } else
        end = 0xffff;

    gatt_find_info(attrib, start, end, char_desc_cb, NULL);
}

static void cmd_read_hnd(int argcp, char **argvp)
{
    int handle;

    if (conn_state != STATE_CONNECTED) {
        resp_error(err_BAD_STATE);
        return;
    }

    if (argcp < 2) {
        resp_error(err_BAD_PARAM);
        return;
    }

    handle = strtohandle(argvp[1]);
    if (handle < 0) {
        resp_error(err_BAD_PARAM);
        return;
    }

    gatt_read_char(attrib, handle, char_read_cb, attrib);
}

static void cmd_read_uuid(int argcp, char **argvp)
{
    struct characteristic_data *char_data;
    int start = 0x0001;
    int end = 0xffff;
    bt_uuid_t uuid;

    if (conn_state != STATE_CONNECTED) {
        resp_error(err_BAD_STATE);
        return;
    }

    if (argcp < 2 ||
            bt_string_to_uuid(&uuid, argvp[1]) < 0) {
        resp_error(err_BAD_PARAM);
        return;
    }

    if (argcp > 2) {
        start = strtohandle(argvp[2]);
        if (start < 0) {
            resp_error(err_BAD_PARAM);
            return;
        }
    }

    if (argcp > 3) {
        end = strtohandle(argvp[3]);
        if (end < 0) {
            resp_error(err_BAD_PARAM);
            return;
        }
    }

    char_data = g_new(struct characteristic_data, 1);
    char_data->orig_start = start;
    char_data->start = start;
    char_data->end = end;
    char_data->uuid = uuid;

    gatt_read_char_by_uuid(attrib, start, end, &char_data->uuid,
            char_read_by_uuid_cb, char_data);
}

static void char_write_req_cb(guint8 status, const guint8 *pdu, guint16 plen,
        gpointer user_data)
{
    if (status != 0) {
        resp_error(err_COMM_ERR); // Todo: status
        return;
    }

    if (!dec_write_resp(pdu, plen) && !dec_exec_write_resp(pdu, plen)) {
        resp_error(err_PROTO_ERR);
        return;
    }

    resp_begin(rsp_WRITE);
    resp_end();
}

static void cmd_char_write_common(int argcp, char **argvp, int with_response)
{
    uint8_t *value;
    size_t plen;
    int handle;

    if (conn_state != STATE_CONNECTED) {
        resp_error(err_BAD_STATE);
        return;
    }

    if (argcp < 3) {
        resp_error(err_BAD_PARAM);
        return;
    }

    handle = strtohandle(argvp[1]);
    if (handle <= 0) {
        resp_error(err_BAD_PARAM);
        return;
    }

    plen = gatt_attr_data_from_string(argvp[2], &value);
    if (plen == 0) {
        resp_error(err_BAD_PARAM);
        return;
    }

    if (with_response)
        gatt_write_char(attrib, handle, value, plen,
                char_write_req_cb, NULL);
    else
    {
        gatt_write_char(attrib, handle, value, plen, NULL, NULL);
        resp_begin(rsp_WRITE);
        resp_end();
    }

    g_free(value);
}

static void cmd_char_write(int argcp, char **argvp)
{
    cmd_char_write_common(argcp, argvp, 0);
}

static void cmd_char_write_rsp(int argcp, char **argvp)
{
    cmd_char_write_common(argcp, argvp, 1);
}

static void exchange_mtu_cb(guint8 status, const guint8 *pdu, guint16 plen,
        gpointer user_data)
{
    uint16_t mtu;

    if (status != 0) {
        resp_error(err_COMM_ERR); // Todo: status
        return;
    }

    if (!dec_mtu_resp(pdu, plen, &mtu)) {
        resp_error(err_PROTO_ERR);
        return;
    }

    mtu = MIN(mtu, opt_mtu);
    /* Set new value for MTU in client */
    if (g_attrib_set_mtu(attrib, mtu))
    {
        opt_mtu = mtu;
    }
    else
    {
        printf("# Error exchanging MTU\n");
        resp_error(err_COMM_ERR);
    }
}

int init_touchscreen() {
    int fd = open("/dev/input/event9", O_RDONLY | O_NONBLOCK);

    int grab = 1;
    ioctl(fd, EVIOCGRAB, &grab);

    return fd;
}

int main(int argc, char *argv[])
{
	int c;

	while (1) {
		int this_option_optind = optind ? optind : 1;
		int option_index = 0;

		static struct option long_options[] = {
			{"verbose",	no_argument,       0,  	'v'	},
			{0,         0,                 0,  	0 	}
		};

		c = getopt_long(argc, argv, "v",
				 long_options, &option_index);

		if (c == -1) {
			break;
		}

		switch (c) {
		case 'v':
			verbose = 1;
			break;

		case '?':
			break;
		}
	}

    GIOChannel *touchscreen;
    gint events;

    opt_sec_level = g_strdup("low");

    opt_src = NULL;
    opt_dst = g_strdup("00:17:53:57:81:86");
    opt_dst_type = g_strdup("public");

    LOG("# " __FILE__ " built at " __TIME__ " on " __DATE__ "\n");

    event_loop = g_main_loop_new(NULL, FALSE);

    uinfo.create_mode = WDAEMON_CREATE;

    printf("Opening uinput device...");
    if (uinput_create(&uinfo)) {
        printf("UInput device created successfully.");
    }

    touchscreen = g_io_channel_unix_new(init_touchscreen());

    g_io_channel_set_encoding(touchscreen, NULL, NULL);

    g_io_add_watch(touchscreen, G_IO_IN | G_IO_HUP | G_IO_ERR, touchscreen_update, NULL);

    iochannel = gatt_connect(NULL, opt_dst, opt_dst_type, opt_sec_level,
            0, 0, connect_cb);

    if (iochannel == NULL) {
        printf("NULL iochannel");
        return 1;
    } else {
        g_io_add_watch(iochannel, G_IO_HUP, channel_watcher, NULL);
    }

    printf("Starting loop.");
    g_main_loop_run(event_loop);

    disconnect_io();
    fflush(stdout);
    g_main_loop_unref(event_loop);

    g_io_channel_shutdown(touchscreen, FALSE, NULL);
    g_io_channel_unref(touchscreen);
    touchscreen = NULL;

    g_free(opt_src);
    g_free(opt_dst);
    g_free(opt_sec_level);

    ioctl(fd, EVIOCGRAB, NULL);
    close(fd);

    return EXIT_SUCCESS;
}

