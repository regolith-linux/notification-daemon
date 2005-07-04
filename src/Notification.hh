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

    Notification(DBusConnection *dbusConn);
    Notification(const Notification &obj);
    virtual ~Notification();

	/* Called when the contents have changed */
    virtual void update() {;}

    virtual void action_invoke(uint aid);

	void SetUrgencyLevel(int urgencyLevel);
	int GetUrgencyLevel(void) const;

	void SetSummary(const std::string &summary);
	const std::string &GetSummary(void) const;

	void SetBody(const std::string &body);
	const std::string &GetBody(void) const;

	void AddImage(Image *image);
	const ImageList &GetImages(void) const;
	ImageList &GetImages(void);

	int GetPrimaryFrame(void) const;

	void SetTimeout(int timeout);
	int GetTimeout(void) const;

	void SetUseTimeout(bool useTimeout);
	bool GetUseTimeout(void) const;

	void AddAction(int id, const std::string &value);
	const std::string &GetAction(int id) const;

	const ActionsMap &GetActions(void) const;
	ActionsMap &GetActions(void);

	void SetHint(const std::string &key, const std::string &value);
	const std::string &GetHint(const std::string &key) const;
	bool HasHint(const std::string &key) const;

	const HintsMap &GetHints(void) const;
	HintsMap &GetHints(void);

	void SetId(int id);
	int GetId(void) const;

private:
    int mUrgency;         /* Urgency level */
    std::string mSummary; /* UTF-8 encoded text containing a brief
							 description */
    std::string mBody;    /* UTF-8 encoded body, optionally containing markup */
    ImageList mImages;    /* An array of frames in the animated image. would
							 this be better as a ptr array? */
    int mPrimaryFrame;    /* For notifiers that can't show animations, the
							 still frame to use */
    int mTimeout;         /* 0 means use heuristics */
    bool mUseTimeout;     /* Should the notification ever time out? */

    ActionsMap mActions;  /* The mapping of action ids to action strings */
	HintsMap mHints;      /* The mapping of hints. */

    int mId;

    /*
	 * The connection which generated this notification. Used for signal
	 * dispatch.
	 */
    DBusConnection *mDBusConn;
};

typedef std::map<int, Notification *> NotificationsMap;

#endif /* _NOTIFYD_NOTIFICATION_HH */
