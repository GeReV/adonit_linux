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

#ifndef LOG_H
#define LOG_H
#include <syslog.h>
extern int log_use_stdout;
static void inline set_log_stdout(void)
{
	log_use_stdout = 1;
}

static void inline set_log_syslog(void)
{
	log_use_stdout = 0;
}

#define log(l, x...) do { \
		if (log_use_stdout) { \
			fprintf(stderr, x); \
		} else \
			syslog(l, x); \
	} while(0)
#endif	/* LOG_H */

