#include "displays/display.h"

#include <libwow/wmo.h>

#include <inttypes.h>
#include <stdbool.h>

#define ADD_TREE_COLUMN(id, name) \
do \
{ \
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(name, renderer, "text", id, NULL); \
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column); \
	gtk_tree_view_column_set_sort_column_id(column, id); \
	gtk_tree_view_column_set_resizable(column, true); \
} while (0)

#define SET_TREE_VALUE(id, fmt, ...) \
do \
{ \
	char tmp[256]; \
	GValue value = G_VALUE_INIT; \
	g_value_init(&value, G_TYPE_STRING); \
	snprintf(tmp, sizeof(tmp), fmt, ##__VA_ARGS__); \
	g_value_set_string(&value, tmp); \
	gtk_list_store_set_value(store, &iter, id, &value); \
} while (0)

#define SET_TREE_VALUE_U64(id, v) \
do \
{ \
	GValue value = G_VALUE_INIT; \
	g_value_init(&value, G_TYPE_UINT64); \
	g_value_set_uint64(&value, v); \
	gtk_list_store_set_value(store, &iter, id, &value); \
} while (0)

#define SET_TREE_VALUE_I64(id, v) \
do \
{ \
	GValue value = G_VALUE_INIT; \
	g_value_init(&value, G_TYPE_INT64); \
	g_value_set_int64(&value, v); \
	gtk_list_store_set_value(store, &iter, id, &value); \
} while (0)

#define SET_TREE_VALUE_FLT(id, v) \
do \
{ \
	GValue value = G_VALUE_INIT; \
	g_value_init(&value, G_TYPE_FLOAT); \
	g_value_set_float(&value, v); \
	gtk_list_store_set_value(store, &iter, id, &value); \
} while (0)

#define SET_TREE_VALUE_STR(id, v) \
do \
{ \
	GValue value = G_VALUE_INIT; \
	g_value_init(&value, G_TYPE_STRING); \
	g_value_set_string(&value, v); \
	gtk_list_store_set_value(store, &iter, id, &value); \
} while (0)

enum wmo_category
{
	WMO_CATEGORY_MVER = 1,
	WMO_CATEGORY_MOHD,
	WMO_CATEGORY_MOTX,
	WMO_CATEGORY_MOMT,
	WMO_CATEGORY_MOGN,
	WMO_CATEGORY_MOGI,
	WMO_CATEGORY_MOSB,
	WMO_CATEGORY_MOPV,
	WMO_CATEGORY_MOPT,
	WMO_CATEGORY_MOPR,
	WMO_CATEGORY_MOVV,
	WMO_CATEGORY_MOVB,
	WMO_CATEGORY_MOLT,
	WMO_CATEGORY_MODS,
	WMO_CATEGORY_MODN,
	WMO_CATEGORY_MODD,
	WMO_CATEGORY_MFOG,
};

struct wmo_display
{
	struct display display;
	wow_wmo_file_t *file;
};

static GtkWidget *build_gtk_paned(struct wmo_display *display);

static void dtr(struct display *ptr)
{
	struct wmo_display *display = (struct wmo_display*)ptr;
	wow_wmo_file_delete(display->file);
}

struct display *wmo_display_new(const struct node *node, const char *path, struct wow_mpq_file *mpq_file)
{
	wow_wmo_file_t *file = wow_wmo_file_new(mpq_file);
	if (!file)
		return wmo_group_display_new(node, path, mpq_file);
	struct wmo_display *display = malloc(sizeof(*display));
	if (!display)
	{
		fprintf(stderr, "wom display allocation failed\n");
		wow_wmo_file_delete(file);
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

static GtkWidget *build_mver(struct wmo_display *display)
{
	char data[32];
	snprintf(data, sizeof(data), "%" PRIu32, display->file->mver.version);
	return build_text_width(data);
}

static GtkWidget *build_mohd(struct wmo_display *display)
{
	struct wow_mohd *mohd = &display->file->mohd;
	char data[4096];
	snprintf(data, sizeof(data),
	         "textures: %" PRIu32 "\n"
	         "groups: %" PRIu32 "\n"
	         "portals: %" PRIu32 "\n"
	         "lights: %" PRIu32 "\n"
	         "models: %" PRIu32 "\n"
	         "doodad defs: %" PRIu32 "\n"
	         "doodad sets: %" PRIu32 "\n"
	         "ambient color: {%" PRIu8 ", %" PRIu8 ", %" PRIu8 ", %" PRIu8 "}\n"
	         "id: %" PRIu32 "\n"
	         "aabb: {%f, %f, %f} - {%f, %f, %f}\n"
	         "flags: %" PRIx16 "\n"
	         "lods: %" PRIu16,
	         mohd->textures_nb,
	         mohd->groups_nb,
	         mohd->portals_nb,
	         mohd->lights_nb,
	         mohd->models_nb,
	         mohd->doodad_defs_nb,
	         mohd->doodad_sets_nb,
	         mohd->ambient.x, mohd->ambient.y, mohd->ambient.z, mohd->ambient.w,
	         mohd->wmo_id,
	         mohd->aabb0.x, mohd->aabb0.y, mohd->aabb0.z, mohd->aabb1.x, mohd->aabb1.y, mohd->aabb1.z,
	         mohd->flags,
	         mohd->num_lod
	         );
	return build_text_width(data);
}

static GtkWidget *build_motx(struct wmo_display *display)
{
	GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	GtkWidget *tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), true);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	ADD_TREE_COLUMN(0, "offset");
	ADD_TREE_COLUMN(1, "texture");
	for (uint32_t i = 0; i < display->file->motx.data_len; ++i)
	{
		if (!display->file->motx.data[i])
			continue;
		char offset[32];
		snprintf(offset, sizeof(offset), "%" PRIu32, i);
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 0, offset, 1, &display->file->motx.data[i], -1);
		i += strlen(&display->file->motx.data[i]);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	gtk_widget_show(tree);
	return tree;
}

static GtkWidget *build_momt(struct wmo_display *display)
{
	GtkListStore *store = gtk_list_store_new(13, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	GtkWidget *tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), true);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	ADD_TREE_COLUMN(0, "id");
	ADD_TREE_COLUMN(1, "flags");
	ADD_TREE_COLUMN(2, "shader");
	ADD_TREE_COLUMN(3, "blend_mode");
	ADD_TREE_COLUMN(4, "texture1");
	ADD_TREE_COLUMN(5, "emissive_color");
	ADD_TREE_COLUMN(6, "sidn_emissive_color");
	ADD_TREE_COLUMN(7, "texture2");
	ADD_TREE_COLUMN(8, "diff_color");
	ADD_TREE_COLUMN(9, "group_type");
	ADD_TREE_COLUMN(10, "texture3");
	ADD_TREE_COLUMN(11, "color2");
	ADD_TREE_COLUMN(12, "flags2");
	for (uint32_t i = 0; i < display->file->momt.data_nb; ++i)
	{
		struct wow_momt_data *momt = &display->file->momt.data[i];
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		SET_TREE_VALUE(0, "%" PRIu32, i);
		SET_TREE_VALUE(1, "%" PRIx32, momt->flags);
		SET_TREE_VALUE(2, "%" PRIu32, momt->shader);
		SET_TREE_VALUE(3, "%" PRIu32, momt->blend_mode);
		SET_TREE_VALUE(4, "%" PRIu32, momt->texture1);
		SET_TREE_VALUE(5, "{%" PRIu8 ", %" PRIu8 ", %" PRIu8 ", %" PRIu8 "}", momt->emissive_color.x, momt->emissive_color.y, momt->emissive_color.z, momt->emissive_color.w);
		SET_TREE_VALUE(6, "{%" PRIu8 ", %" PRIu8 ", %" PRIu8 ", %" PRIu8 "}", momt->sidn_emissive_color.x, momt->sidn_emissive_color.y, momt->sidn_emissive_color.z, momt->sidn_emissive_color.w);
		SET_TREE_VALUE(7, "%" PRIu32, momt->texture2);
		SET_TREE_VALUE(8, "%" PRIu32, momt->diff_color);
		SET_TREE_VALUE(9, "%" PRIu32, momt->group_type);
		SET_TREE_VALUE(10, "%" PRIu32, momt->texture3);
		SET_TREE_VALUE(11, "%" PRIu32, momt->color2);
		SET_TREE_VALUE(12, "%" PRIu32, momt->flags2);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	gtk_widget_show(tree);
	return tree;
}

static GtkWidget *build_mogn(struct wmo_display *display)
{
	GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	GtkWidget *tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), true);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	ADD_TREE_COLUMN(0, "offset");
	ADD_TREE_COLUMN(1, "texture");
	for (uint32_t i = 0; i < display->file->mogn.data_len; ++i)
	{
		if (!display->file->mogn.data[i])
			continue;
		char offset[32];
		snprintf(offset, sizeof(offset), "%" PRIu32, i);
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 0, offset, 1, &display->file->mogn.data[i], -1);
		i += strlen(&display->file->mogn.data[i]);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	gtk_widget_show(tree);
	return tree;
}

