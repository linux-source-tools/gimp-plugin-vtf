/* 
 * gdkpixbuf-loader-vtf.c
 * 
 * Copyright (C) 2012 VTF loader developers
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 * 
 */



#include <gdk-pixbuf/gdk-pixbuf.h>
#include "vtf.h"


typedef struct _LoadContext {
	GByteArray *buffer;
	GdkPixbufModulePreparedFunc prepared;
	gpointer udata;
} LoadContext;


static GdkPixbuf *
gdk_pixbuf__vtf_image_load (FILE *fd, GError **error)
{
	GdkPixbuf *pixbuf = NULL;
	Vtf *vtf = vtf_open_fd (fd, error);
	if (!vtf)
		return NULL;
	
	guchar *data = vtf_get_image_rgba (vtf, 0);
	if (!data) {
		g_set_error (error, VTF_ERROR, VTF_ERROR_FORMAT,
				"Format %s is not supported",
				vtf_get_format_name (vtf_get_format (vtf)));
		goto finish;
	}
	
	gint width = vtf_get_width (vtf);
	gint height = vtf_get_height (vtf);
	pixbuf = gdk_pixbuf_new_from_data (data, GDK_COLORSPACE_RGB,
			TRUE, 8, width, height, width * 4, NULL, NULL);
	
finish:
	vtf_close (vtf);
	return pixbuf;
}


static gpointer
gdk_pixbuf__vtf_image_begin_load (GdkPixbufModuleSizeFunc size_func,
		GdkPixbufModulePreparedFunc prepare_func,
		GdkPixbufModuleUpdatedFunc update_func, gpointer udata, GError **error)
{
	LoadContext *lc = g_slice_new (LoadContext);
	lc->buffer = g_byte_array_sized_new (5 * 1024);
	lc->prepared = prepare_func;
	lc->udata = udata;
	return lc;
}


static void
free_buffer (guchar *pixels, gpointer data)
{
	g_free (pixels);
}


static gboolean
gdk_pixbuf__vtf_image_stop_load (gpointer context_ptr, GError **error)
{
	LoadContext *lc = context_ptr;
	
	Vtf *vtf = vtf_open_mem (lc->buffer->data, lc->buffer->len, error);
	if (!vtf)
		goto err;
	
	guchar *data = vtf_get_image_rgba (vtf, 0);
	if (!data) {
		g_set_error (error, VTF_ERROR, VTF_ERROR_FORMAT,
				"Format %s is not supported",
				vtf_get_format_name (vtf_get_format (vtf)));
		goto err;
	}
	
	gint width = vtf_get_width (vtf);
	gint height = vtf_get_height (vtf);
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_data (data, GDK_COLORSPACE_RGB,
			TRUE, 8, width, height, width * 4, free_buffer, NULL);
	
	GdkPixbufSimpleAnim *anim = NULL;
	gint i, frames = vtf_get_frame_count (vtf);
	if (frames > 0) {
		GdkPixbuf *frame;
		anim = gdk_pixbuf_simple_anim_new (width, height, 10.0f);
		gdk_pixbuf_simple_anim_set_loop (anim, TRUE);
		for (i = 0; i < frames; i++) {
			data = vtf_get_image_rgba (vtf, i);
			frame = gdk_pixbuf_new_from_data (data, GDK_COLORSPACE_RGB,
					TRUE, 8, width, height, width * 4, free_buffer, NULL);
			gdk_pixbuf_simple_anim_add_frame (anim, frame);
			g_object_unref (frame);
		}
	}
	
	lc->prepared (pixbuf, GDK_PIXBUF_ANIMATION (anim), lc->udata);
	g_object_unref (pixbuf);
	g_object_unref (anim);
	
	vtf_close (vtf);
	g_byte_array_free (lc->buffer, TRUE);
	g_slice_free (LoadContext, lc);
	return TRUE;
	
err:
	g_byte_array_free (lc->buffer, TRUE);
	g_slice_free (LoadContext, lc);
	if (vtf)
		vtf_close (vtf);
	return FALSE;
}


static gboolean
gdk_pixbuf__vtf_image_load_increment (gpointer context_ptr, const guchar *data,
		guint size, GError **error)
{
	LoadContext *lc = context_ptr;
	g_byte_array_append (lc->buffer, data, size);
	return TRUE;
}



#ifndef INCLUDE_vtf
	#define MODULE_ENTRY(fn) G_MODULE_EXPORT void fn
#else
	#define MODULE_ENTRY(fn) void _gdk_pixbuf__vtf_##fn
#endif


MODULE_ENTRY (fill_vtable) (GdkPixbufModule* module)
{
	module->load = gdk_pixbuf__vtf_image_load;
	module->begin_load = gdk_pixbuf__vtf_image_begin_load;
	module->stop_load = gdk_pixbuf__vtf_image_stop_load;
    module->load_increment = gdk_pixbuf__vtf_image_load_increment;
}


MODULE_ENTRY (fill_info) (GdkPixbufFormat *info)
{
    static GdkPixbufModulePattern signature[] = {
    	{"VTF\0",	NULL,	100},
    	{NULL,		NULL,	0}
    };
    static gchar * mime_types[] = {
    	"image/x-vtf",
    	NULL
    };
    static gchar * extensions[] = {
    	"vtf",
    	NULL
    };
	
    info->name = "vtf";
    info->signature = signature;
    info->description = "Valve Texture Format";
    info->mime_types = mime_types;
    info->extensions = extensions;
    info->flags = GDK_PIXBUF_FORMAT_THREADSAFE;
    info->license = "LGPL";
}
