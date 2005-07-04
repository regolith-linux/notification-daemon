/* -*- mode: c++; tab-width: 4; indent-tabs-mode: t; -*- */
/**
 * @file Image.hh Image class
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
#ifndef _NOTIFYD_IMAGE_HH_
#define _NOTIFYD_IMAGE_HH_

#include <string>
#include <vector>

class Image
{
public:
	enum Type { THEME, ABSOLUTE, RAW };

    Image(const std::string &file);
    Image(const unsigned char *data, int len);
    virtual ~Image();

	Type GetType(void) const;
	const std::string &GetFile(void) const;
	void GetData(unsigned char **ret_data, size_t *ret_data_len);

private:
    Type mType;
	std::string mFile;
    unsigned char *mData;
    size_t mDataLen;
};

typedef std::vector<Image *> ImageList;

#endif /* _NOTIFYD_IMAGE_HH_ */
