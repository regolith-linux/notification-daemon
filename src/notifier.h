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

/* some basic string utilities */
#define equal(s1,s2) (!strcmp(s1,s2))
#define $(s) ((char*)s.c_str())    // I can't actually believe that syntax works, but it does ...

struct image {
    /* fill me in */
};

struct sound {
    /* fill me in */
};

struct notification {
    char *summary;           /* UTF-8 encoded text containing a brief description */
    char *body;              /* UTF-8 encoded body, optionally containing markup */
    struct image **images;   /* an array of frames in the animated image */
    int primary_frame;       /* for notifiers that can't show animations, the still frame to use */
    struct sound *sound;     /* the sound to play when the notification appears */
    int timeout;
};

class BaseNotifier {
public:
    virtual void notify(struct notification *n) = 0;
    virtual void unnotify(struct notification *n) = 0;
    virtual ~BaseNotifier() { };
};

extern BaseNotifier *notifier;    /* This holds the backend in use. It's set once, at startup. */

class ConsoleNotifier : public BaseNotifier {
public:    
    virtual void notify(struct notification *n);
    virtual void unnotify(struct notification *n);
};

