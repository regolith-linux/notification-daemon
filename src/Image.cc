/**
 * @file image.cpp Implementation of the image class
 *
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

#include "string.h"
#include "notifier.hh"

Image::Image(char *newfile)
{
    data = NULL;
    datalen = 0;
    file = strdup(newfile);
        
    if (file[0] == '/') type = IMAGE_TYPE_ABSOLUTE;
    else type = IMAGE_TYPE_THEME;
}

/* newdata will be owned by this object and freed on destruction */
Image::Image(unsigned char *newdata, int newdatalen)
{
    data = newdata;
    datalen = newdatalen;
    type = IMAGE_TYPE_RAW;
    file = NULL;
}

Image::~Image()
{
    if (data) dbus_free(data); /* was allocated by DBUS */
    if (file) free(file);
}
