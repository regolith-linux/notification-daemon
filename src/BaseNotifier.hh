/* -*- mode: c++; tab-width: 4; indent-tabs-mode: t; -*- */
/**
 * @file BaseNotifier.hh Base notifier implementation
 *
 * Copyright (C) 2005 Christian Hammond
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
#ifndef _NOTIFYD_BASE_NOTIFIER_HH
#define _NOTIFYD_BASE_NOTIFIER_HH

#include "Notification.hh"

class BaseNotifier
{
protected:
    uint next_id;
    GMainLoop *loop;

    void register_timeout(int hz);

    void setup_timeout(Notification *n);

public:
    /* All notifications are given a unique, non-repeating id which the client can use
       The mapping between the ids and notification objects is stored here */

    NotificationsMap notifications;

    Notification *get(uint id);

    virtual uint notify(Notification *n);
    bool unnotify(uint id);
    virtual bool unnotify(Notification *n);
    virtual void update(Notification *n);

    BaseNotifier(GMainLoop *loop);
    virtual ~BaseNotifier();

    /* This can be overriden by base classes to return subclasses of Notification */
    virtual Notification *create_notification();

    bool timing;
    virtual bool timeout();

};

#endif /* _NOTIFYD_BASE_NOTIFIER_HH */
