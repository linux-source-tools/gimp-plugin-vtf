#include <string.h>
#include <memory>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include "vtf.h"


typedef struct _SaveInfo {
	gint version;
	gint format;
	gint layer;
	gboolean mipmap;
	gboolean lowres;
	gboolean crc;
	
	GtkWidget *ctl_version, *ctl_format, *ctl_layer,
			*ctl_mipmap, *ctl_lowres, *ctl_crc;
} SaveInfo;


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
	static GimpParam	values[4];
	GimpRunMode			run_mode;
	GimpPDBStatusType	status = GIMP_PDB_SUCCESS;
	GimpExportReturn	ret = GIMP_EXPORT_CANCEL;
	GError				*error  = NULL;
	
	run_mode = (GimpRunMode) param[0].data.d_int32;
	
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
		
		if (ret == GIMP_EXPORT_EXPORT)
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
file_vtf_load_layer (Vtf::HiresImageResource *vres, gint32 image, gint16 frame)
{
	uint8_t *rgba = vres->getImageRGBA(0, frame, 0, 0);
	g_return_val_if_fail (rgba != NULL, FALSE);
	
	gchar *name = g_strdup_printf ("Frame %d", frame);
	gint32 layer = gimp_layer_new (image, name, vres->width (), vres->height(),
			GIMP_RGBA_IMAGE, 100, GIMP_NORMAL_MODE);
	gimp_image_insert_layer (image, layer, -1, frame);
	g_free (name);
	
	GimpPixelRgn pixel_rgn;
	GimpDrawable *drawable = gimp_drawable_get (layer);
	gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, drawable->width,
			drawable->height, TRUE, FALSE);
	gimp_pixel_rgn_set_rect (&pixel_rgn, rgba, 0, 0, drawable->width,
			drawable->height);
	gimp_drawable_detach (drawable);
	
	return TRUE;
}


gint32
file_vtf_load_image (const gchar *fname, GError **error)
{
	gimp_progress_init_printf ("Opening '%s'", gimp_filename_to_utf8 (fname));
	
	gint32 image = -1;
	std::auto_ptr<Vtf::File> vtf (new Vtf::File);
	
	try {
		vtf->load(fname);
		
		Vtf::HiresImageResource* vres = (Vtf::HiresImageResource*)
				vtf->findResource (Vtf::Resource::TypeHires);
		if (!vres)
			throw Vtf::Exception ("Cound not find high-resolution image");
		
		image = gimp_image_new (vres->width (), vres->height  (), GIMP_RGB);
		gimp_image_set_filename (image, fname);
		
		guint16 i, frame_count = vres->frameCount ();
		for (i = 0; i < frame_count; i++) {
			if (!file_vtf_load_layer (vres, image, i)) {
				g_set_error (error, 0, 0, "Unsupported format %s",
						Vtf::formatToString (vres->format()));
				gimp_image_delete (image);
				image = -1;
				break;
			}
		}
		
		gimp_progress_update (1.0);
	} catch (std::exception& e) {
		g_set_error (error, 0, 0, e.what ());
	}
	
	return image;
}


gint32
file_vtf_load_thumbnail_image (const gchar *fname, gint *width, gint *height,
		GError **error)
{
	gint32 image = -1;
	gimp_progress_init_printf ("Opening thumbnail for '%s'",
			gimp_filename_to_utf8 (fname));
	
	Vtf::File *vtf = new Vtf::File;
	try {
		vtf->load(fname);
		
		Vtf::HiresImageResource* vres = (Vtf::HiresImageResource*)
				vtf->findResource(Vtf::Resource::TypeHires);
		if (!vres)
			throw Vtf::Exception ("Cound not find high-resolution image");
		
		*width = static_cast<gint> (vres->width ());
		*height = static_cast<gint> (vres->height ());
		
		image = gimp_image_new (*width, *height, GIMP_RGB);
		if (!file_vtf_load_layer (vres, image, 0)) {
			g_set_error (error, 0, 0, "Unsupported format %s",
					Vtf::formatToString(vres->format()));
			gimp_image_delete (image);
			image = -1;
		}
		
		gimp_progress_update (1.0);
	} catch (std::exception& e) {
		g_set_error (error, 0, 0, e.what());
	}
	
	delete vtf;
	return image;
}



static void
save_dialog_version_changed (GtkComboBox *widget, SaveInfo *info)
{
	gint version;
	gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget), &version);
	
	if (version >= 3) {
		g_object_set (G_OBJECT (info->ctl_lowres),
				"sensitive", TRUE,
				"active", info->lowres,
				NULL);
		g_object_set (G_OBJECT (info->ctl_crc),
				"sensitive", TRUE,
				"active", info->crc,
				NULL);
	} else {
		g_object_set (G_OBJECT (info->ctl_lowres),
				"sensitive", FALSE,
				"active", TRUE,
				NULL);
		g_object_set (G_OBJECT (info->ctl_crc),
				"sensitive", FALSE,
				"active", FALSE,
				NULL);
	}
}


