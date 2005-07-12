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

Notification::Notification(DBusConnection *dbusConn)
	: mUrgency(0),
	  mPrimaryFrame(-1),
	  mTimeout(0),
	  mUseTimeout(false),
	  mId(0),
	  mDBusConn(dbusConn)
{
}

Notification::Notification(const Notification &obj)
	: mUrgency(obj.GetUrgencyLevel()),
	  mSummary(obj.GetSummary()),
	  mBody(obj.GetBody()),
	  mPrimaryFrame(obj.GetPrimaryFrame()),
	  mTimeout(obj.GetTimeout()),
	  mUseTimeout(obj.GetUseTimeout()),
	  mId(obj.GetId())
{
}

Notification::~Notification()
{
    TRACE("~Notification: %s, %s\n", GetSummary().c_str(), GetBody().c_str());

	for (ImageList::iterator i = mImages.begin(); i != mImages.end(); i++)
		delete *i;
}

void Notification::action_invoke(uint actionid)
{
    DBusMessage *signal = dbus_message_new_signal("/org/freedesktop/Notifications",
                                                  "org.freedesktop.Notifications",
                                                  "ActionInvoked");

    TRACE("sending Invoked signal on notification id %d, action id %d\n",
		  GetId(), actionid);

	DBusMessageIter iter;
	dbus_message_iter_init_append(signal, &iter);

	int id = GetId();
	_notifyd_dbus_message_iter_append_uint32(&iter, id);
	_notifyd_dbus_message_iter_append_uint32(&iter, actionid);

    dbus_connection_send(mDBusConn, signal, NULL);

    dbus_message_unref(signal);
}

void
Notification::SetUrgencyLevel(int urgencyLevel)
{
	mUrgency = urgencyLevel;
}

int
Notification::GetUrgencyLevel(void)
	const
{
	return mUrgency;
}

void
Notification::SetSummary(const std::string &summary)
{
	mSummary = summary;
}

const std::string &
Notification::GetSummary(void)
	const
{
	return mSummary;
}

void
Notification::SetBody(const std::string &body)
{
	mBody = body;
}

const std::string &
Notification::GetBody(void)
	const
{
	return mBody;
}

void
Notification::AddImage(Image *image)
{
	mImages.push_back(image);
}

const ImageList &
Notification::GetImages(void)
	const
{
	return mImages;
}

ImageList &
Notification::GetImages(void)
{
	return mImages;
}

int
Notification::GetPrimaryFrame(void)
	const
{
	return mPrimaryFrame;
}

void
Notification::SetTimeout(int timeout)
{
	mTimeout = timeout;
}

int
Notification::GetTimeout(void)
	const
{
	return mTimeout;
}

void
Notification::SetUseTimeout(bool useTimeout)
{
	mUseTimeout = useTimeout;
}

bool
Notification::GetUseTimeout(void)
	const
{
	return mUseTimeout;
}

void
Notification::AddAction(int id, const std::string &value)
{
	mActions[id] = value;
}

const std::string &
Notification::GetAction(int id)
	const
{
	return (*mActions.find(id)).second;
}

const Notification::ActionsMap &
Notification::GetActions(void)
	const
{
	return mActions;
}

Notification::ActionsMap &
Notification::GetActions(void)
{
	return mActions;
}

void
Notification::SetHint(const std::string &key, const std::string &value)
{
	mHints[key] = value;
}

const std::string &
Notification::GetHint(const std::string &key)
	const
{
	return (*mHints.find(key)).second;
}

bool
Notification::HasHint(const std::string &key)
	const
{
	return mHints.find(key) != mHints.end();
}

const Notification::HintsMap &
Notification::GetHints(void)
	const
{
	return mHints;
}

Notification::HintsMap &
Notification::GetHints(void)
{
	return mHints;
}

void
Notification::SetId(int id)
{
	mId = id;
}

int
Notification::GetId(void)
	const
{
	return mId;
}
