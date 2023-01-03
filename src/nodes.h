#ifndef EXPLORER_NODES_H
#define EXPLORER_NODES_H

#include <jks/array.h>

struct node;

typedef void (*node_on_click_t)(struct node *node);

struct node
{
	node_on_click_t on_click;
	struct jks_array childs; /* node_t* */
	char *name;
	struct node *parent;
};

struct node *mpq_dir_node_new(const char *name, struct node *parent);
struct node *mpq_file_node_new(const char *name, struct node *parent);
void node_delete(struct node *node);
bool node_add_child(struct node *node, struct node *child);
void node_get_path(struct node *node, char *str, size_t len);

#endif
