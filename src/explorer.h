#ifndef EXPLORER_EXPLORER_H
#define EXPLORER_EXPLORER_H

#include <gtk/gtk.h>

#include <stdint.h>

struct wow_mpq_compound;
struct jks_array;
struct display;
struct tree;
struct node;

struct explorer
{
	GtkWidget *right_paned_scroll;
	GtkWidget *left_paned_scroll;
	GtkWidget *action_bar;
	GtkWidget *menu_bar;
	GtkWidget *window;
	GtkWidget *paned;
	GtkWidget *box;
	struct wow_mpq_compound *mpq_compound;
	struct jks_array *mpq_archives; /* struct wow_mpq_archive* */
	struct display *display;
	struct node *root;
	struct tree *tree;
	uint32_t files_count;
	const char *locale;
	const char *game_path;
};

struct explorer *explorer_new(void);
void explorer_delete(struct explorer *explorer);
int explorer_run(struct explorer *explorer);
void explorer_set_display(struct explorer *explorer, struct display *display);
uint32_t get_color_from_height(float height, float min, float max);
void normalize_mpq_filename(char *filename, size_t size);

extern struct explorer *g_explorer;

#endif
