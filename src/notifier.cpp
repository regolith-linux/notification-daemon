/** -*- mode: c-mode; tab-width: 4; indent-tabs-mode: t; -*-
 * @file notifier.cpp Base class for notification backends
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

/* This file contains the base class for notification backends.
   
   A backend is reponsible for taking a notification and displaying
   it to the user in some form. For instance, you could have a passive
   popup notifier, a console notifier, or a notifier that did something
   entirely different - perhaps a marquee in a panel applet.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

