#include "vtf.h"

int
main (int argc, char **argv)
{
	GError *error = NULL;
	Vtf *vtf = vtf_open (argv[1], &error);
	if (!vtf) {
		g_print ("Error: %s\n", error->message);
		g_error_free (error);
		return -1;
	}
	
	void *data = vtf_get_image_rgba (vtf, 0);
	if (!data) {
		g_print ("Format %s is not supported\n", vtf_get_format_name (vtf_get_format (vtf)));
		g_error_free (error);
		vtf_close (vtf);
		return -1;
	}
	g_free (data);
	
	vtf_close (vtf);
	
	return 0;
}
