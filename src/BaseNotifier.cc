/** -*- mode: c-mode; tab-width: 4; indent-tabs-mode: t; -*-
 * @file notifier.cpp Base class implementations
 *
 * Copyright (C) 2004 Mike Hearn <mike@navi.cx>
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
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h> // for GMainLoop
#include <time.h>

#include "BaseNotifier.hh"
#include "Notification.hh"

#include "dbus-compat.h"
#include "logging.hh"

BaseNotifier::BaseNotifier(GMainLoop *main_loop)
{
    loop = main_loop;

    g_main_loop_ref(loop);

    next_id = 1;
    timing = false;
}

BaseNotifier::~BaseNotifier()
{
    g_main_loop_unref(loop);
}

/* returns true if more heartbeats are needed */
bool BaseNotifier::timeout()
{
    /* check each notification to see if it timed out yet */
    time_t now = time(NULL);
    bool needed = false;

    TRACE("timeout\n");

	for (NotificationsMap::const_iterator i = notifications.begin();
		 i != notifications.end();
		 i++)
	{
		Notification *n = i->second;

        if (n->GetUseTimeout())
			needed = true;

        if (n->GetUseTimeout() && n->GetTimeout() <= now) {
            unnotify(n);
            break;
        }
    }

    TRACE("heartbeat: %d, %d notifications left\n", now, notifications.size());

    return needed;
}

/* called by the glib main loop */
static gboolean timeout_dispatch(gpointer data)
{
    BaseNotifier *n = (BaseNotifier *) data;

    bool ret = n->timeout();
    if (!ret) n->timing = false;

    return ret ? TRUE : FALSE;
}

void BaseNotifier::register_timeout(int hz)
{
    g_timeout_add(hz, (GSourceFunc) timeout_dispatch, this);
}

void BaseNotifier::setup_timeout(Notification *n)
{
    /*
	 * Decide a sensible timeout. For now, let's just use 5 seconds.
	 * In the future, maybe make this based on text length?
	 */
    if (n->GetUseTimeout() && n->GetTimeout() == 0)
		n->SetTimeout(time(NULL) + 5);

    /* we don't have a timeout triggering constantly as otherwise n-d
       could never be fully paged out by the kernel. */

    if (n->GetUseTimeout() && !timing)
    {
        register_timeout(1000);
        timing = true; /* set to false when ::timeout returns false */
    }
}


uint BaseNotifier::notify(Notification *n)
{
    n->SetId(next_id++);

    update(n);  // can throw

    /* don't commit to the map until after the notification has been able to update */
    notifications[n->GetId()] = n;

    return n->GetId();
}

void BaseNotifier::update(Notification *n)
{
    setup_timeout(n);
    n->update();  // can throw
}

bool BaseNotifier::unnotify(uint id)
{
    Notification *n = get(id);

    validate( n != NULL, false, "Given ID (%d) is not valid", id );

    return unnotify(n);
}

bool BaseNotifier::unnotify(Notification *n)
{
    if (!notifications.erase(n->GetId()))
    {
        WARN("no such notification registered (%p), id=%d\n", n, n->GetId());
        return false;
    }

    TRACE("deleting due to unnotify (%p)\n", n);
    delete n;

    return true;
}

Notification* BaseNotifier::create_notification(DBusConnection *dbusConn)
{
    /*
	 * Base classes override this to add extra info and abilities
	 * to the Notification class
	 */
    return new Notification(dbusConn);
}

Notification *BaseNotifier::get(uint id)
{
    if (notifications.find(id) == notifications.end()) return NULL;
    return notifications[id];
}

