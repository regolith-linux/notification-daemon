/**
 * @file PopupNotifier.cc GTK+ based popup notifier
 *
 * Copyright (C) 2004 Mike Hearn <mike@navi.cx>
 * Copyright (C) 2004 Christian Hammond <chipx86@chipx86.com>
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
#include <iostream>
#include <sstream>
#include <exception>
#include <stdexcept>
#include <assert.h>

#include "PopupNotifier.hh"
#include "PopupNotification.hh"
#include "Notification.hh"
#include "logging.hh"

PopupNotifier::PopupNotifier(GMainLoop *main_loop, int *argc, char ***argv)
    : BaseNotifier(main_loop)
{
    gtk_init(argc, argv);
}

/*
 * This method is responsible for calculating the height offsets of all
 * currently displayed notifications. In future, it may take into account
 * animations and such.
 *
 * This may be called many times per second so it should be reasonably fast.
 */
void
PopupNotifier::reflow()
{
    /* the height offset is the distance from the top/bottom of the
       screen to the nearest edge of the popup */

    int offset = 0;
    int offsub = 0;

    for (NotificationsMap::iterator i = notifications.begin();
         i != notifications.end();
         i++, offsub++)
    {
        PopupNotification *n = dynamic_cast<PopupNotification *>(i->second);
        assert(n != NULL);

        n->set_height_offset(offset - offsub);

        offset += n->get_height();
    }
}

uint
PopupNotifier::notify(Notification *base)
{
    PopupNotification *n = dynamic_cast<PopupNotification*> (base);
    assert( n != NULL );

    uint id = BaseNotifier::notify(base);  // can throw

    reflow();

    n->show();

    return id;
}

bool
PopupNotifier::unnotify(Notification *n)
{
    bool ret = BaseNotifier::unnotify(n);

    reflow();

    return ret;
}

Notification*
PopupNotifier::create_notification(DBusConnection *dbusConn)
{
    return new PopupNotification(this, dbusConn);
}