static GtkWidget *build_mogi(struct wmo_display *display)
{
	GtkListStore *store = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	GtkWidget *tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), true);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	ADD_TREE_COLUMN(0, "id");
	ADD_TREE_COLUMN(1, "flags");
	ADD_TREE_COLUMN(2, "aabb");
	ADD_TREE_COLUMN(3, "name_offset");
	for (uint32_t i = 0; i < display->file->mogi.data_nb; ++i)
	{
		struct wow_mogi_data *mogi = &display->file->mogi.data[i];
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		SET_TREE_VALUE(0, "%" PRIu32, i);
		SET_TREE_VALUE(1, "%" PRIx32, mogi->flags);
		SET_TREE_VALUE(2, "{%f, %f, %f}, {%f, %f, %f}", mogi->aabb0.x, mogi->aabb0.y, mogi->aabb0.z, mogi->aabb1.x, mogi->aabb1.y, mogi->aabb1.z);
		SET_TREE_VALUE(3, "%" PRId32, mogi->name_offset);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	gtk_widget_show(tree);
	return tree;
}

static GtkWidget *build_mosb(struct wmo_display *display)
{
	GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	GtkWidget *tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), true);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	ADD_TREE_COLUMN(0, "offset");
	ADD_TREE_COLUMN(1, "name");
	for (uint32_t i = 0; i < display->file->mosb.data_len; ++i)
	{
		if (!display->file->mosb.data[i])
			continue;
		char offset[32];
		snprintf(offset, sizeof(offset), "%" PRIu32, i);
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 0, offset, 1, &display->file->mosb.data[i], -1);
		i += strlen(&display->file->mosb.data[i]);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	gtk_widget_show(tree);
	return tree;
}

