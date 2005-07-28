/* -*- mode: c++; tab-width: 4; indent-tabs-mode: t; -*- */
/**
 * @file Hint.hh Hint class
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
#ifndef _NOTIFYD_HINT_HH_
#define _NOTIFYD_HINT_HH_

#include <map>

class Hint
{
public:
	enum Type { UNSET, STRING, INTEGER, BOOLEAN };

	Hint() :mType(UNSET) {};
	Hint(const std::string &key, const std::string &value)
		: mType(STRING), mKey(key), mString(value)
	{
	}

	Hint(const std::string &key, dbus_uint32_t value)
		: mType(INTEGER), mKey(key), mInteger(value)
	{
	}

	Hint(const std::string &key, bool value)
		: mType(BOOLEAN), mKey(key), mBoolean(value)
	{
	}

	const std::string &GetKey(void) const { return mKey; }
	Type GetType(void) const { return mType; }

	const std::string &GetString(void) const { return mString; }
	int GetInteger(void) const { return mInteger; }
	bool GetBoolean(void) const { return mBoolean; }

private:
	Type mType;
	std::string mKey;
	std::string mString;
	int mInteger;
	bool mBoolean;
};

typedef std::map<std::string, Hint> HintMap;

#endif /* _NOTIFYD_HINT_HH_ */
