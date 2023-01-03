#include "displays/display.h"

#include <libwow/m2.h>

#include <inttypes.h>
#include <stdbool.h>

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

enum m2_category
{
	M2_CATEGORY_HEADER = 1,
	M2_CATEGORY_TEXTURE_COMBINERS_COMBOS,
	M2_CATEGORY_TEXTURE_UNIT_LOOKUPS,
	M2_CATEGORY_TEXTURE_TRANSFORMS,
	M2_CATEGORY_TEXTURE_WEIGHTS,
	M2_CATEGORY_SKIN_PROFILES,
	M2_CATEGORY_MATERIALS,
	M2_CATEGORY_SEQUENCES,
	M2_CATEGORY_PARTICLES,
	M2_CATEGORY_TEXTURES,
	M2_CATEGORY_CAMERAS,
	M2_CATEGORY_LIGHTS,
	M2_CATEGORY_BONES,
};

struct m2_display
{
	struct display display;
	wow_m2_file_t *file;
};

static GtkWidget *build_tree(struct m2_display *display, wow_m2_file_t *file);

static void dtr(struct display *ptr)
{
	struct m2_display *display = (struct m2_display*)ptr;
	wow_m2_file_delete(display->file);
}

struct display *m2_display_new(const struct node *node, const char *path, wow_mpq_file_t *mpq_file)
{
	(void)node;
	(void)path;
	wow_m2_file_t *file = wow_m2_file_new(mpq_file);
	if (!file)
	{
		fprintf(stderr, "failed to parse blp file\n");
		return NULL;
	}
	struct m2_display *display = malloc(sizeof(*display));
	if (!display)
	{
		fprintf(stderr, "m2 display allocation failed\n");
		wow_m2_file_delete(file);
		return NULL;
	}
	display->file = file;
	display->display.dtr = dtr;
	GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	GtkWidget *left_paned = gtk_scrolled_window_new(NULL, NULL);
	GtkWidget *right_paned = gtk_scrolled_window_new(NULL, NULL);
	GtkWidget *tree = build_tree(display, file);
	gtk_container_add(GTK_CONTAINER(left_paned), tree);
	gtk_widget_show(left_paned);
	gtk_widget_show(right_paned);
	gtk_paned_pack1(GTK_PANED(paned), left_paned, true, true);
	gtk_paned_pack2(GTK_PANED(paned), right_paned, false, false);
	gtk_paned_set_position(GTK_PANED(paned), 200);
	gtk_widget_show(paned);
	display->display.root = paned;
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

static GtkWidget *build_main_header(struct m2_display *display)
{
	char data[4096];
	snprintf(data, sizeof(data),
		"magic: %08" PRIx32 "\n"
		"version: %" PRIu32 "\n"
		"name: %s\n"
		"flags: 0x%08" PRIx32 "\n",
		display->file->header.magic,
		display->file->header.version,
		display->file->name,
		display->file->header.flags);
	return build_text_width(data);
}

static GtkWidget *build_skin_sections(wow_m2_skin_profile_t *skin_profile)
{
	GtkListStore *store = gtk_list_store_new(11, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	GtkWidget *tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), true);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *column;
	column = gtk_tree_view_column_new_with_attributes("id", renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	gtk_tree_view_column_set_sort_column_id(column, 0);
	column = gtk_tree_view_column_new_with_attributes("level", renderer, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("vertex_start", renderer, "text", 2, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("vertex_count", renderer, "text", 3, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("index_start", renderer, "text", 4, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("index_count", renderer, "text", 5, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("bone_count", renderer, "text", 6, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("bone_combo_index", renderer, "text", 7, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("bone_influences", renderer, "text", 8, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("center_bone_index", renderer, "text", 9, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("sort_radius", renderer, "text", 10, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	for (uint32_t i = 0; i < skin_profile->sections_nb; ++i)
	{
		wow_m2_skin_section_t *section = &skin_profile->sections[i];
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		SET_TREE_VALUE(0, "%" PRIu32, i);
		SET_TREE_VALUE(1, "%" PRIu16, section->level);
		SET_TREE_VALUE(2, "%" PRIu16, section->vertex_start);
		SET_TREE_VALUE(3, "%" PRIu16, section->vertex_count);
		SET_TREE_VALUE(4, "%" PRIu16, section->index_start);
		SET_TREE_VALUE(5, "%" PRIu16, section->index_count);
		SET_TREE_VALUE(6, "%" PRIu16, section->bone_count);
		SET_TREE_VALUE(7, "%" PRIu16, section->bone_combo_index);
		SET_TREE_VALUE(8, "%" PRIu16, section->bone_influences);
		SET_TREE_VALUE(9, "%" PRIu16, section->center_bone_index);
		SET_TREE_VALUE(10, "%f", section->sort_radius);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	gtk_widget_show(tree);
	return tree;
}

static GtkWidget *build_batchs(wow_m2_skin_profile_t *skin_profile)
{
	GtkListStore *store = gtk_list_store_new(14, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	GtkWidget *tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), true);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *column;
	column = gtk_tree_view_column_new_with_attributes("id", renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	gtk_tree_view_column_set_sort_column_id(column, 0);
	column = gtk_tree_view_column_new_with_attributes("flags", renderer, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("priority_plane", renderer, "text", 2, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("shader_id", renderer, "text", 3, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("skin_section_index", renderer, "text", 4, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("geoset_index", renderer, "text", 5, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("color_index", renderer, "text", 6, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("material_index", renderer, "text", 7, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("material_layer", renderer, "text", 8, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("texture_count", renderer, "text", 9, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("texture_combo_index", renderer, "text", 10, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("texture_coord_combo_index", renderer, "text", 11, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("texture_weight_combo_index", renderer, "text", 12, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("texture_transform_combo_index", renderer, "text", 13, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	for (uint32_t i = 0; i < skin_profile->batches_nb; ++i)
	{
		wow_m2_batch_t *batch = &skin_profile->batches[i];
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		SET_TREE_VALUE(0, "%" PRIu32, i);
		SET_TREE_VALUE(1, "%" PRIu8, batch->flags);
		SET_TREE_VALUE(2, "%" PRIu8, batch->priority_plane);
		SET_TREE_VALUE(3, "%" PRIu8, batch->shader_id);
		SET_TREE_VALUE(4, "%" PRIu8, batch->skin_section_index);
		SET_TREE_VALUE(5, "%" PRIu8, batch->geoset_index);
		SET_TREE_VALUE(6, "%" PRIu8, batch->color_index);
		SET_TREE_VALUE(7, "%" PRIu8, batch->material_index);
		SET_TREE_VALUE(8, "%" PRIu8, batch->material_layer);
		SET_TREE_VALUE(9, "%" PRIu8, batch->texture_count);
		SET_TREE_VALUE(10, "%" PRIu8, batch->texture_combo_index);
		SET_TREE_VALUE(11, "%" PRIu8, batch->texture_coord_combo_index);
		SET_TREE_VALUE(12, "%" PRIu8, batch->texture_weight_combo_index);
		SET_TREE_VALUE(13, "%" PRIu8, batch->texture_transform_combo_index);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	gtk_widget_show(tree);
	return tree;
}

static GtkWidget *build_skin_profile(struct m2_display *display, uint32_t val)
{
	uint8_t profile_id = val & 0xff;
	wow_m2_skin_profile_t *skin_profile = &display->file->skin_profiles[profile_id - 1];
	switch ((val >> 8) & 0xf)
	{
		case 1:
			return build_skin_sections(skin_profile);
		case 2:
			return build_batchs(skin_profile);
	}
	return NULL;
}

static GtkWidget *build_materials(struct m2_display *display)
{
	GtkListStore *store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	GtkWidget *tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), true);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *column;
	column = gtk_tree_view_column_new_with_attributes("id", renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	gtk_tree_view_column_set_sort_column_id(column, 0);
	column = gtk_tree_view_column_new_with_attributes("flags", renderer, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("blend_mode", renderer, "text", 2, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	for (uint32_t i = 0; i < display->file->materials_nb; ++i)
	{
		wow_m2_material_t *material = &display->file->materials[i];
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		SET_TREE_VALUE(0, "%" PRIu32, i);
		SET_TREE_VALUE(1, "0x%" PRIx16, material->flags);
		SET_TREE_VALUE(2, "0x%" PRIx16, material->blend_mode);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	gtk_widget_show(tree);
	return tree;
}

static GtkWidget *build_textures(struct m2_display *display)
{
	GtkListStore *store = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	GtkWidget *tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), true);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *column;
	column = gtk_tree_view_column_new_with_attributes("id", renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	gtk_tree_view_column_set_sort_column_id(column, 0);
	column = gtk_tree_view_column_new_with_attributes("type", renderer, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("flags", renderer, "text", 2, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("filename", renderer, "text", 3, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	for (uint32_t i = 0; i < display->file->textures_nb; ++i)
	{
		wow_m2_texture_t *texture = &display->file->textures[i];
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		SET_TREE_VALUE(0, "%" PRIu32, i);
		SET_TREE_VALUE(1, "0x%" PRIx32, texture->type);
		SET_TREE_VALUE(2, "0x%" PRIx32, texture->flags);
		SET_TREE_VALUE(3, "%s", texture->filename);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	gtk_widget_show(tree);
	return tree;
}

static GtkWidget *build_cameras(struct m2_display *display)
{
	GtkListStore *store = gtk_list_store_new(5, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	GtkWidget *tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), true);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *column;
	column = gtk_tree_view_column_new_with_attributes("id", renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	gtk_tree_view_column_set_sort_column_id(column, 0);
	column = gtk_tree_view_column_new_with_attributes("type", renderer, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("fov", renderer, "text", 2, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("far_clip", renderer, "text", 3, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("near_clip", renderer, "text", 4, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	for (uint32_t i = 0; i < display->file->cameras_nb; ++i)
	{
		wow_m2_camera_t *camera = &display->file->cameras[i];
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		SET_TREE_VALUE(0, "%" PRIu32, i);
		SET_TREE_VALUE(1, "%" PRIu32, camera->type);
		SET_TREE_VALUE(2, "%f", camera->fov);
		SET_TREE_VALUE(3, "%f", camera->far_clip);
		SET_TREE_VALUE(4, "%f", camera->near_clip);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	gtk_widget_show(tree);
	return tree;
}

static GtkWidget *build_lights(struct m2_display *display)
{
	GtkListStore *store = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	GtkWidget *tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), true);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *column;
	column = gtk_tree_view_column_new_with_attributes("id", renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	gtk_tree_view_column_set_sort_column_id(column, 0);
	column = gtk_tree_view_column_new_with_attributes("type", renderer, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("bone", renderer, "text", 2, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("position", renderer, "text", 3, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	for (uint32_t i = 0; i < display->file->lights_nb; ++i)
	{
		wow_m2_light_t *light = &display->file->lights[i];
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		SET_TREE_VALUE(0, "%" PRIu32, i);
		SET_TREE_VALUE(1, "%" PRIu16, light->type);
		SET_TREE_VALUE(2, "%" PRIi16, light->bone);
		SET_TREE_VALUE(3, "{%f, %f, %f}", light->position.x, light->position.y, light->position.z);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	gtk_widget_show(tree);
	return tree;
}

static GtkWidget *build_sequences(struct m2_display *display)
{
	GtkListStore *store = gtk_list_store_new(11, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	GtkWidget *tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), true);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *column;
	column = gtk_tree_view_column_new_with_attributes("id", renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	gtk_tree_view_column_set_sort_column_id(column, 0);
	column = gtk_tree_view_column_new_with_attributes("id", renderer, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("variation_index", renderer, "text", 2, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("start", renderer, "text", 3, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("end", renderer, "text", 4, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("movespeed", renderer, "text", 5, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("flags", renderer, "text", 6, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("frequency", renderer, "text", 7, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("blend_time", renderer, "text", 8, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("variation_next", renderer, "text", 9, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("alias_next", renderer, "text", 10, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	for (uint32_t i = 0; i < display->file->sequences_nb; ++i)
	{
		wow_m2_sequence_t *sequence = &display->file->sequences[i];
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		SET_TREE_VALUE(0, "%" PRIu32, i);
		SET_TREE_VALUE(1, "%" PRIu16, sequence->id);
		SET_TREE_VALUE(2, "%" PRIu16, sequence->variation_index);
		SET_TREE_VALUE(3, "%" PRIu32, sequence->start);
		SET_TREE_VALUE(4, "%" PRIu32, sequence->end);
		SET_TREE_VALUE(5, "%f", sequence->movespeed);
		SET_TREE_VALUE(6, "%" PRIu32, sequence->flags);
		SET_TREE_VALUE(7, "%" PRId16, sequence->frequency);
		SET_TREE_VALUE(8, "%" PRIu32, sequence->blend_time);
		SET_TREE_VALUE(9, "%" PRId16, sequence->variation_next);
		SET_TREE_VALUE(10, "%" PRIu16, sequence->alias_next);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	gtk_widget_show(tree);
	return tree;
}

static GtkWidget *build_bones(struct m2_display *display)
{
	GtkListStore *store = gtk_list_store_new(7, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	GtkWidget *tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), true);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *column;
	column = gtk_tree_view_column_new_with_attributes("id", renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	gtk_tree_view_column_set_sort_column_id(column, 0);
	column = gtk_tree_view_column_new_with_attributes("key_bone_id", renderer, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("flags", renderer, "text", 2, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("parent_bone", renderer, "text", 3, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("submesh_id", renderer, "text", 4, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("bone_name_crc", renderer, "text", 5, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("pivot", renderer, "text", 6, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	for (uint32_t i = 0; i < display->file->bones_nb; ++i)
	{
		wow_m2_bone_t *bone = &display->file->bones[i];
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		SET_TREE_VALUE(0, "%" PRIu32, i);
		SET_TREE_VALUE(1, "%" PRId32, bone->key_bone_id);
		SET_TREE_VALUE(2, "0x%" PRIx32, bone->flags);
		SET_TREE_VALUE(3, "%" PRId16, bone->parent_bone);
		SET_TREE_VALUE(4, "%" PRIu16, bone->submesh_id);
		SET_TREE_VALUE(5, "%" PRIu32, bone->bone_name_crc);
		SET_TREE_VALUE(6, "{%f, %f, %f}", bone->pivot.x, bone->pivot.y, bone->pivot.z);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	gtk_widget_show(tree);
	return tree;
}

static GtkWidget *build_particles(struct m2_display *display)
{
	GtkListStore *store = gtk_list_store_new(35, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	GtkWidget *tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), true);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *column;
	column = gtk_tree_view_column_new_with_attributes("id", renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	gtk_tree_view_column_set_sort_column_id(column, 0);
	column = gtk_tree_view_column_new_with_attributes("flags", renderer, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("position", renderer, "text", 2, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("bone", renderer, "text", 3, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("texture", renderer, "text", 4, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("gemetry_model", renderer, "text", 5, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("recursion_model", renderer, "text", 6, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("blending_type", renderer, "text", 7, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("emitter_type", renderer, "text", 8, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("particle_type", renderer, "text", 9, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("head_or_tail", renderer, "text", 10, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("texture_tile_rotation", renderer, "text", 11, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("texture_dimensions_rows", renderer, "text", 12, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("texture_dimensions_columns", renderer, "text", 13, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("mid_point", renderer, "text", 14, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("color_values", renderer, "text", 15, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("scale_values", renderer, "text", 16, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("lifespan_uv_anim", renderer, "text", 17, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("decay_uv_anim", renderer, "text", 18, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("tail_uv_anim", renderer, "text", 19, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("tail_decay_uv_anim", renderer, "text", 20, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("tail_length", renderer, "text", 21, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("twinkle_speed", renderer, "text", 22, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("twinkle_percent", renderer, "text", 23, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("twinkle_scale_min", renderer, "text", 24, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("twinkle_scale_max", renderer, "text", 25, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("burst_multiplier", renderer, "text", 26, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("drag", renderer, "text", 27, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("spin", renderer, "text", 28, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("wind_vector", renderer, "text", 29, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("wind_time", renderer, "text", 30, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("follow_speed1", renderer, "text", 31, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("follow_scale1", renderer, "text", 32, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("follow_speed2", renderer, "text", 33, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	column = gtk_tree_view_column_new_with_attributes("follow_scale2", renderer, "text", 34, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	for (uint32_t i = 0; i < display->file->particles_nb; ++i)
	{
		wow_m2_particle_t *particle = &display->file->particles[i];
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		SET_TREE_VALUE(0, "%" PRIu32, particle->id);
		SET_TREE_VALUE(1, "0x%" PRIx32, particle->flags);
		SET_TREE_VALUE(2, "{%f, %f, %f}", particle->position.x, particle->position.y, particle->position.z);
		SET_TREE_VALUE(3, "%" PRIu16, particle->bone);
		SET_TREE_VALUE(4, "%" PRIu16, particle->texture);
		SET_TREE_VALUE(5, "%s", particle->geometry_model_filename);
		SET_TREE_VALUE(6, "%s", particle->recursion_model_filename);
		SET_TREE_VALUE(7, "%" PRIu16, particle->blending_type);
		SET_TREE_VALUE(8, "%" PRIu16, particle->emitter_type);
		SET_TREE_VALUE(9, "%" PRIu8, particle->particle_type);
		SET_TREE_VALUE(10, "%" PRIu8, particle->head_or_tail);
		SET_TREE_VALUE(11, "%" PRIu16, particle->texture_tile_rotation);
		SET_TREE_VALUE(12, "%" PRIu16, particle->texture_dimensions_rows);
		SET_TREE_VALUE(13, "%" PRIu16, particle->texture_dimensions_columns);
		SET_TREE_VALUE(14, "%f", particle->mid_point);
		SET_TREE_VALUE(15, "{{0x%" PRIx8 ", 0x%" PRIx8 ", 0x%" PRIx8 ", 0x%" PRIx8 "},{0x%" PRIx8 ", 0x%" PRIx8 ", 0x%" PRIx8 ", 0x%" PRIx8 "},{0x%" PRIx8 ", 0x%" PRIx8 ", 0x%" PRIx8 ", 0x%" PRIx8 "}}", particle->color_values[0].x, particle->color_values[0].y, particle->color_values[0].z, particle->color_values[0].w, particle->color_values[1].x, particle->color_values[1].y, particle->color_values[1].z, particle->color_values[1].w, particle->color_values[2].x, particle->color_values[2].y, particle->color_values[2].z, particle->color_values[2].w);
		SET_TREE_VALUE(16, "{%f, %f, %f}", particle->scale_values[0], particle->scale_values[1], particle->scale_values[2]);
		SET_TREE_VALUE(17, "{%" PRIu16 ", %" PRIu16 ", %" PRIu16 "}", particle->lifespan_uv_anim[0], particle->lifespan_uv_anim[1], particle->lifespan_uv_anim[2]);
		SET_TREE_VALUE(18, "{%" PRIu16 ", %" PRIu16 ", %" PRIu16 "}", particle->decay_uv_anim[0], particle->decay_uv_anim[1], particle->decay_uv_anim[2]);
		SET_TREE_VALUE(19, "{%" PRId16 ", %" PRId16 "}", particle->tail_uv_anim[0], particle->tail_uv_anim[1]);
		SET_TREE_VALUE(20, "{%" PRId16 ", %" PRId16 "}", particle->tail_decay_uv_anim[0], particle->tail_decay_uv_anim[1]);
		SET_TREE_VALUE(21, "%f", particle->tail_length);
		SET_TREE_VALUE(22, "%f", particle->twinkle_speed);
		SET_TREE_VALUE(23, "%f", particle->twinkle_percent);
		SET_TREE_VALUE(24, "%f", particle->twinkle_scale_min);
		SET_TREE_VALUE(25, "%f", particle->twinkle_scale_max);
		SET_TREE_VALUE(26, "%f", particle->burst_multiplier);
		SET_TREE_VALUE(27, "%f", particle->drag);
		SET_TREE_VALUE(28, "%f", particle->spin);
		SET_TREE_VALUE(29, "{%f, %f, %f}", particle->wind_vector.x, particle->wind_vector.y, particle->wind_vector.z);
		SET_TREE_VALUE(30, "%f", particle->wind_time);
		SET_TREE_VALUE(31, "%f", particle->follow_speed1);
		SET_TREE_VALUE(32, "%f", particle->follow_scale1);
		SET_TREE_VALUE(33, "%f", particle->follow_speed2);
		SET_TREE_VALUE(34, "%f", particle->follow_scale2);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	gtk_widget_show(tree);
	return tree;
}

static GtkWidget *build_texture_combiners_combos(struct m2_display *display)
{
	GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);
	GtkWidget *tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), true);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *column;
	column = gtk_tree_view_column_new_with_attributes("value", renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	gtk_tree_view_column_set_sort_column_id(column, 0);
	for (size_t i = 0; i < display->file->texture_combiner_combos_nb; ++i)
	{
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		SET_TREE_VALUE(0, "%" PRIu16, display->file->texture_combiner_combos[i]);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	gtk_widget_show(tree);
	return tree;
}

static GtkWidget *build_texture_unit_lookups(struct m2_display *display)
{
	GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);
	GtkWidget *tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), true);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *column;
	column = gtk_tree_view_column_new_with_attributes("value", renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	gtk_tree_view_column_set_sort_column_id(column, 0);
	for (size_t i = 0; i < display->file->texture_unit_lookups_nb; ++i)
	{
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		SET_TREE_VALUE(0, "%" PRIu16, display->file->texture_unit_lookups[i]);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	gtk_widget_show(tree);
	return tree;
}

static void on_gtk_block_row_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *column, gpointer data)
{
	struct m2_display *display = data;
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
	switch (val & 0xFF)
	{
		case M2_CATEGORY_HEADER:
			child = build_main_header(display);
			break;
		case M2_CATEGORY_TEXTURE_COMBINERS_COMBOS:
			child = build_texture_combiners_combos(display);
			break;
		case M2_CATEGORY_TEXTURE_UNIT_LOOKUPS:
			child = build_texture_unit_lookups(display);
			break;
		case M2_CATEGORY_SKIN_PROFILES:
			child = build_skin_profile(display, val >> 8);
			break;
		case M2_CATEGORY_MATERIALS:
			child = build_materials(display);
			break;
		case M2_CATEGORY_SEQUENCES:
			child = build_sequences(display);
			break;
		case M2_CATEGORY_PARTICLES:
			child = build_particles(display);
			break;
		case M2_CATEGORY_TEXTURES:
			child = build_textures(display);
			break;
		case M2_CATEGORY_CAMERAS:
			child = build_cameras(display);
			break;
		case M2_CATEGORY_LIGHTS:
			child = build_lights(display);
			break;
		case M2_CATEGORY_BONES:
			child = build_bones(display);
			break;
	}
	if (child)
		gtk_container_add(GTK_CONTAINER(scrolled), child);
}

GtkWidget *build_tree(struct m2_display *display, wow_m2_file_t *file)
{
	GtkTreeStore *store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_INT);
	GtkWidget *tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), false);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	g_signal_connect(tree, "row-activated", G_CALLBACK(on_gtk_block_row_activated), display);
	gtk_tree_view_set_activate_on_single_click(GTK_TREE_VIEW(tree), true);
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("Blocks", renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	gtk_widget_set_vexpand(tree, true);
	GtkTreeIter iter;
	gtk_tree_store_append(store, &iter, NULL);
	gtk_tree_store_set(store, &iter, 0, "Header", 1, M2_CATEGORY_HEADER, -1);
	gtk_tree_store_append(store, &iter, NULL);
	gtk_tree_store_set(store, &iter, 0, "Texture combiners combos", 1, M2_CATEGORY_TEXTURE_COMBINERS_COMBOS, -1);
	gtk_tree_store_append(store, &iter, NULL);
	gtk_tree_store_set(store, &iter, 0, "Texture unit lookups", 1, M2_CATEGORY_TEXTURE_UNIT_LOOKUPS, -1);
	gtk_tree_store_append(store, &iter, NULL);
	gtk_tree_store_set(store, &iter, 0, "Texture transforms", 1, M2_CATEGORY_TEXTURE_TRANSFORMS, -1);
	for (uint32_t i = 0; i < file->texture_transforms_nb; ++i)
	{
		GtkTreeIter child;
		char name[14];
		snprintf(name, sizeof(name), "%" PRIu32, i + 1);
		gtk_tree_store_append(store, &child, &iter);
		gtk_tree_store_set(store, &child, 0, name, 1, M2_CATEGORY_TEXTURE_TRANSFORMS + 0x100 * (i + 1), -1);
	}
	gtk_tree_store_append(store, &iter, NULL);
	gtk_tree_store_set(store, &iter, 0, "Texture weights", 1, M2_CATEGORY_TEXTURE_WEIGHTS, -1);
	for (uint32_t i = 0; i < file->texture_weights_nb; ++i)
	{
		GtkTreeIter child;
		char name[14];
		snprintf(name, sizeof(name), "%" PRIu32, i + 1);
		gtk_tree_store_append(store, &child, &iter);
		gtk_tree_store_set(store, &child, 0, name, 1, M2_CATEGORY_TEXTURE_WEIGHTS + 0x100 * (i + 1), -1);
	}
	gtk_tree_store_append(store, &iter, NULL);
	gtk_tree_store_set(store, &iter, 0, "Skin profiles", 1, M2_CATEGORY_SKIN_PROFILES, -1);
	for (uint32_t i = 0; i < file->skin_profiles_nb; ++i)
	{
		GtkTreeIter child;
		GtkTreeIter child2;
		char name[14];
		snprintf(name, sizeof(name), "%" PRIu32, i + 1);
		gtk_tree_store_append(store, &child, &iter);
		gtk_tree_store_set(store, &child, 0, name, 1, M2_CATEGORY_SKIN_PROFILES + 0x100 * (i + 1), -1);
		gtk_tree_store_append(store, &child2, &child);
		gtk_tree_store_set(store, &child2, 0, "Sections", 1, M2_CATEGORY_SKIN_PROFILES + 0x100 * (i + 1) + 0x10000, -1);
		gtk_tree_store_append(store, &child2, &child);
		gtk_tree_store_set(store, &child2, 0, "Batches", 1, M2_CATEGORY_SKIN_PROFILES + 0x100 * (i + 1) + 0x20000, -1);
	}
	gtk_tree_store_append(store, &iter, NULL);
	gtk_tree_store_set(store, &iter, 0, "Materials", 1, M2_CATEGORY_MATERIALS, -1);
	gtk_tree_store_append(store, &iter, NULL);
	gtk_tree_store_set(store, &iter, 0, "Sequences", 1, M2_CATEGORY_SEQUENCES, -1);
	gtk_tree_store_append(store, &iter, NULL);
	gtk_tree_store_set(store, &iter, 0, "Particles", 1, M2_CATEGORY_PARTICLES, -1);
	gtk_tree_store_append(store, &iter, NULL);
	gtk_tree_store_set(store, &iter, 0, "Textures", 1, M2_CATEGORY_TEXTURES, -1);
	gtk_tree_store_append(store, &iter, NULL);
	gtk_tree_store_set(store, &iter, 0, "Cameras", 1, M2_CATEGORY_CAMERAS, -1);
	gtk_tree_store_append(store, &iter, NULL);
	gtk_tree_store_set(store, &iter, 0, "Lights", 1, M2_CATEGORY_LIGHTS, -1);
	gtk_tree_store_append(store, &iter, NULL);
	gtk_tree_store_set(store, &iter, 0, "Bones", 1, M2_CATEGORY_BONES, -1);
	gtk_widget_show(tree);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	return tree;
}
