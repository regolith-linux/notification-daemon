/* -*- mode: c++; tab-width: 4; indent-tabs-mode: t; -*- */
/**
 * @file Notification.hh Base notification implementation
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
#ifndef _NOTIFYD_NOTIFICATION_HH
#define _NOTIFYD_NOTIFICATION_HH

#include <gtk/gtk.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>

#include <map>
#include <string>

#include "Image.hh"


/*
 * This class represents a notification. It's a class rather than a struct
 * so notifiers can subclass it and append whatever information or
 * functionality they want. For instance, a PopupNotifier might want to
 * add layout information here.
 */
class Notification
{
public:
	typedef std::map<int, std::string> ActionsMap;
	typedef std::map<std::string, std::string> HintsMap;

    int urgency;              /* Urgency level */
    std::string summary;            /* UTF-8 encoded text containing a brief description */
    std::string body;               /* UTF-8 encoded body, optionally containing markup */
    ImageList images;         /* an array of frames in the animated image. would this be better as a ptr array? */
    int primary_frame;        /* for notifiers that can't show animations, the still frame to use */
    int timeout;             /* 0 means use heuristics */
    bool use_timeout;         /* should the notification ever time out? */

    ActionsMap actions;       /* the mapping of action ids to action strings */
	HintsMap hints;          /* The mapping of hints. */

	GtkWidget *spacer;

    int id;

	int hint_x;
	int hint_y;

    /* the connection which generated this notification. used for signal dispatch */
    DBusConnection *connection;

    Notification();
    Notification(const Notification &obj);
    virtual ~Notification();

    virtual void update() {;} /* called when the contents have changed */

    virtual void action_invoke(uint aid);
};

typedef std::map<int, Notification *> NotificationsMap;

#endif /* _NOTIFYD_NOTIFICATION_HH */
