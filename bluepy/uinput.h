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

#ifndef UINPUT_H
#define UINPUT_H
#include <linux/input.h>
#include <linux/uinput.h>

enum create_type {
	WDAEMON_CREATE = 8,	/* wdaemon creates uinput dev */
	SELF_CREATE,		/* backend creates uinput dev */
};

struct uinput_info {
	int fd;
	char name[UINPUT_MAX_NAME_SIZE];
	int (*init_info)(struct uinput_info *info,
			 struct uinput_user_dev *dev);
	int (*enable_events)(struct uinput_info *info,
			     struct uinput_user_dev *dev);
	void *priv;
	enum create_type create_mode;
};

extern int uinput_create(struct uinput_info *info);
extern int uinput_write_dev(struct uinput_info *info, struct uinput_user_dev *dev);
extern int uinput_write_event(struct uinput_info *info, struct input_event *ev);
extern int adonit_set_events(struct uinput_info *info);
extern int adonit_set_initial_values(struct uinput_info *info, struct uinput_user_dev *dev);

#endif	/* UINPUT_H */

/* vim: set noexpandtab tabstop=8 shiftwidth=8: */
