CC = gcc
CFLAGS = -Wall -g -O2 -Iinclude $(shell pkg-config --cflags gtk-layer-shell-0 gtk+-3.0 cairo json-c gdk-pixbuf-2.0)
LIBS = $(shell pkg-config --libs gtk-layer-shell-0 gtk+-3.0 cairo json-c gdk-pixbuf-2.0) -lm -lpthread

TARGET = bin/solaris-hologram
INSTALL_DIR ?= $(HOME)/.local/bin

SRCS = src/main.c \
       src/common/math3d.c \
       src/ipc/niri_ipc.c \
       src/model/model.c \
       src/render/particle_system.c \
       src/render/render_core.c \
       src/render/render_orbits.c \
       src/render/render_nodes.c \
       src/render/render_hud.c \
       src/ui/command_center.c \
       src/ui/layer_window.c

OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p bin
	$(CC) -o $(TARGET) $(OBJS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o src/*/*.o $(TARGET)

install: $(TARGET)
	mkdir -p $(INSTALL_DIR)
	cp -f $(TARGET) $(INSTALL_DIR)/solaris-hologram
	cp -f $(TARGET) $(INSTALL_DIR)/solaris-bg-hologram
	chmod +x $(INSTALL_DIR)/solaris-hologram $(INSTALL_DIR)/solaris-bg-hologram

.PHONY: all clean install
