#ifndef __VTF_H__
#define __VTF_H__

#include <stdio.h>
#include <glib.h>


/* Format */
#define VTF_FORMAT_NONE					-1
#define VTF_FORMAT_RGBA8888				0
#define VTF_FORMAT_ABGR8888				1
#define VTF_FORMAT_RGB888				2
#define VTF_FORMAT_BGR888				3
#define VTF_FORMAT_RGB565				4
#define VTF_FORMAT_I8					5
#define VTF_FORMAT_IA88					6
#define VTF_FORMAT_P8					7
#define VTF_FORMAT_A8					8
#define VTF_FORMAT_RGB888_BLUESCREEN	9
#define VTF_FORMAT_BGR888_BLUESCREEN	10
#define VTF_FORMAT_ARGB8888				11
#define VTF_FORMAT_BGRA8888				12
#define VTF_FORMAT_DXT1					13
#define VTF_FORMAT_DXT3					14
#define VTF_FORMAT_DXT5					15
#define VTF_FORMAT_BGRX8888				16
#define VTF_FORMAT_BGR565				17
#define VTF_FORMAT_BGRX5551				18
#define VTF_FORMAT_BGRA4444				19
#define VTF_FORMAT_DXT1_ONEBITALPHA		20
#define VTF_FORMAT_BGRA5551				21
#define VTF_FORMAT_UV88					22
#define VTF_FORMAT_UVWQ8888				23
#define VTF_FORMAT_RGBA16161616F		24
#define VTF_FORMAT_RGBA16161616			25
#define VTF_FORMAT_UVLX8888				26


#define VTF_ERROR vtf_error_quark ()

typedef enum {
	VTF_ERROR_INVALID,
	VTF_ERROR_UNSUPPORTED,
	VTF_ERROR_DIMENTION,
	VTF_ERROR_FORMAT,
	VTF_ERROR_RESOURCE
} VtfErrorEnum;



typedef struct _Vtf Vtf;

Vtf *vtf_open (const gchar *fname, GError **error);
Vtf *vtf_open_fd (FILE *fd, GError **error);
Vtf *vtf_open_mem (gpointer data, gsize length, GError **error);
void vtf_close (Vtf *vtf);
guint vtf_get_width (Vtf *vtf);
guint vtf_get_height (Vtf *vtf);
guint vtf_get_format (Vtf *vtf);
const gchar *vtf_get_format_name (gint format);
guint vtf_get_frame_count (Vtf *vtf);
void *vtf_get_image (Vtf *vtf, int frame);
void *vtf_get_image_rgba (Vtf *vtf, int frame);

GQuark vtf_error_quark (void);

#endif
