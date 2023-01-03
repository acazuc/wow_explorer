#include "displays/display.h"

#include "explorer.h"

#include <libwow/wdl.h>

#include <stdbool.h>
#include <stdint.h>

struct wdl_display
{
	struct display display;
};

static void dummy_free(unsigned char *ptr, void *osef)
{
	(void)osef;
	free(ptr);
}

static void dtr(struct display *ptr)
{
	(void)ptr;
}

struct display *wdl_display_new(const struct node *node, const char *path, wow_mpq_file_t *mpq_file)
{
	(void)node;
	(void)path;
	wow_wdl_file_t *file = wow_wdl_file_new(mpq_file);
	if (!file)
	{
		fprintf(stderr, "failed to parse wdl file\n");
		return NULL;
	}
	struct wdl_display *display = malloc(sizeof(*display));
	if (!display)
	{
		fprintf(stderr, "wdl display allocation failed\n");
		wow_wdl_file_delete(file);
		return NULL;
	}
	display->display.dtr = dtr;
	size_t width = 17 * 64;
	size_t height = 17 * 64;
	uint8_t *data = malloc(width * height * 3);
	int16_t min = SHRT_MAX;
	int16_t max = SHRT_MIN;
	for (size_t y = 0; y < height; ++y)
	{
		for (size_t x = 0; x < width; ++x)
		{
			int16_t pos = file->mare[y / 17][x / 17].data[(y % 17) * 17 + x % 17];
			if (pos < min)
				min = pos;
			if (pos > max)
				max = pos;
		}
	}
	for (size_t y = 0; y < height; ++y)
	{
		for (size_t x = 0; x < width; ++x)
		{
			int16_t pos = file->mare[y / 17][x / 17].data[(y % 17) * 17 + x % 17];
			size_t i = (y * width + x) * 3;
			uint32_t color = get_color_from_height(pos, min, max);
			data[i + 0] = color >> 16;
			data[i + 1] = color >> 8;
			data[i + 2] = color >> 0;
		}
	}
	GtkWidget *image = gtk_image_new_from_pixbuf(gdk_pixbuf_new_from_data(data, GDK_COLORSPACE_RGB, false, 8, width, height, width * 3, dummy_free, NULL));
	gtk_widget_show(image);
	GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolled), image);
	gtk_widget_set_vexpand(scrolled, true);
	gtk_widget_set_hexpand(scrolled, true);
	gtk_widget_show(scrolled);
	display->display.root = scrolled;
	wow_wdl_file_delete(file);
	return &display->display;
}
