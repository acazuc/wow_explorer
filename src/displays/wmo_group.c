#include "displays/table_macro.h"
#include "displays/display.h"

#include <libwow/wmo_group.h>
#include <libwow/mpq.h>

#include <inttypes.h>
#include <stdbool.h>

enum wmo_group_category
{
	WMO_GROUP_CATEGORY_MVER = 1,
	WMO_GROUP_CATEGORY_MOGP,
	WMO_GROUP_CATEGORY_MOPY,
	WMO_GROUP_CATEGORY_MOVI,
	WMO_GROUP_CATEGORY_MOVT,
	WMO_GROUP_CATEGORY_MONR,
	WMO_GROUP_CATEGORY_MOTV,
	WMO_GROUP_CATEGORY_MOBA,
	WMO_GROUP_CATEGORY_MOLR,
	WMO_GROUP_CATEGORY_MODR,
	WMO_GROUP_CATEGORY_MOBN,
	WMO_GROUP_CATEGORY_MOBR,
	WMO_GROUP_CATEGORY_MOCV,
	WMO_GROUP_CATEGORY_MLIQ,
};

struct wmo_group_display
{
	struct display display;
	struct wow_wmo_group_file *file;
};

static GtkWidget *build_gtk_paned(struct wmo_group_display *display);

static void dtr(struct display *ptr)
{
	struct wmo_group_display *display = (struct wmo_group_display*)ptr;
	wow_wmo_group_file_delete(display->file);
}

struct display *wmo_group_display_new(const struct node *node, const char *path, struct wow_mpq_file *mpq_file)
{
	mpq_file->pos = 0; /* XXX remove this hack */
	struct wow_wmo_group_file *file = wow_wmo_group_file_new(mpq_file);
	if (!file)
	{
		fprintf(stderr, "failed to open wmo file\n");
		return NULL;
	}
	struct wmo_group_display *display = malloc(sizeof(*display));
	if (!display)
	{
		fprintf(stderr, "wmo group display allocation failed\n");
		wow_wmo_group_file_delete(file);
		return NULL;
	}
	display->display.dtr = dtr;
	display->file = file;
	display->display.root = build_gtk_paned(display);
	return &display->display;
}

static GtkWidget *build_text_width(char *str)
{
	GtkTextBuffer *buffer = gtk_text_buffer_new(NULL);
	gtk_text_buffer_set_text(buffer, str, strlen(str));
	GtkWidget *text = gtk_text_view_new_with_buffer(buffer);
	gtk_text_view_set_monospace(GTK_TEXT_VIEW(text), true);
	gtk_widget_show(text);
	return text;
}

static GtkWidget *build_mver(struct wmo_group_display *display)
{
	char data[32];
	snprintf(data, sizeof(data), "%" PRIu32, display->file->mver.version);
	return build_text_width(data);
}

static GtkWidget *build_mogp(struct wmo_group_display *display)
{
	const struct wow_mogp *mogp = &display->file->mogp;
	char data[4096];
	snprintf(data, sizeof(data),
	         "group_name: %" PRIu32 "\n"
	         "descriptive_group_name: %" PRIu32 "\n"
	         "flags: %" PRIx32 "\n"
	         "aabb: {{%f, %f, %f}, {%f, %f, %f}}\n"
	         "portal_start: %" PRIu16 "\n"
	         "portal_count: %" PRIu16 "\n"
	         "trans_batch_count: %" PRIu16 "\n"
	         "ext_batch_count: %" PRIu16 "\n"
	         "int_batch_count: %" PRIu16 "\n"
	         "padding_or_batch_type: %" PRIu16 "\n"
	         "fog_ids: {%" PRIu8 ", %" PRIu8 ", %" PRIu8 ", %" PRIu8 "}\n"
	         "group_liquid: %" PRIu32 "\n"
	         "group_id: %" PRIu32 "\n"
	         "flags2: %" PRIx32 "\n"
	         "unk1: %" PRIu32,
	         mogp->group_name,
	         mogp->descriptive_group_name,
	         mogp->flags,
	         mogp->aabb0.x, mogp->aabb0.z, -mogp->aabb0.y, mogp->aabb1.x, mogp->aabb1.z, -mogp->aabb1.y,
	         mogp->portal_start,
	         mogp->portal_count,
	         mogp->trans_batch_count,
	         mogp->ext_batch_count,
	         mogp->int_batch_count,
	         mogp->padding_or_batch_type_d,
	         mogp->fog_ids[0], mogp->fog_ids[1], mogp->fog_ids[2], mogp->fog_ids[3],
	         mogp->group_liquid,
	         mogp->group_id,
	         mogp->flags2,
	         mogp->unk1);
	return build_text_width(data);
}