static GtkWidget *build_mopv(struct wmo_display *display)
{
	GtkListStore *store = gtk_list_store_new(4, G_TYPE_UINT64, G_TYPE_FLOAT, G_TYPE_FLOAT, G_TYPE_FLOAT);
	GtkWidget *tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), true);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	ADD_TREE_COLUMN(0, "id");
	ADD_TREE_COLUMN(1, "x");
	ADD_TREE_COLUMN(2, "y");
	ADD_TREE_COLUMN(3, "z");
	for (uint32_t i = 0; i < display->file->mopv.vertices_nb; ++i)
	{
		struct wow_vec3f *mopv = &display->file->mopv.vertices[i];
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		SET_TREE_VALUE_U64(0, i);
		SET_TREE_VALUE_FLT(1, mopv->x);
		SET_TREE_VALUE_FLT(2, mopv->z);
		SET_TREE_VALUE_FLT(3, -mopv->y);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	gtk_widget_show(tree);
	return tree;
}

static GtkWidget *build_mopt(struct wmo_display *display)
{
	GtkListStore *store = gtk_list_store_new(5, G_TYPE_UINT64, G_TYPE_UINT64, G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_FLOAT);
	GtkWidget *tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), true);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	ADD_TREE_COLUMN(0, "id");
	ADD_TREE_COLUMN(1, "start_vertex");
	ADD_TREE_COLUMN(2, "count");
	ADD_TREE_COLUMN(3, "normal");
	ADD_TREE_COLUMN(4, "distance");
	for (uint32_t i = 0; i < display->file->mopt.data_nb; ++i)
	{
		struct wow_mopt_data *mopt = &display->file->mopt.data[i];
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		SET_TREE_VALUE_U64(0, i);
		SET_TREE_VALUE_U64(1, mopt->start_vertex);
		SET_TREE_VALUE_FLT(2, mopt->count);
		SET_TREE_VALUE(3, "{%f, %f, %f}", mopt->normal.x, mopt->normal.z, -mopt->normal.y);
		SET_TREE_VALUE_FLT(4, mopt->distance);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	gtk_widget_show(tree);
	return tree;
}

