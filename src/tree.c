#include "explorer.h"
#include "nodes.h"
#include "tree.h"

#include <libwow/mpq.h>

#include <ctype.h>

static void on_gtk_row_expanded(GtkTreeView *treeview, GtkTreeIter *iter, GtkTreePath *path, gpointer data);
static void on_gtk_row_collapsed(GtkTreeView *treeview, GtkTreeIter *iter, GtkTreePath *path, gpointer data);
static void on_gtk_row_activated(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column, gpointer data);
static void on_gtk_row_button_pressed(GtkTreeView *treeview, GdkEventButton *event, gpointer data);
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
	g_signal_connect(tree->treeview, "row-expanded", G_CALLBACK(on_gtk_row_expanded), tree);
	g_signal_connect(tree->treeview, "row-collapsed", G_CALLBACK(on_gtk_row_collapsed), tree);
	g_signal_connect(tree->treeview, "row-activated", G_CALLBACK(on_gtk_row_activated), tree);
	//g_signal_connect(tree->treeview, "button-press-event", G_CALLBACK(on_gtk_row_button_pressed), tree);
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

static void on_gtk_row_expanded(GtkTreeView *treeview, GtkTreeIter *iter, GtkTreePath *path, gpointer data)
{
	/*Tree *tree = (Tree*)data;
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
	}*/
}

static void on_gtk_row_collapsed(GtkTreeView *treeview, GtkTreeIter *iter, GtkTreePath *path, gpointer data)
{
	/*Tree *tree = (Tree*)data;
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
		node->onCollapse();*/
}

static void on_gtk_row_activated(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column, gpointer data)
{
	struct tree *tree = data;
	GtkTreeIter iter;
	gtk_tree_model_get_iter(GTK_TREE_MODEL(tree->store), &iter, path);
	struct node *node;
	gtk_tree_model_get(GTK_TREE_MODEL(tree->store), &iter, 1, &node, -1);
	if (node)
		node->on_click(node);
}

static void on_gtk_row_button_pressed(GtkTreeView *treeview, GdkEventButton *event, gpointer data)
{
	if (event->button == 1)
	{
		GtkTreePath *path;
		GtkTreeViewColumn *column;
		if (!gtk_tree_view_get_path_at_pos(treeview, event->x, event->y, &path, &column, NULL, NULL))
			return;
		gtk_tree_view_row_activated(treeview, path, column);
		return;
	}
	if (event->type != GDK_BUTTON_PRESS)
		return;
	if (event->button != 3)
		return;
	GtkWidget *menu = gtk_menu_new();
	gtk_widget_show(menu);
	gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent*)event);
}

static void add_child(struct tree *tree, GtkTreeIter *parent, struct node *node)
{
	GtkTreeIter iter;
	gtk_tree_store_append(tree->store, &iter, parent);
	gtk_tree_store_set(tree->store, &iter, 0, node->name, 1, node, -1);
	for (size_t i = 0; i < node->childs.size; ++i)
		add_child(tree, &iter, *JKS_ARRAY_GET(&node->childs, i, struct node*));
}