static GtkWidget *build_mopy(struct wmo_group_display *display)
{
	GtkListStore *store = gtk_list_store_new(3, G_TYPE_UINT64, G_TYPE_UINT64, G_TYPE_UINT64);
	GtkWidget *tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), true);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	ADD_TREE_COLUMN(0, "id");
	ADD_TREE_COLUMN(1, "flags");
	ADD_TREE_COLUMN(2, "material_id");
	for (uint32_t i = 0; i < display->file->mopy.data_nb; ++i)
	{
		const struct wow_mopy_data *mopy = &display->file->mopy.data[i];
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		SET_TREE_VALUE_U64(0, i);
		SET_TREE_VALUE_U64(1, mopy->flags);
		SET_TREE_VALUE_U64(2, mopy->material_id);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	gtk_widget_show(tree);
	return tree;
}

static GtkWidget *build_movi(struct wmo_group_display *display)
{
	GtkListStore *store = gtk_list_store_new(2, G_TYPE_UINT64, G_TYPE_UINT64);
	GtkWidget *tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), true);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	ADD_TREE_COLUMN(0, "id");
	ADD_TREE_COLUMN(1, "indice");
	for (uint32_t i = 0; i < display->file->movi.indices_nb; ++i)
	{
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		SET_TREE_VALUE_U64(0, i);
		SET_TREE_VALUE_U64(1, display->file->movi.indices[i]);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	gtk_widget_show(tree);
	return tree;
}

static GtkWidget *build_movt(struct wmo_group_display *display)
{
	GtkListStore *store = gtk_list_store_new(4, G_TYPE_UINT64, G_TYPE_FLOAT, G_TYPE_FLOAT, G_TYPE_FLOAT);
	GtkWidget *tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), true);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	ADD_TREE_COLUMN(0, "id");
	ADD_TREE_COLUMN(1, "x");
	ADD_TREE_COLUMN(2, "y");
	ADD_TREE_COLUMN(3, "z");
	for (uint32_t i = 0; i < display->file->movt.vertexes_nb; ++i)
	{
		const struct wow_vec3f *movt = &display->file->movt.vertexes[i];
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		SET_TREE_VALUE_U64(0, i);
		SET_TREE_VALUE_FLT(1, movt->x);
		SET_TREE_VALUE_FLT(2, movt->z);
		SET_TREE_VALUE_FLT(3, -movt->y);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	gtk_widget_show(tree);
	return tree;
}

static GtkWidget *build_monr(struct wmo_group_display *display)
{
	GtkListStore *store = gtk_list_store_new(4, G_TYPE_UINT64, G_TYPE_FLOAT, G_TYPE_FLOAT, G_TYPE_FLOAT);
	GtkWidget *tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), true);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	ADD_TREE_COLUMN(0, "id");
	ADD_TREE_COLUMN(1, "x");
	ADD_TREE_COLUMN(2, "y");
	ADD_TREE_COLUMN(3, "z");
	for (uint32_t i = 0; i < display->file->monr.normals_nb; ++i)
	{
		const struct wow_vec3f *monr = &display->file->monr.normals[i];
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		SET_TREE_VALUE_U64(0, i);
		SET_TREE_VALUE_FLT(1, monr->x);
		SET_TREE_VALUE_FLT(2, monr->z);
		SET_TREE_VALUE_FLT(3, -monr->y);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	gtk_widget_show(tree);
	return tree;
}

