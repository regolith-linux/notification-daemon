/** -*- mode: c++; tab-width: 4; indent-tabs-mode: t; -*-
 * @file notifier.h Base class for notification backends
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

#ifndef _NOTIFYD_NOTIFIER_HH_
#define _NOTIFYD_NOTIFIER_HH_

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <glib.h>

#include <string>
#include <map>
#include <vector>

#include "Image.hh"

/* some basic utilities */
#define S(str) std::string(str)

/* This class represents a notification. It's a class rather than a struct
   so notifiers can subclass it and append whatever information or functionality
   they want. For instance, a PopupNotifier might want to add layout information
   here.
 */

#include "BaseNotifier.hh"

extern BaseNotifier *notifier;    /* This holds the backend in use. It's set once, at startup. */

#endif /* _NOTIFYD_NOTIFIER_HH_ */
