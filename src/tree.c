#include "explorer.h"
#include "nodes.h"
#include "tree.h"

#include <libwow/mpq.h>

#include <sys/stat.h>

#include <ctype.h>

#if 0
static void on_gtk_row_expanded(GtkTreeView *treeview, GtkTreeIter *iter, GtkTreePath *path, gpointer data);
static void on_gtk_row_collapsed(GtkTreeView *treeview, GtkTreeIter *iter, GtkTreePath *path, gpointer data);
#endif
static void on_gtk_row_activated(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column, gpointer data);
static gboolean on_gtk_row_button_pressed(GtkTreeView *treeview, GdkEventButton *event, gpointer data);
static void add_child(struct tree *tree, GtkTreeIter *parent, struct node *node);

struct tree *tree_new(struct explorer *explorer)
{
	struct tree *tree = malloc(sizeof(*tree));
	if (!tree)
		return NULL;
	tree->explorer = explorer;
	tree->store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
	tree->treeview = gtk_tree_view_new();
	tree->renderer = gtk_cell_renderer_text_new();
	tree->column = gtk_tree_view_column_new_with_attributes(NULL, tree->renderer, "text", 0, NULL);
#if 0
	g_signal_connect(tree->treeview, "row-expanded", G_CALLBACK(on_gtk_row_expanded), tree);
	g_signal_connect(tree->treeview, "row-collapsed", G_CALLBACK(on_gtk_row_collapsed), tree);
#endif
	g_signal_connect(tree->treeview, "row-activated", G_CALLBACK(on_gtk_row_activated), tree);
	g_signal_connect(tree->treeview, "button-press-event", G_CALLBACK(on_gtk_row_button_pressed), tree);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree->treeview), tree->column);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree->treeview), false);
	for (size_t i = 0; i < tree->explorer->root->childs.size; ++i)
		add_child(tree, NULL, *JKS_ARRAY_GET(&tree->explorer->root->childs, i, struct node*));
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree->treeview), GTK_TREE_MODEL(tree->store));
	gtk_widget_show(tree->treeview);
	return tree;
}

void tree_delete(struct tree *tree)
{
	if (!tree)
		return;
	gtk_widget_destroy(tree->treeview);
	free(tree);
}

#if 0
static void on_gtk_row_expanded(GtkTreeView *treeview, GtkTreeIter *iter, GtkTreePath *path, gpointer data)
{
	Tree *tree = (Tree*)data;
	GtkTreeIter childIter;
	while (gtk_tree_model_iter_children(GTK_TREE_MODEL(tree->store), &childIter, iter))
		gtk_tree_store_remove(GTK_TREE_STORE(tree->store), &childIter);
	Node *node;
	gtk_tree_model_get(GTK_TREE_MODEL(tree->store), iter, 1, &node, -1);
	if (!node)
		return;
	for (Node *child : node->getChilds())
	{
		tree->addChild(iter, child->getName(), &childIter, child);
		if (!child->getChilds().empty())
		{
			GtkTreeIter osef;
			tree->addChild(&childIter, "dummy", &osef, NULL);
		}
	}
}

static void on_gtk_row_collapsed(GtkTreeView *treeview, GtkTreeIter *iter, GtkTreePath *path, gpointer data)
{
	Tree *tree = (Tree*)data;
	Node *node;
	GtkTreeIter childIter;
	bool valid = gtk_tree_model_iter_children(GTK_TREE_MODEL(tree->store), &childIter, iter);
	while (valid)
	{
		GtkTreeIter subIter;
		bool subValid = gtk_tree_model_iter_children(GTK_TREE_MODEL(tree->store), &subIter, &childIter);
		while (subValid)
		{
			gtk_tree_store_remove(GTK_TREE_STORE(tree->store), &subIter);
			subValid = gtk_tree_model_iter_children(GTK_TREE_MODEL(tree->store), &subIter, &childIter);
		}
		gtk_tree_model_get(GTK_TREE_MODEL(tree->store), &childIter, 1, &node, -1);
		if (node)
			node->onHide();
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(tree->store), &childIter);
	}
	gtk_tree_model_get(GTK_TREE_MODEL(tree->store), iter, 1, &node, -1);
	if (node)
		node->onCollapse();
}
#endif

static void on_gtk_row_activated(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column, gpointer data)
{
	(void)treeview;
	(void)column;
	struct tree *tree = data;
	GtkTreeIter iter;
	gtk_tree_model_get_iter(GTK_TREE_MODEL(tree->store), &iter, path);
	struct node *node;
	gtk_tree_model_get(GTK_TREE_MODEL(tree->store), &iter, 1, &node, -1);
	if (node)
		node->on_click(node);
}

