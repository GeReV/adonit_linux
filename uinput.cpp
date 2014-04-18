/*
 *    This file is part of wdaemon.
 *
 *    wdaemon is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    wdaemon is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with wdaemon; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include "log.h"
#include "uinput.h"

#ifndef UINPUT_DEVICE
//#error Please define UINPUT_DEVICE in Makefile
#define UINPUT_DEVICE "/dev/uinput"
#endif

#define USB_VENDOR_ID_WACOM 0x056a
#ifndef BUS_VIRTUAL
#define BUS_VIRTUAL 6
#endif

#define set_event(x, y, z) do { if (ioctl((x)->fd, (y), (z)) == -1) { \
					log(LOG_ERR, "Error enabling %s (%s)\n", \
					    #z, strerror(errno)); \
					return 1; \
				} \
			} while(0)

int UInput::wacom_set_events(struct uinput_info *info) {
	/* common */
	set_event(info, UI_SET_EVBIT, EV_KEY);
	set_event(info, UI_SET_EVBIT, EV_ABS);

	set_event(info, UI_SET_KEYBIT, BTN_0);
	set_event(info, UI_SET_KEYBIT, BTN_1);

	set_event(info, UI_SET_ABSBIT, ABS_PRESSURE);

	return 0;
}

int UInput::wacom_set_initial_values(struct uinput_info *info,
				     struct uinput_user_dev *dev)
{
	strcpy(dev->name, "Wacom: Wacom Bamboo");
	dev->id.bustype = BUS_VIRTUAL;
	dev->id.vendor = USB_VENDOR_ID_WACOM;
	dev->id.product = 0;
	dev->id.version = 9999;	/* XXX */

	/* common */
	dev->absmax[ABS_PRESSURE] = 2048;

	return uinput_write_dev(info, dev);
}

int UInput::uinput_write_dev(struct uinput_info *info,
		     struct uinput_user_dev *dev)
{
	int rc = 1;
	int retval;

	retval = write(info->fd, dev, sizeof(*dev));
	if (retval < 0) {
		perror("Unable to create input device");
		goto err;
	}
	if (retval != sizeof(*dev)) {
		fprintf(stderr, "Short write while creating input device. "
			"Different versions?\n");
		goto err;
	}
	rc = 0;

err:
	return rc;
}

int UInput::uinput_create(struct uinput_info *info)
{
	struct uinput_user_dev dev;
	char file[50], *tmp;
	int retval = -1;
	int need_init = 0;

	tmp = getenv("UINPUT_DEVICE");
	if (!tmp)
		tmp = UINPUT_DEVICE;
	snprintf(file, sizeof(file), "%s", tmp);

	info->fd = open(file, O_RDWR);
	if (info->fd < 0) {
		log(LOG_ERR, "Unable to open uinput file %s: %s\n", file,
		    strerror(errno));
		return -1;
	}

	memset(&dev, 0, sizeof(dev));

	if (wacom_set_initial_values(info, &dev))
		goto err;

	switch(info->create_mode) {
		case SELF_CREATE:
			break;
		case WDAEMON_CREATE:
			need_init = 1;
			break;
		default:
			fprintf(stderr, "Invalid create mode for device.\n");
			return -1;
	}

	if (wacom_set_events(info))
		goto err;

	if (need_init) {
		retval = ioctl(info->fd, UI_DEV_CREATE);
		if (retval == -1) {
			perror("Unable to create uinput device: ");
			goto err;
		}
	}

	retval = 0;
out:
	return retval;
err:
	close(info->fd);
	goto out;
}

int UInput::uinput_write_event(struct uinput_info *info, struct input_event *ev)
{
	if (write(info->fd, ev, sizeof(struct input_event)) !=
						sizeof(struct input_event)) {
		perror("Error writing uinput event: ");
		return 1;
	}
	return 0;
}

/* vim: set noexpandtab tabstop=8 shiftwidth=8: */
