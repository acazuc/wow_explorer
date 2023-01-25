#include "displays/display.h"

#include <libwow/mpq.h>

struct txt_display
{
	struct display display;
};

struct display *txt_display_new(const struct node *node, const char *path, struct wow_mpq_file *file)
{
	(void)node;
	(void)path;
	struct txt_display *display = malloc(sizeof(*display));
	if (!display)
	{
		fprintf(stderr, "txt display allocation failed\n");
		return NULL;
	}
	display->display.dtr = NULL;
	GtkTextBuffer *buffer = gtk_text_buffer_new(NULL);
	gtk_text_buffer_set_text(buffer, (const char*)file->data, file->size);
	GtkWidget *text = gtk_text_view_new_with_buffer(buffer);
	gtk_text_view_set_monospace(GTK_TEXT_VIEW(text), true);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(text), false);
	gtk_widget_show(text);
	GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolled), text);
	gtk_widget_set_vexpand(scrolled, true);
	gtk_widget_set_hexpand(scrolled, true);
	gtk_widget_show(scrolled);
	display->display.root = scrolled;
	return &display->display;
}
