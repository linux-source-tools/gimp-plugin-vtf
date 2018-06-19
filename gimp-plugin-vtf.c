#include <string.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include "vtf.h"


static gint32 file_vtf_load_image (const gchar *fname, GError **error);
static gint32 file_vtf_load_thumbnail_image (const gchar *fname,
		gint *width, gint *height, GError **error);
static GimpPDBStatusType file_vtf_save_image (const gchar *fname,
		gint32 image, gint32 run_mode, GError **error);



#define LOAD_PROC			"file-vtf-load"
#define LOAD_THUMB_PROC		"file-vtf-load-thumb"
#define SAVE_PROC			"file-vtf-save"



static void
query ()
{
	static const GimpParamDef load_args[] = {
		{GIMP_PDB_INT32,	"run-mode",		"The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }"},
		{GIMP_PDB_STRING,	"filename",		"The name of the file to load"},
		{GIMP_PDB_STRING,	"raw-filename",	"The name entered"}
	};
	static const GimpParamDef load_return_vals[] = {
		{GIMP_PDB_IMAGE,	"image",		"Output image"}
	};
	
	static const GimpParamDef thumb_args[] = {
		{GIMP_PDB_STRING,	"filename",		"The name of the file to load"},
		{GIMP_PDB_INT32,	"thumb-size",	"Preferred thumbnail size"}
	};
	static const GimpParamDef thumb_return_vals[] = {
		{GIMP_PDB_IMAGE,	"image",		"Thumbnail image"},
		{GIMP_PDB_INT32,	"image-width",	"Width of full-sized image"},
		{GIMP_PDB_INT32,	"image-height",	"Height of full-sized image"}
	};
	
	static const GimpParamDef save_args[] = {
		{GIMP_PDB_INT32,	"run-mode",		"The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }"},
		{GIMP_PDB_IMAGE,	"image",		"Input image"},
		{GIMP_PDB_DRAWABLE,	"drawable",		"Drawable to save"},
		{GIMP_PDB_STRING,	"filename",		"The name of the file to save the image in"},
		{GIMP_PDB_STRING,	"raw-filename",	"The name entered"}
	};
	
	gimp_install_procedure (LOAD_PROC,
			"Loads files of Valve Texture File format",
			"Loads files of Valve Texture File format",
			"Alexander AB <lxndr87i@gmail.com>",
			"Alexander AB <lxndr87i@gmail.com>",
			"2012",
			"Valve Texture",
			NULL,
			GIMP_PLUGIN,
			G_N_ELEMENTS (load_args),
			G_N_ELEMENTS (load_return_vals),
			load_args,
			load_return_vals);
	gimp_register_file_handler_mime (LOAD_PROC, "image/x-vtf");
	gimp_register_magic_load_handler (LOAD_PROC,
			"vtf",
			"",
			"0,string,VTF\\000");
	
	gimp_install_procedure (LOAD_THUMB_PROC,
			"Loads a preview from an Valve Texture file",
			"",
			"Alexander AB <lxndr87i@gmail.com>",
			"Alexander AB <lxndr87i@gmail.com>",
			"2012",
			NULL,
			NULL,
			GIMP_PLUGIN,
			G_N_ELEMENTS (thumb_args),
			G_N_ELEMENTS (thumb_return_vals),
			thumb_args,
			thumb_return_vals);
	gimp_register_thumbnail_loader (LOAD_PROC, LOAD_THUMB_PROC);
	
	gimp_install_procedure (SAVE_PROC,
			"Saves files in Valve Texture File format",
			"Saves files in Valve Texture File format",
			"Alexander AB <lxndr87i@gmail.com>",
			"Alexander AB <lxndr87i@gmail.com>",
			"2012",
			"Valve Texture",
			"*",
			GIMP_PLUGIN,
			G_N_ELEMENTS (save_args),
			0,
			save_args,
			NULL);
	gimp_register_file_handler_mime (SAVE_PROC, "image/x-vtf");
	gimp_register_save_handler (SAVE_PROC, "vtf", "");
}



