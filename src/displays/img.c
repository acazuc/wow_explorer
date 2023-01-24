#include "displays/display.h"

#include <libwow/mpq.h>

struct img_display
{
	struct display display;
};

struct display *img_display_new(const struct node *node, const char *path, struct wow_mpq_file *file)
{
	(void)node;
	struct img_display *display = malloc(sizeof(*display));
	if (!display)
	{
		fprintf(stderr, "img display allocation failed\n");
		return NULL;
	}
	display->display.dtr = NULL;
	const char *ext = strrchr(path, '.');
	if (!strcmp(ext, ".jpg"))
		ext = "jpeg";
	else
		ext++;
	GdkPixbufLoader *loader = gdk_pixbuf_loader_new_with_type(ext, NULL);
	if (!loader)
	{
		fprintf(stderr, "failed to create gdk loader with ext %s\n", ext);
		free(display);
		return NULL;
	}
	if (!gdk_pixbuf_loader_write(loader, file->data, file->size, NULL))
	{
		fprintf(stderr, "failed to write data to loader\n");
		/* XXX free loader */
		free(display);
		return NULL;
	}
	GdkPixbuf *pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
	if (!pixbuf)
	{
		fprintf(stderr, "failed to get pixbuf from loader\n");
		/* XXX free loader */
		free(display);
		return NULL;
	}
	GtkWidget *image = gtk_image_new_from_pixbuf(pixbuf);
	gtk_widget_show(image);
	GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolled), image);
	gtk_widget_set_vexpand(scrolled, true);
	gtk_widget_set_hexpand(scrolled, true);
	gtk_widget_show(scrolled);
	display->display.root = scrolled;
	return &display->display;
}
