/*
 Copyright (C) 2001-2006, William Joseph.
 All Rights Reserved.

 This file is part of GtkRadiant.

 GtkRadiant is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 GtkRadiant is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with GtkRadiant; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "image.h"

#include <gtk/gtk.h>

#include "string/string.h"
#include "stream/stringstream.h"
#include "stream/textstream.h"

namespace
{
	std::string g_bitmapsPath;
}

void BitmapsPath_set (const std::string& path)
{
	g_bitmapsPath = path;
}

GdkPixbuf* pixbuf_new_from_file_with_mask (const std::string& fileName)
{
	GdkPixbuf* rgb = gdk_pixbuf_new_from_file(fileName.c_str(), 0);
	if (rgb == 0) {
		return 0;
	} else {
		GdkPixbuf* rgba = gdk_pixbuf_add_alpha(rgb, TRUE, 255, 0, 255);
		gdk_pixbuf_unref(rgb);
		return rgba;
	}
}

GtkImage* image_new_from_file_with_mask (const std::string& fileName)
{
	GdkPixbuf* rgba = pixbuf_new_from_file_with_mask(fileName);
	if (rgba == 0) {
		return 0;
	} else {
		GtkImage* image = GTK_IMAGE(gtk_image_new_from_pixbuf(rgba));
		gdk_pixbuf_unref(rgba);
		return image;
	}
}

GtkImage* image_new_missing ()
{
	return GTK_IMAGE(gtk_image_new_from_stock(GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_SMALL_TOOLBAR));
}

static inline GtkImage* new_image (const std::string& fileName)
{
	GtkImage* image = image_new_from_file_with_mask(fileName);
	if (image != 0)
		return image;

	return image_new_missing();
}

GtkImage* new_local_image (const std::string& fileName)
{
	return new_image(g_bitmapsPath + fileName);
}

namespace gtkutil
{
	// Return a GdkPixbuf from a local image
	GdkPixbuf* getLocalPixbuf (const std::string& fileName)
	{
		return gdk_pixbuf_new_from_file(std::string(g_bitmapsPath + fileName).c_str(), NULL);
	}

	GdkPixbuf* getLocalPixbufWithMask (const std::string& fileName)
	{
		return pixbuf_new_from_file_with_mask(g_bitmapsPath + fileName);
	}
} // namespace gtkutil