static void
run (const gchar *name, gint nparams, const GimpParam *param,
		gint *nreturn_vals, GimpParam **return_vals)
{
	static GimpParam		values[4];
	GimpRunMode			run_mode;
	GimpPDBStatusType	status = GIMP_PDB_SUCCESS;
	GimpExportReturn	export = GIMP_EXPORT_CANCEL;
	GError				*error  = NULL;
	
	run_mode = param[0].data.d_int32;
	
	*nreturn_vals = 1;
	*return_vals  = values;
	values[0].type          = GIMP_PDB_STATUS;
	values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
	
	if (strcmp (name, LOAD_PROC) == 0) {
		switch (run_mode) {
		case GIMP_RUN_INTERACTIVE:
			break;
		
		case GIMP_RUN_NONINTERACTIVE:
			if (nparams != 3)
				status = GIMP_PDB_CALLING_ERROR;
			break;
		
		default:
			break;
		}
		
		if (status == GIMP_PDB_SUCCESS) {
			gint32 image_ID;
			
			image_ID = file_vtf_load_image (param[1].data.d_string, &error);
			if (image_ID != -1) {
				*nreturn_vals = 2;
				values[1].type			= GIMP_PDB_IMAGE;
				values[1].data.d_image	= image_ID;
			} else {
				status = GIMP_PDB_EXECUTION_ERROR;
			}
		}
	
	} else if (strcmp (name, LOAD_THUMB_PROC) == 0) {
		if (nparams < 2) {
			status = GIMP_PDB_CALLING_ERROR;
		} else {
			const gchar *filename = param[0].data.d_string;
			gint width  = param[1].data.d_int32;
			gint height = param[1].data.d_int32;
			gint32 image_ID;
			
			image_ID = file_vtf_load_thumbnail_image (filename,
					&width, &height, &error);
			
			if (image_ID != -1) {
				*nreturn_vals = 4;
				
				values[1].type         = GIMP_PDB_IMAGE;
				values[1].data.d_image = image_ID;
				values[2].type         = GIMP_PDB_INT32;
				values[2].data.d_int32 = width;
				values[3].type         = GIMP_PDB_INT32;
				values[3].data.d_int32 = height;
			} else {
				status = GIMP_PDB_EXECUTION_ERROR;
			}
		}
	
	} else if (strcmp (name, SAVE_PROC) == 0) {
		gchar *file_name;
		gint32 image_ID;
		
		image_ID = param[1].data.d_int32;
		file_name = param[3].data.d_string;
		
		switch (run_mode) {
		case GIMP_RUN_INTERACTIVE:
			break;
		
        case GIMP_RUN_NONINTERACTIVE:
			if (nparams < 5)
				status = GIMP_PDB_CALLING_ERROR;
			break;
		
		case GIMP_RUN_WITH_LAST_VALS:
			break;
		
		default:
			break;
		}
		
		if (status == GIMP_PDB_SUCCESS) {
			status = file_vtf_save_image (file_name, image_ID, run_mode, &error);
		}
		
		if (export == GIMP_EXPORT_EXPORT)
			gimp_image_delete (image_ID);
	
	} else {
		status = GIMP_PDB_CALLING_ERROR;
	}
	
	if (status != GIMP_PDB_SUCCESS && error) {
		*nreturn_vals			= 2;
		values[1].type			= GIMP_PDB_STRING;
		values[1].data.d_string	= error->message;
	}
	
	values[0].data.d_status = status;
}



const GimpPlugInInfo PLUG_IN_INFO = {
	NULL,
	NULL,
	query,
	run,
};


MAIN ()