static gboolean
file_vtf_save_dialog (SaveInfo *info)
{
	gimp_ui_init ("file-vtf", TRUE);
	GtkWidget *dialog = gimp_export_dialog_new ("Valve Texture",
			"file-vtf", "plug-in-vtf");
	
	GtkWidget *table = gtk_table_new (2, 2, FALSE);
	g_object_set (G_OBJECT (table),
			"border-width", 12,
			"column-spacing", 8,
			"row-spacing", 4,
			NULL);
	
	info->ctl_version = gimp_int_combo_box_new (
			"7.0",	0,
			"7.2",	2,
			"7.3",	3,
			"7.4",	4,
			NULL);
	gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (info->ctl_version), info->version);
	g_signal_connect (info->ctl_version, "changed",
			G_CALLBACK (save_dialog_version_changed), info);
	gtk_table_attach (GTK_TABLE (table), info->ctl_version, 1, 2, 0, 1,
			(GtkAttachOptions) (GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);
	
	GtkWidget *label = gtk_label_new ("_Version:");
	g_object_set (G_OBJECT (label),
			"mnemonic-widget", info->ctl_version,
			"use-underline", TRUE,
			"xalign", 1.0,
			NULL);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
			GTK_FILL, GTK_FILL, 0, 0);
	
	info->ctl_format = gimp_int_combo_box_new (
			Vtf::formatToString (Vtf::FormatRGBA8888),	Vtf::FormatRGBA8888,
			Vtf::formatToString (Vtf::FormatDXT1),		Vtf::FormatDXT1,
			Vtf::formatToString (Vtf::FormatDXT3),		Vtf::FormatDXT3,
			Vtf::formatToString (Vtf::FormatDXT5),		Vtf::FormatDXT5,
			NULL);
	gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (info->ctl_format), info->format);
	gtk_table_attach (GTK_TABLE (table), info->ctl_format, 1, 2, 1, 2,
			(GtkAttachOptions) (GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);
	
	label = gtk_label_new ("_Format:");
	g_object_set (G_OBJECT (label),
			"mnemonic-widget", info->ctl_format,
			"use-underline", TRUE,
			"xalign", 1.0,
			NULL);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
			GTK_FILL, GTK_FILL, 0, 0);
	
	info->ctl_layer = gimp_int_combo_box_new (
			"Frames", 	0,
			"Faces",	1,
			"Z slices",	2,
			NULL);
	gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (info->ctl_layer), 0);
	gtk_table_attach (GTK_TABLE (table), info->ctl_layer, 1, 2, 2, 3,
			(GtkAttachOptions) (GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);
	
	label = gtk_label_new ("_Layers to:");
	g_object_set (G_OBJECT (label),
			"mnemonic-widget", info->ctl_layer,
			"use-underline", TRUE,
			"xalign", 1.0,
			NULL);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
			GTK_FILL, GTK_FILL, 0, 0);
	
	info->ctl_mipmap = gtk_check_button_new_with_label ("Add and generate _mipmaps");
	g_object_set (G_OBJECT (info->ctl_mipmap),
			"use-underline", TRUE,
			"active", TRUE,
			NULL);
	gtk_table_attach (GTK_TABLE (table), info->ctl_mipmap, 0, 2, 3, 4,
			GTK_FILL, GTK_FILL, 0, 0);
	
	info->ctl_lowres = gtk_check_button_new_with_label ("Add low _resolution image");
	g_object_set (G_OBJECT (info->ctl_lowres),
			"use-underline", TRUE,
			"active", TRUE,
			NULL);
	gtk_table_attach (GTK_TABLE (table), info->ctl_lowres, 0, 2, 4, 5,
			GTK_FILL, GTK_FILL, 0, 0);
	
	info->ctl_crc = gtk_check_button_new_with_label ("Add _CRC");
	g_object_set (G_OBJECT (info->ctl_crc),
			"use-underline", TRUE,
			"active", TRUE,
			NULL);
	gtk_table_attach (GTK_TABLE (table), info->ctl_crc, 0, 2, 5, 6,
			GTK_FILL, GTK_FILL, 0, 0);
	
	gtk_box_pack_start (GTK_BOX (gimp_export_dialog_get_content_area (dialog)),
			table, TRUE, TRUE, 0);
	gtk_widget_show_all (table);
	
	gtk_widget_show (dialog);
	gint resp = gimp_dialog_run (GIMP_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	return (resp == GTK_RESPONSE_OK);
}


GimpPDBStatusType
file_vtf_save_image (const gchar *fname, gint32 image, gint32 run_mode,
		GError **error)
{
	SaveInfo info;
	info.version = 4;
	info.format = Vtf::FormatDXT5;
	info.layer = 0;
	info.mipmap = TRUE;
	info.lowres = TRUE;
	info.crc = TRUE;
	
	if (run_mode == GIMP_RUN_INTERACTIVE)
		if (!file_vtf_save_dialog (&info))
			return GIMP_PDB_CANCEL;
	
	gimp_progress_init_printf ("Saving '%s'", gimp_filename_to_utf8 (fname));
	
	std::auto_ptr<Vtf::File> vtf (new Vtf::File);
	
	try {
		Vtf::HiresImageResource* vres = new Vtf::HiresImageResource;
//		vres->setup (info.format, info.width, info.height, info);
		vres->check ();
		
		vtf->addResource (vres);
		vtf->save (fname, info.version);
	} catch (std::exception& e) {
	}
	
	gimp_progress_update (1.0);
	
	return GIMP_PDB_SUCCESS;
}
