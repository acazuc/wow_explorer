#include "displays/display.h"

#include <libwow/wmo_group.h>

struct wmo_group_display
{
	wow_wmo_group_file_t *file;
};

struct display *wmo_group_display_new(const struct node *node, const char *path, struct wow_mpq_file *mpq_file)
{
}