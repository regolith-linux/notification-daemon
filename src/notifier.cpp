/** -*- mode: c-mode; tab-width: 4; indent-tabs-mode: t; -*-
 * @file notifier.cpp Base class implementations
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

#include "notifier.h"

Notification::Notification()
{
	summary = body = sound = NULL;
	images = NULL;
	primary_frame = -1;
	timeout = 0;
	use_timeout = true;
}

Notification::~Notification()
{
	if (this->summary) free(this->summary);
	if (this->body) free(this->body);
	// FIXME: free images/sound data
}

Notification*
BaseNotifier::create_notification()
{
	return new Notification();
}