static gboolean
file_vtf_load_layer (Vtf *vtf, gint32 image, gint32 frame)
{
	void *rgba = vtf_get_image_rgba (vtf, frame);
	if (!rgba)
		return FALSE;
	
	gchar *name = g_strdup_printf ("Frame %d", frame);
	gint32 layer = gimp_layer_new (image, name, vtf_get_width (vtf),
			vtf_get_height (vtf), GIMP_RGBA_IMAGE, 100, GIMP_NORMAL_MODE);
	gimp_image_insert_layer (image, layer, -1, frame);
	g_free (name);
	
	GimpPixelRgn pixel_rgn;
	GimpDrawable *drawable = gimp_drawable_get (layer);
	gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0,
			drawable->width, drawable->height, TRUE, FALSE);
	gimp_pixel_rgn_set_rect (&pixel_rgn, rgba, 0, 0,
				drawable->width, drawable->height);
	gimp_drawable_detach (drawable);
	
	g_free (rgba);
	return TRUE;
}


gint32
file_vtf_load_image (const gchar *fname, GError **error)
{
	gimp_progress_init_printf ("Opening '%s'", gimp_filename_to_utf8 (fname));
	
	Vtf *vtf = vtf_open (fname, error);
	if (!vtf)
		return -1;
	
	gint32 image = gimp_image_new (vtf_get_width (vtf), vtf_get_height (vtf),
			GIMP_RGB);
	gimp_image_set_filename (image, fname);
	
	gint i, frame_count = vtf_get_frame_count (vtf);
	for (i = 0; i < frame_count; i++) {
		if (!file_vtf_load_layer (vtf, image, i)) {
			g_set_error (error, VTF_ERROR, VTF_ERROR_FORMAT,
					"Unsupported format %s", vtf_get_format_name (vtf_get_format (vtf)));
			/* clean up */
			gimp_image_delete (image);
			image = -1;
			break;
		}
	}
	
	vtf_close (vtf);
	gimp_progress_update (1.0);
	return image;
}


gint32
file_vtf_load_thumbnail_image (const gchar *fname, gint *width, gint *height,
		GError **error)
{
	gimp_progress_init_printf ("Opening thumbnail for '%s'",
			gimp_filename_to_utf8 (fname));
	
	Vtf *vtf = vtf_open (fname, error);
	if (!vtf)
		return -1;
	
	*width = vtf_get_width (vtf);
	*height = vtf_get_height (vtf);
	
	gint32 image = gimp_image_new (*width, *height, GIMP_RGB);
	if (!file_vtf_load_layer (vtf, image, 0)) {
		g_set_error (error, VTF_ERROR, VTF_ERROR_FORMAT,
				"Unsupported format %s", vtf_get_format_name (vtf_get_format (vtf)));
		gimp_image_delete (image);
		image = -1;
	}
	
	vtf_close (vtf);
	gimp_progress_update (1.0);
	return image;
}


static gboolean
file_vtf_save_dialog ()
{
	gimp_ui_init ("file-vtf", TRUE);
	GtkWidget *dialog = gimp_export_dialog_new ("Valve Texture",
			"file-vtf", "plug-in-vtf");
	
	GtkWidget *main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
	gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
	gtk_box_pack_start (GTK_BOX (gimp_export_dialog_get_content_area (dialog)),
			main_vbox, TRUE, TRUE, 0);
	gtk_widget_show (main_vbox);
	
	GtkWidget *btn = gtk_button_new_with_label ("hi! :)");
	gtk_box_pack_start (GTK_BOX (main_vbox), btn, FALSE, FALSE, 0);
	
	gtk_widget_show_all (dialog);
	gint resp = gimp_dialog_run (GIMP_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	return (resp == GTK_RESPONSE_OK);
}


GimpPDBStatusType
file_vtf_save_image (const gchar *fname, gint32 image, gint32 run_mode,
		GError **error)
{
	if (run_mode == GIMP_RUN_INTERACTIVE) {
		if (!file_vtf_save_dialog ())
			return GIMP_PDB_CANCEL;
	}
	
	gimp_progress_init_printf ("Saving '%s'", gimp_filename_to_utf8 (fname));
	
	gimp_progress_update (1.0);
	
	return GIMP_PDB_SUCCESS;
}
