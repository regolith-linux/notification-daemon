/** -*- mode: c++-mode; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4; -*-
 * @file ConsoleNotifier.cc Basic console notifications
 *
 * Copyright (C) 2005 Christian Hammond <chipx86@chipx86.com>
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
#include <stdio.h>

#include "ConsoleNotifier.hh"
#include "Notification.hh"

uint
ConsoleNotifier::notify(Notification *n)
{
    printf("NOTIFICATION: %s\n%s\n\n",
		   n->GetSummary().c_str(), n->GetBody().c_str());

    return BaseNotifier::notify(n);
}

bool
ConsoleNotifier::unnotify(uint id)
{
    return BaseNotifier::unnotify(id);
}