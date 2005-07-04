/* -*- mode: c++; tab-width: 4; indent-tabs-mode: t; -*- */
/**
 * @file Notification.cc Base notification implementation
 *
 * Copyright (C) 2005 Christian Hammond <chipx86@chipx86.com>
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
#include "Notification.hh"
#include "logging.hh"
#include "dbus-compat.h"

Notification::Notification()
{
    primary_frame = -1;
    timeout = 0;
    use_timeout = false;
    id = 0;
}

Notification::Notification(const Notification &obj)
{
    primary_frame = obj.primary_frame;
    timeout = obj.timeout;
    use_timeout = obj.use_timeout;
    id = obj.id;
}

Notification::~Notification()
{
    TRACE("~Notification: %s, %s\n", summary.c_str(), body.c_str());

	for (ImageList::iterator i = images.begin(); i != images.end(); i++)
		delete *i;
}

void Notification::action_invoke(uint actionid)
{
    DBusMessage *signal = dbus_message_new_signal("/org/freedesktop/Notifications",
                                                  "org.freedesktop.Notifications",
                                                  "ActionInvoked");

    TRACE("sending Invoked signal on notification id %d, action id %d\n", id, actionid);

	DBusMessageIter iter;
	dbus_message_iter_init_append(signal, &iter);

	_notifyd_dbus_message_iter_append_uint32(&iter, id);
	_notifyd_dbus_message_iter_append_uint32(&iter, actionid);

    dbus_connection_send(connection, signal, NULL);

    dbus_message_unref(signal);
}
