/** -*- mode: c++; tab-width: 4; indent-tabs-mode: t; -*-
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

#include <glib.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>

#include <string>
#include <map>
#include <vector>
using std::string;   // we should be using a Glibmm::ustring for most of this ....

/* some basic utilities */
#define S(str) std::string(str)
#define ifnull(expr1, expr2) (expr1 ? expr1 : expr2)
#define foreach(type, list) for (type::iterator i = list.begin(); i != list.end(); i++)

enum image_type
{
    IMAGE_TYPE_THEME = 0,
    IMAGE_TYPE_ABSOLUTE = 1,
    IMAGE_TYPE_RAW = 2
};

class Image
{
public:
    enum image_type type;
    char *file;
    unsigned char *data;
    size_t datalen;

    Image(char *newfile);

    /* newdata will be owned by this object and freed on destruction */
    Image(unsigned char *newdata, int newdatalen);
    
    virtual ~Image();
};

/* This class represents a notification. It's a class rather than a struct
   so notifiers can subclass it and append whatever information or functionality
   they want. For instance, a PopupNotifier might want to add layout information
   here.
 */

typedef std::map<int, char*> ActionsMap;
typedef std::vector<Image*> ImageList;

class Notification
{
public:
    int urgency;              /* Urgency level */
    char *summary;            /* UTF-8 encoded text containing a brief description */
    char *body;               /* UTF-8 encoded body, optionally containing markup */
    ImageList images;         /* an array of frames in the animated image. would this be better as a ptr array? */
    int primary_frame;        /* for notifiers that can't show animations, the still frame to use */
    int timeout;             /* 0 means use heuristics */
    bool use_timeout;         /* should the notification ever time out? */

    ActionsMap actions;       /* the mapping of action ids to action strings */

    int id;

    /* the connection which generated this notification. used for signal dispatch */
    DBusConnection *connection;

    Notification();
    Notification(const Notification &obj);
    virtual ~Notification();

    virtual void update() {;} /* called when the contents have changed */

    virtual void action_invoke(uint aid);
};

typedef std::map<int, Notification*> NotificationsMap;

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

extern BaseNotifier *notifier;    /* This holds the backend in use. It's set once, at startup. */


class ConsoleNotifier : public BaseNotifier
{
public:
    virtual uint notify(Notification *n);
    virtual bool unnotify(uint id);

    ConsoleNotifier(GMainLoop *loop) : BaseNotifier(loop) {};
};

class PopupNotifier : public BaseNotifier
{
private:
    void reflow();

public:
    virtual uint notify(Notification *n);
    virtual bool unnotify(Notification *n);

    virtual Notification *create_notification();

    void handle_button_release(Notification *n);

    PopupNotifier(GMainLoop *loop, int *argc, char ***argv);
};

#endif
