/** -*- mode: c++-mode; tab-width: 4; indent-tabs-mode: t; -*-
 * @file notifier.h Base class for notification backends
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

#ifndef NOTIFIER_H
#define NOTIFIER_H

#include <string>
using std::string;

/* some basic string utilities */
#define equal(s1,s2) (!strcmp(s1,s2))
#define $(s) ((char*)s.c_str())    // I can't actually believe that syntax works, but it does ...

struct image {
    /* fill me in */
};

struct sound {
    /* fill me in */
};

/* This class represents a notification. It's a class rather than a struct
   so notifiers can subclass it and append whatever information or functionality
   they want. For instance, a PopupNotifier might want to add layout information
   here.
 */

class Notification {
public:    
    char *summary;            /* UTF-8 encoded text containing a brief description */
    char *body;               /* UTF-8 encoded body, optionally containing markup */
    struct image **images;    /* an array of frames in the animated image */
    int primary_frame;        /* for notifiers that can't show animations, the still frame to use */
    char *sound;              /* the sound to play when the notification appears */
    int timeout;              /* 0 means use heuristics */
    bool use_timeout;         /* should the notification ever time out? */

    Notification();
    virtual ~Notification();
};

class BaseNotifier {
public:
    virtual void notify(Notification *n) = 0;
    virtual void unnotify(Notification *n) = 0;
    virtual ~BaseNotifier() { };

    /* This can be overriden by base classes to return subclasses of Notification */
    virtual Notification *create_notification();
};

extern BaseNotifier *notifier;    /* This holds the backend in use. It's set once, at startup. */

class ConsoleNotifier : public BaseNotifier {
public:    
    virtual void notify(Notification *n);
    virtual void unnotify(Notification *n);
};

#endif