static GtkWidget *build_mopr(struct wmo_display *display)
{
	GtkListStore *store = gtk_list_store_new(4, G_TYPE_UINT64, G_TYPE_UINT64, G_TYPE_UINT64, G_TYPE_INT64);
	GtkWidget *tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), true);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	ADD_TREE_COLUMN(0, "id");
	ADD_TREE_COLUMN(1, "portal_index");
	ADD_TREE_COLUMN(2, "group_index");
	ADD_TREE_COLUMN(3, "side");
	for (uint32_t i = 0; i < display->file->mopr.data_nb; ++i)
	{
		struct wow_mopr_data *mopr = &display->file->mopr.data[i];
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		SET_TREE_VALUE_U64(0, i);
		SET_TREE_VALUE_U64(1, mopr->portal_index);
		SET_TREE_VALUE_U64(2, mopr->group_index);
		SET_TREE_VALUE_I64(3, mopr->side);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	gtk_widget_show(tree);
	return tree;
}

static GtkWidget *build_movv(struct wmo_display *display)
{
	GtkListStore *store = gtk_list_store_new(4, G_TYPE_UINT64, G_TYPE_FLOAT, G_TYPE_FLOAT, G_TYPE_FLOAT);
	GtkWidget *tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), true);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	ADD_TREE_COLUMN(0, "id");
	ADD_TREE_COLUMN(1, "x");
	ADD_TREE_COLUMN(2, "y");
	ADD_TREE_COLUMN(3, "z");
	for (uint32_t i = 0; i < display->file->movv.vertices_nb; ++i)
	{
		struct wow_vec3f *movv = &display->file->movv.vertices[i];
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		SET_TREE_VALUE_U64(0, i);
		SET_TREE_VALUE_FLT(1, movv->x);
		SET_TREE_VALUE_FLT(2, movv->z);
		SET_TREE_VALUE_FLT(3, -movv->y);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	gtk_widget_show(tree);
	return tree;
}

static GtkWidget *build_movb(struct wmo_display *display)
{
	GtkListStore *store = gtk_list_store_new(3, G_TYPE_UINT64, G_TYPE_UINT64, G_TYPE_UINT64);
	GtkWidget *tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), true);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	ADD_TREE_COLUMN(0, "id");
	ADD_TREE_COLUMN(1, "first_vertex");
	ADD_TREE_COLUMN(2, "count");
	for (uint32_t i = 0; i < display->file->movb.data_nb; ++i)
	{
		struct wow_movb_data *movb = &display->file->movb.data[i];
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		SET_TREE_VALUE_U64(0, i);
		SET_TREE_VALUE_U64(1, movb->first_vertex);
		SET_TREE_VALUE_U64(1, movb->count);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	gtk_widget_show(tree);
	return tree;
}