static void on_gtk_wmo_row_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *column, gpointer data)
{
	struct wmo_group_display *display = data;
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model(tree);
	gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path);
	uint32_t val;
	gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, 1, &val, -1);
	if (!val)
		return;
	GtkWidget *scrolled = gtk_paned_get_child2(GTK_PANED(display->display.root));
	if (!scrolled)
		return;
	GtkWidget *child = gtk_bin_get_child(GTK_BIN(scrolled));
	if (child)
		gtk_widget_destroy(child);
	child = NULL;
	switch (val)
	{
		case WMO_GROUP_CATEGORY_MVER:
			child = build_mver(display);
			break;
		case WMO_GROUP_CATEGORY_MOGP:
			child = build_mogp(display);
			break;
		case WMO_GROUP_CATEGORY_MOPY:
			child = build_mopy(display);
			break;
		case WMO_GROUP_CATEGORY_MOVI:
			child = build_movi(display);
			break;
		case WMO_GROUP_CATEGORY_MOVT:
			child = build_movt(display);
			break;
		case WMO_GROUP_CATEGORY_MONR:
			child = build_monr(display);
			break;
	}
	if (child)
		gtk_container_add(GTK_CONTAINER(scrolled), child);
}

static GtkWidget *build_gtk_tree(struct wmo_group_display *display)
{
	GtkTreeStore *store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_INT);
	GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), true);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("Chunks", renderer, "text", 0, NULL);
	g_signal_connect(tree, "row-activated", G_CALLBACK(on_gtk_wmo_row_activated), display);
	gtk_tree_view_set_activate_on_single_click(GTK_TREE_VIEW(tree), true);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	gtk_widget_set_vexpand(tree, true);
	GtkTreeIter iter;
#define ADD_TREE_NODE(name) \
	gtk_tree_store_append(store, &iter, NULL); \
	gtk_tree_store_set(store, &iter, 0, #name, 1, WMO_GROUP_CATEGORY_##name, -1)
	ADD_TREE_NODE(MVER);
	ADD_TREE_NODE(MOGP);
	ADD_TREE_NODE(MOPY);
	ADD_TREE_NODE(MOVI);
	ADD_TREE_NODE(MOVT);
	ADD_TREE_NODE(MONR);
	ADD_TREE_NODE(MOTV);
	ADD_TREE_NODE(MOBA);
	ADD_TREE_NODE(MOLR);
	ADD_TREE_NODE(MODR);
	ADD_TREE_NODE(MOBN);
	ADD_TREE_NODE(MOBR);
	ADD_TREE_NODE(MOCV);
	ADD_TREE_NODE(MLIQ);
#undef ADD_TREE_NODE
	gtk_widget_show(tree);
	GtkTreePath *path = gtk_tree_path_new_from_indices(0, -1);
	gtk_tree_selection_select_path(gtk_tree_view_get_selection(GTK_TREE_VIEW(tree)), path);
	gtk_tree_path_free(path);
	return tree;
}

static GtkWidget *build_gtk_paned(struct wmo_group_display *display)
{
	GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	GtkWidget *left_paned = gtk_scrolled_window_new(NULL, NULL);
	GtkWidget *right_paned = gtk_scrolled_window_new(NULL, NULL);
	GtkWidget *tree = build_gtk_tree(display);
	gtk_container_add(GTK_CONTAINER(left_paned), tree);
	gtk_widget_show(left_paned);
	gtk_widget_show(right_paned);
	gtk_paned_pack1(GTK_PANED(paned), left_paned, false, false);
	gtk_paned_pack2(GTK_PANED(paned), right_paned, true, true);
	gtk_paned_set_position(GTK_PANED(paned), 200);
	gtk_widget_show(paned);
	return paned;
}
