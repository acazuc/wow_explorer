NAME = explorer

SL_LIBS = libwow jks

LIB_DIR = lib/build

INCLUDES = -I src
INCLUDES+= -I $(LIB_DIR)/include

CFLAGS = -Wall -Wextra -Wshadow -Wunused -pipe -g -O2
CFLAGS+= $(shell pkg-config --cflags gtk+-3.0)

LIBRARY = -L $(LIB_DIR)/lib
LIBRARY+= -lwow
LIBRARY+= -ljks
LIBRARY+= $(shell pkg-config --libs gtk+-3.0)
LIBRARY+= -lz

SRCS_PATH = src

SRCS_NAME = explorer.c \
            tree.c \
            nodes.c \
            utils/bc.c \
            utils/blp.c \
            utils/dx9_shader.c \
            utils/nv_register_shader.c \
            utils/nv_texture_shader.c \
            displays/adt.c \
            displays/blp.c \
            displays/bls.c \
            displays/dbc.c \
            displays/dir.c \
            displays/display.c \
            displays/img.c \
            displays/m2.c \
            displays/ttf.c \
            displays/txt.c \
            displays/wdl.c \
            displays/wdt.c \
            displays/wmo.c \

SRCS = $(addprefix $(SRCS_PATH)/, $(SRCS_NAME))

OBJS_PATH = obj

OBJS_NAME = $(SRCS_NAME:.c=.o)

OBJS = $(addprefix $(OBJS_PATH)/, $(OBJS_NAME))

all: $(NAME)

$(NAME): $(OBJS)
	@echo "LD $(NAME)"
	@$(CXX) $(LDFLAGS) -o $(NAME) $^ $(LIBRARY)

$(OBJS_PATH)/%.o: $(SRCS_PATH)/%.c
	@mkdir -p $(dir $@)
	@echo "CC $<"
	@$(CC) $(CFLAGS) -std=gnu11 $(CPPFLAGS) -o $@ -c $< $(INCLUDES)

clean:
	@rm -f $(OBJS)
	@rm -f $(NAME)

lib:
	@cd lib/sl_lib && SL_LIBS="$(SL_LIBS)" CFLAGS="$(CFLAGS)" sh build.sh -xb -t "linux_64" -m static -o "$(PWD)/$(LIB_DIR)" -j6

.PHONY: clean lib