static GtkWidget *build_molt(struct wmo_display *display)
{
	GtkListStore *store = gtk_list_store_new(10, G_TYPE_UINT64, G_TYPE_UINT64, G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_FLOAT, G_TYPE_STRING, G_TYPE_FLOAT, G_TYPE_FLOAT);
	GtkWidget *tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), true);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	ADD_TREE_COLUMN(0, "id");
	ADD_TREE_COLUMN(1, "type");
	ADD_TREE_COLUMN(2, "use_atten");
	ADD_TREE_COLUMN(3, "pad");
	ADD_TREE_COLUMN(4, "color");
	ADD_TREE_COLUMN(5, "position");
	ADD_TREE_COLUMN(6, "intensity");
	ADD_TREE_COLUMN(7, "unknown");
	ADD_TREE_COLUMN(8, "atten_start");
	ADD_TREE_COLUMN(9, "atten_end");
	for (uint32_t i = 0; i < display->file->molt.data_nb; ++i)
	{
		struct wow_molt_data *molt = &display->file->molt.data[i];
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		SET_TREE_VALUE_U64(0, i);
		SET_TREE_VALUE_U64(1, molt->type);
		SET_TREE_VALUE_U64(2, molt->use_atten);
		SET_TREE_VALUE(3, "{%" PRIu8 ", %" PRIu8 "}", molt->pad[0], molt->pad[1]);
		SET_TREE_VALUE(4, "{%" PRIu8 ", %" PRIu8 ", %" PRIu8 ", %" PRIu8 "}", molt->color.x, molt->color.y, molt->color.z, molt->color.w);
		SET_TREE_VALUE(5, "{%f, %f, %f}", molt->position.x, molt->position.y, molt->position.z);
		SET_TREE_VALUE_FLT(6, molt->intensity);
		SET_TREE_VALUE(7, "{%f, %f, %f, %f}", molt->unknown[0], molt->unknown[1], molt->unknown[2], molt->unknown[3]);
		SET_TREE_VALUE_FLT(8, molt->atten_start);
		SET_TREE_VALUE_FLT(9, molt->atten_end);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	gtk_widget_show(tree);
	return tree;
}

static GtkWidget *build_mods(struct wmo_display *display)
{
	GtkListStore *store = gtk_list_store_new(5, G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_UINT64, G_TYPE_STRING);
	GtkWidget *tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), true);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	ADD_TREE_COLUMN(0, "id");
	ADD_TREE_COLUMN(1, "name");
	ADD_TREE_COLUMN(2, "start_index");
	ADD_TREE_COLUMN(3, "count");
	ADD_TREE_COLUMN(4, "pad");
	for (uint32_t i = 0; i < display->file->mods.data_nb; ++i)
	{
		struct wow_mods_data *mods = &display->file->mods.data[i];
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		SET_TREE_VALUE_U64(0, i);
		SET_TREE_VALUE_STR(1, mods->name);
		SET_TREE_VALUE_U64(2, mods->start_index);
		SET_TREE_VALUE_U64(3, mods->count);
		SET_TREE_VALUE(4, "{%" PRId8 ", %" PRId8 ", %" PRId8 ", %" PRId8 "}", mods->pad[0], mods->pad[1], mods->pad[2], mods->pad[3]);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	gtk_widget_show(tree);
	return tree;
}

static GtkWidget *build_modn(struct wmo_display *display)
{
	GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	GtkWidget *tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), true);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	ADD_TREE_COLUMN(0, "offset");
	ADD_TREE_COLUMN(1, "name");
	for (uint32_t i = 0; i < display->file->modn.data_len; ++i)
	{
		if (!display->file->modn.data[i])
			continue;
		char offset[32];
		snprintf(offset, sizeof(offset), "%" PRIu32, i);
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 0, offset, 1, &display->file->modn.data[i], -1);
		i += strlen(&display->file->modn.data[i]);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	gtk_widget_show(tree);
	return tree;
}

