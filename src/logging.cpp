/** -*- mode: c++-mode; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4; -*-
 * @file logging.cpp Logging support
 *
 * Copyright (C) 2004 Mike Hearn
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 */

#include "logging.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void log(enum loglevel level, char *s, ...) {
    va_list args;

	fprintf(stderr, "notification-daemon: ");
	
    switch (level) {
        case LOG_WARNING: fprintf(stderr, "warning: "); break;
        case LOG_TRACE: fprintf(stderr, "trace: "); break;
        case LOG_ERROR: fprintf(stderr, "error: "); break;
        case LOG_FIXME: fprintf(stderr, "fixme: "); break;
    }
                                                    
    va_start(args, s);
    vprintf(s, args);
    va_end(args);
}
