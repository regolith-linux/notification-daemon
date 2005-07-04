/**
 * @file Image.cpp Implementation of the image class
 *
 * Copyright (C) 2004 Mike Hearn <mike@navi.cx>
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
#include "Image.hh"

Image::Image(const std::string &file)
	: mFile(file),
	  mData(NULL),
	  mDataLen(0)
{
	if (file[0] == '/')
		mType = ABSOLUTE;
	else
		mType = THEME;
}

Image::Image(const unsigned char *data,
			 int dataLen)
	: mType(RAW)
{
	mData = new unsigned char[dataLen];
	memcpy(mData, data, dataLen);
}

Image::~Image()
{
	delete mData;
}

Image::Type
Image::GetType(void)
	const
{
	return mType;
}

const std::string &
Image::GetFile(void)
	const
{
	return mFile;
}

void
Image::GetData(unsigned char **ret_data, size_t *ret_data_len)
{
	if (ret_data != NULL)
		*ret_data = mData;

	if (ret_data_len != NULL)
		*ret_data_len = mDataLen;
}