static GtkWidget *build_modd(struct wmo_display *display)
{
	GtkListStore *store = gtk_list_store_new(11, G_TYPE_UINT64, G_TYPE_UINT64, G_TYPE_UINT64, G_TYPE_UINT64, G_TYPE_UINT64, G_TYPE_UINT64, G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_STRING);
	GtkWidget *tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), true);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	ADD_TREE_COLUMN(0, "id");
	ADD_TREE_COLUMN(1, "name_index");
	ADD_TREE_COLUMN(2, "accept_proj_tex");
	ADD_TREE_COLUMN(3, "flag_0x2");
	ADD_TREE_COLUMN(4, "flag_0x4");
	ADD_TREE_COLUMN(5, "flag_0x8");
	ADD_TREE_COLUMN(6, "padding");
	ADD_TREE_COLUMN(7, "position");
	ADD_TREE_COLUMN(8, "rotation");
	ADD_TREE_COLUMN(9, "scale");
	ADD_TREE_COLUMN(10, "color");
	for (uint32_t i = 0; i < display->file->modd.data_nb; ++i)
	{
		struct wow_modd_data *modd = &display->file->modd.data[i];
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		SET_TREE_VALUE_U64(0, i);
		SET_TREE_VALUE_U64(1, modd->name_index);
		SET_TREE_VALUE_U64(2, modd->flag_accept_proj_tex);
		SET_TREE_VALUE_U64(3, modd->flag_0x2);
		SET_TREE_VALUE_U64(4, modd->flag_0x4);
		SET_TREE_VALUE_U64(5, modd->flag_0x8);
		SET_TREE_VALUE_U64(6, modd->padding);
		SET_TREE_VALUE(7, "{%f, %f, %f}", modd->position.x, modd->position.z, -modd->position.y);
		SET_TREE_VALUE(8, "{%f, %f, %f, %f}", modd->rotation.x, modd->rotation.y, modd->rotation.z, modd->rotation.w);
		SET_TREE_VALUE_FLT(9, modd->scale);
		SET_TREE_VALUE(10, "{%" PRIu8 ", %" PRIu8 ", %" PRIu8 ", %" PRIu8 "}", modd->color.x, modd->color.y, modd->color.z, modd->color.w);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	gtk_widget_show(tree);
	return tree;
}

static GtkWidget *build_mfog(struct wmo_display *display)
{
	GtkListStore *store = gtk_list_store_new(11, G_TYPE_UINT64, G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_FLOAT, G_TYPE_FLOAT, G_TYPE_FLOAT, G_TYPE_FLOAT, G_TYPE_STRING, G_TYPE_FLOAT, G_TYPE_FLOAT, G_TYPE_STRING);
	GtkWidget *tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), true);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	ADD_TREE_COLUMN(0, "id");
	ADD_TREE_COLUMN(1, "flags");
	ADD_TREE_COLUMN(2, "position");
	ADD_TREE_COLUMN(3, "small_radius");
	ADD_TREE_COLUMN(4, "large_radius");
	ADD_TREE_COLUMN(5, "fog_end1");
	ADD_TREE_COLUMN(6, "fog_start1");
	ADD_TREE_COLUMN(7, "color1");
	ADD_TREE_COLUMN(8, "fog_end2");
	ADD_TREE_COLUMN(9, "fog_start2");
	ADD_TREE_COLUMN(10, "color2");
	for (uint32_t i = 0; i < display->file->mfog.data_nb; ++i)
	{
		struct wow_mfog_data *mfog = &display->file->mfog.data[i];
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		SET_TREE_VALUE_U64(0, i);
		SET_TREE_VALUE_U64(1, mfog->flags);
		SET_TREE_VALUE(2, "{%f, %f, %f}", mfog->pos.x, mfog->pos.z, -mfog->pos.y);
		SET_TREE_VALUE_FLT(3, mfog->small_radius);
		SET_TREE_VALUE_FLT(4, mfog->large_radius);
		SET_TREE_VALUE_FLT(5, mfog->fog_end1);
		SET_TREE_VALUE_FLT(6, mfog->fog_start1);
		SET_TREE_VALUE(7, "{%" PRIu8 ", %" PRIu8 ", %" PRIu8 ", %" PRIu8 "}", mfog->color1.x, mfog->color1.y, mfog->color1.z, mfog->color1.w);
		SET_TREE_VALUE_FLT(8, mfog->fog_end2);
		SET_TREE_VALUE_FLT(9, mfog->fog_start2);
		SET_TREE_VALUE(10, "{%" PRIu8 ", %" PRIu8 ", %" PRIu8 ", %" PRIu8 "}", mfog->color2.x, mfog->color2.y, mfog->color2.z, mfog->color2.w);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	gtk_widget_show(tree);
	return tree;
}