static void copy_path(GtkWidget *widget, gpointer data)
{
	(void)widget;
	GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	char path[4096];
	struct node *node = data;
	node_get_path(node, path, sizeof(path));
	gtk_clipboard_set_text(clipboard, path, strlen(path));
}

static void save_mpq_file(struct wow_mpq_file *file, const char *path)
{
	FILE *fp = fopen(path, "wb");
	if (!fp)
	{
		fprintf(stderr, "failed to open '%s': %s\n", path, strerror(errno));
		return;
	}
	if (fwrite(file->data, 1, file->size, fp) != file->size)
		fprintf(stderr, "failed to write data\n");
	fclose(fp);
	printf("saved '%s'\n", path);
}

static void save_node(struct node *node, const char *path)
{
	if (node->childs.size)
	{
		mkdir(path, 0755); /* best effort.. */
		for (size_t i = 0; i < node->childs.size; ++i)
		{
			struct node *child = *JKS_ARRAY_GET(&node->childs, i, struct node*);
			char child_path[4096];
			snprintf(child_path, sizeof(child_path), "%s/%s", path, child->name);
			save_node(child, child_path);
		}
		return;
	}
	char mpq_path[4096];
	node_get_path(node, mpq_path, sizeof(mpq_path));
	normalize_mpq_filename(mpq_path, sizeof(mpq_path));
	struct wow_mpq_file *file = wow_mpq_get_file(g_explorer->mpq_compound, mpq_path);
	if (file)
	{
		save_mpq_file(file, path);
		wow_mpq_file_delete(file);
	}
	else
	{
		fprintf(stderr, "failed to open mpq '%s'", mpq_path);
	}
}

static void node_export(GtkWidget *widget, gpointer data)
{
	(void)widget;
	struct node *node = data;
	GtkWidget *dialog = gtk_file_chooser_dialog_new("destination directory", GTK_WINDOW(g_explorer->window), GTK_FILE_CHOOSER_ACTION_SAVE, "Cancel", GTK_RESPONSE_CANCEL, "Save", GTK_RESPONSE_ACCEPT, NULL);
	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), node->name);
	gtk_file_chooser_set_create_folders(GTK_FILE_CHOOSER(dialog), TRUE);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
	gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), TRUE);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		char *filename;
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
		filename = gtk_file_chooser_get_filename(chooser);
		save_node(node, filename);
		g_free(filename);
	}
	gtk_widget_destroy(dialog);
}

static gboolean on_gtk_row_button_pressed(GtkTreeView *treeview, GdkEventButton *event, gpointer data)
{
	(void)data;
	if (event->type != GDK_BUTTON_PRESS)
		return FALSE;
	if (event->button != 3)
		return FALSE;
	GtkTreeModel *treemodel = gtk_tree_view_get_model(treeview);
	if (!treemodel)
		return FALSE;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	if (gtk_tree_selection_count_selected_rows(selection) > 1)
		return FALSE;
	GtkTreePath *path;
	if (!gtk_tree_view_get_path_at_pos(treeview, event->x, event->y, &path, NULL, NULL, NULL))
		return FALSE;
	gtk_tree_selection_unselect_all(selection);
	gtk_tree_selection_select_path(selection, path);
	GtkTreeIter iter;
	if (!gtk_tree_model_get_iter(treemodel, &iter, path))
	{
		gtk_tree_path_free(path);
		return FALSE;
	}
	struct node *node;
	gtk_tree_model_get(treemodel, &iter, 1, &node, -1);
	GtkWidget *menu = gtk_menu_new();
	GtkWidget *item = gtk_menu_item_new_with_label("copy path");
	g_signal_connect(item, "activate", G_CALLBACK(copy_path), node);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	item = gtk_menu_item_new_with_label("export");
	g_signal_connect(item, "activate", G_CALLBACK(node_export), node);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	gtk_widget_show_all(menu);
	gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent*)event);
	return TRUE;
}

static void add_child(struct tree *tree, GtkTreeIter *parent, struct node *node)
{
	GtkTreeIter iter;
	gtk_tree_store_append(tree->store, &iter, parent);
	gtk_tree_store_set(tree->store, &iter, 0, node->name, 1, node, -1);
	for (size_t i = 0; i < node->childs.size; ++i)
		add_child(tree, &iter, *JKS_ARRAY_GET(&node->childs, i, struct node*));
}
