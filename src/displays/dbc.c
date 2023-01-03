#include "displays/display.h"

#include "nodes.h"

#include <libwow/dbc.h>

#include <inttypes.h>
#include <stdbool.h>

struct dbc_display
{
	struct display display;
	wow_dbc_file_t *file;
};

static void dtr(struct display *ptr)
{
	struct dbc_display *display = (struct dbc_display*)ptr;
	wow_dbc_file_delete(display->file);
}

struct display *dbc_display_new(const struct node *node, const char *path, wow_mpq_file_t *mpq_file)
{
	(void)path;
	wow_dbc_file_t *file = wow_dbc_file_new(mpq_file);
	if (!file)
	{
		fprintf(stderr, "failed to parse dbc file\n");
		return NULL;
	}
	struct dbc_display *display = malloc(sizeof(*display));
	if (!display)
	{
		fprintf(stderr, "dbc display allocation failed\n");
		wow_dbc_file_delete(file);
		return NULL;
	}
	display->display.dtr = dtr;
	display->file = file;
	const wow_dbc_def_t *def = NULL;
	struct
	{
		const char *file;
		const wow_dbc_def_t *def;
	} defs[] =
	{
		{"animationdata.dbc"            , wow_dbc_animation_data_def},
		{"areapoi.dbc"                  , wow_dbc_area_poi_def},
		{"areatable.dbc"                , wow_dbc_area_table_def},
		{"auctionhouse.dbc"             , wow_dbc_auction_house_def},
		{"charbaseinfo.dbc"             , wow_dbc_char_base_info_def},
		{"charhairgeosets.dbc"          , wow_dbc_char_hair_geosets_def},
		{"charsections.dbc"             , wow_dbc_char_sections_def},
		{"charstartoutfit.dbc"          , wow_dbc_char_start_outfit_def},
		{"chartitles.dbc"               , wow_dbc_char_titles_def},
		{"characterfacialhairstyles.dbc", wow_dbc_character_facial_hair_styles_def},
		{"chrclasses.dbc"               , wow_dbc_chr_classes_def},
		{"chrraces.dbc"                 , wow_dbc_chr_races_def},
		{"creaturedisplayinfo.dbc"      , wow_dbc_creature_display_info_def},
		{"creaturedisplayinfoextra.dbc" , wow_dbc_creature_display_info_extra_def},
		{"creaturemodeldata.dbc"        , wow_dbc_creature_model_data_def},
		{"gameobjectdisplayinfo.dbc"    , wow_dbc_game_object_display_info_def},
		{"groundeffecttexture.dbc"      , wow_dbc_ground_effect_texture_def},
		{"groundeffectdoodad.dbc"       , wow_dbc_ground_effect_doodad_def},
		{"helmetgeosetvisdata.dbc"      , wow_dbc_helmet_geoset_vis_data_def},
		{"item.dbc"                     , wow_dbc_item_def},
		{"itemclass.dbc"                , wow_dbc_item_class_def},
		{"itemdisplayinfo.dbc"          , wow_dbc_item_display_info_def},
		{"itemset.dbc"                  , wow_dbc_item_set_def},
		{"itemsubclass.dbc"             , wow_dbc_item_sub_class_def},
		{"lightintband.dbc"             , wow_dbc_light_int_band_def},
		{"lightfloatband.dbc"           , wow_dbc_light_float_band_def},
		{"map.dbc"                      , wow_dbc_map_def},
		{"namegen.dbc"                  , wow_dbc_name_gen_def},
		{"soundentries.dbc"             , wow_dbc_sound_entries_def},
		{"spell.dbc"                    , wow_dbc_spell_def},
		{"spellicon.dbc"                , wow_dbc_spell_icon_def},
		{"talent.dbc"                   , wow_dbc_talent_def},
		{"talenttab.dbc"                , wow_dbc_talent_tab_def},
		{"worldmaparea.dbc"             , wow_dbc_world_map_area_def},
		{"worldmapcontinent.dbc"        , wow_dbc_world_map_continent_def},
		{"worldmapoverlay.dbc"          , wow_dbc_world_map_overlay_def},
		{"worldmaptransforms.dbc"       , wow_dbc_world_map_transforms_def},
		{"wowerror_strings.dbc"         , wow_dbc_wow_error_strings_def},
	};
	for (size_t i = 0; i < sizeof(defs) / sizeof(*defs); ++i)
	{
		if (!strcmp(defs[i].file, node->name))
		{
			def = defs[i].def;
			break;
		}
	}
	/* Tree */
	GType types[512];
	size_t types_nb = 0;
	if (def)
	{
		while (def[types_nb].type != WOW_DBC_TYPE_END)
			types[types_nb++] = G_TYPE_STRING;
	}
	else
	{
		for (size_t i = 0; i < file->header.record_size / 4; ++i)
			types[types_nb++] = G_TYPE_STRING;
	}
	GtkListStore *store = gtk_list_store_newv(types_nb, types);
	GtkWidget *tree = gtk_tree_view_new();
	gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(tree), true);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), true);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	if (def)
	{
		for (uint32_t i = 0; def[i].type != WOW_DBC_TYPE_END; ++i)
		{
			renderer = gtk_cell_renderer_text_new();
			char row_name[256];
			size_t len = strlen(def[i].name);
			char *tmp = row_name;;
			for (size_t j = 0; j < len && (unsigned)(tmp - row_name) < sizeof(row_name) - 1; ++j)
			{
				*tmp = def[i].name[j];
				if (*tmp == '_')
					*(++tmp) = '_';
				tmp++;
			}
			*tmp = '\0';
			GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(row_name, renderer, "text", i, NULL);
			gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
			gtk_tree_view_column_set_resizable(column, true);
			gtk_tree_view_column_set_sort_column_id(column, i);
			gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
		}
	}
	else
	{
		for (uint32_t i = 0; i < file->header.record_size / 4; ++i)
		{
			renderer = gtk_cell_renderer_text_new();
			char row_name[64];
			snprintf(row_name, sizeof(row_name), "row %" PRIu32, i);
			GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(row_name, renderer, "text", i, NULL);
			gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
			gtk_tree_view_column_set_resizable(column, true);
			gtk_tree_view_column_set_sort_column_id(column, i);
			gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
		}
	}
	for (size_t i = 0; i < file->header.record_count; ++i)
	{
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		wow_dbc_row_t row = wow_dbc_get_row(file, i);
		GValue value = G_VALUE_INIT;
		g_value_init(&value, G_TYPE_STRING);
		if (def)
		{
			size_t j = 0;
			for (uint32_t idx = 0; def[idx].type != WOW_DBC_TYPE_END; ++idx)
			{
				char str[512];
				switch (def[idx].type)
				{
					case WOW_DBC_TYPE_I8:
						snprintf(str, sizeof(str), "%" PRId8, wow_dbc_get_i8(&row, j));
						j += 1;
						break;
					case WOW_DBC_TYPE_U8:
						snprintf(str, sizeof(str), "%" PRIu8, wow_dbc_get_u8(&row, j));
						j += 1;
						break;
					case WOW_DBC_TYPE_I16:
						snprintf(str, sizeof(str), "%" PRId16, wow_dbc_get_i16(&row, j));
						j += 2;
						break;
					case WOW_DBC_TYPE_U16:
						snprintf(str, sizeof(str), "%" PRIu16, wow_dbc_get_u16(&row, j));
						j += 2;
						break;
					case WOW_DBC_TYPE_I32:
						snprintf(str, sizeof(str), "%" PRId32, wow_dbc_get_i32(&row, j));
						j += 4;
						break;
					case WOW_DBC_TYPE_U32:
						snprintf(str, sizeof(str), "%" PRIu32, wow_dbc_get_u32(&row, j));
						j += 4;
						break;
					case WOW_DBC_TYPE_I64:
						snprintf(str, sizeof(str), "%" PRId64, wow_dbc_get_i64(&row, j));
						j += 8;
						break;
					case WOW_DBC_TYPE_U64:
						snprintf(str, sizeof(str), "%" PRIu64, wow_dbc_get_u64(&row, j));
						j += 8;
						break;
					case WOW_DBC_TYPE_STR:
						snprintf(str, sizeof(str), "%s", wow_dbc_get_str(&row, j));
						j += 4;
						break;
					case WOW_DBC_TYPE_LSTR:
						snprintf(str, sizeof(str), "%s", wow_dbc_get_str(&row, j + 8));
						j += 4 * 17;
						break;
					case WOW_DBC_TYPE_FLT:
						snprintf(str, sizeof(str), "%f", wow_dbc_get_flt(&row, j));
						j += 4;
						break;
					case WOW_DBC_TYPE_END:
						break;
				}
				g_value_set_string(&value, str);
				gtk_list_store_set_value(store, &iter, idx, &value);
			}
		}
		else
		{
			for (uint32_t j = 0; j < file->header.record_size / 4; ++j)
			{
				char str[64];
				snprintf(str, sizeof(str), "%" PRIu32, wow_dbc_get_u32(&row, j * 4));
				g_value_set_string(&value, str);
				gtk_list_store_set_value(store, &iter, j, &value);
			}
		}
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	gtk_widget_show(tree);
	/* Scroll */
	GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_vexpand(scroll, true);
	gtk_widget_set_hexpand(scroll, true);
	gtk_container_add(GTK_CONTAINER(scroll), tree);
	gtk_widget_show(scroll);
	display->display.root = scroll;
	return &display->display;
}