static void on_gtk_wmo_row_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *column, gpointer data)
{
	struct wmo_display *display = data;
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
		case WMO_CATEGORY_MVER:
			child = build_mver(display);
			break;
		case WMO_CATEGORY_MOHD:
			child = build_mohd(display);
			break;
		case WMO_CATEGORY_MOTX:
			child = build_motx(display);
			break;
		case WMO_CATEGORY_MOMT:
			child = build_momt(display);
			break;
		case WMO_CATEGORY_MOGN:
			child = build_mogn(display);
			break;
		case WMO_CATEGORY_MOGI:
			child = build_mogi(display);
			break;
		case WMO_CATEGORY_MOSB:
			child = build_mosb(display);
			break;
		case WMO_CATEGORY_MOPV:
			child = build_mopv(display);
			break;
		case WMO_CATEGORY_MOPT:
			child = build_mopt(display);
			break;
		case WMO_CATEGORY_MOPR:
			child = build_mopr(display);
			break;
		case WMO_CATEGORY_MOVV:
			child = build_movv(display);
			break;
		case WMO_CATEGORY_MOVB:
			child = build_movb(display);
			break;
		case WMO_CATEGORY_MOLT:
			child = build_molt(display);
			break;
		case WMO_CATEGORY_MODS:
			child = build_mods(display);
			break;
		case WMO_CATEGORY_MODN:
			child = build_modn(display);
			break;
		case WMO_CATEGORY_MODD:
			child = build_modd(display);
			break;
		case WMO_CATEGORY_MFOG:
			child = build_mfog(display);
			break;
	}
	if (child)
		gtk_container_add(GTK_CONTAINER(scrolled), child);
}

static GtkWidget *build_gtk_tree(struct wmo_display *display)
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
	gtk_tree_store_set(store, &iter, 0, #name, 1, WMO_CATEGORY_##name, -1)
	ADD_TREE_NODE(MVER);
	ADD_TREE_NODE(MOHD);
	ADD_TREE_NODE(MOTX);
	ADD_TREE_NODE(MOMT);
	ADD_TREE_NODE(MOGN);
	ADD_TREE_NODE(MOGI);
	ADD_TREE_NODE(MOSB);
	ADD_TREE_NODE(MOPV);
	ADD_TREE_NODE(MOPT);
	ADD_TREE_NODE(MOPR);
	ADD_TREE_NODE(MOVV);
	ADD_TREE_NODE(MOVB);
	ADD_TREE_NODE(MOLT);
	ADD_TREE_NODE(MODS);
	ADD_TREE_NODE(MODN);
	ADD_TREE_NODE(MODD);
	ADD_TREE_NODE(MFOG);
#undef ADD_TREE_NODE
	gtk_widget_show(tree);
	GtkTreePath *path = gtk_tree_path_new_from_indices(0, -1);
	gtk_tree_selection_select_path(gtk_tree_view_get_selection(GTK_TREE_VIEW(tree)), path);
	gtk_tree_path_free(path);
	return tree;
}

static GtkWidget *build_gtk_paned(struct wmo_display *display)
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
