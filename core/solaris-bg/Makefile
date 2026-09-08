CC = gcc
CFLAGS = -Wall -O2 -Iinclude $(shell pkg-config --cflags gtk+-3.0 gtk-layer-shell-0 cairo pango pangocairo json-c)
LDFLAGS = $(shell pkg-config --libs gtk+-3.0 gtk-layer-shell-0 cairo pango pangocairo json-c) -lm

COMMON_SRCS = src/common/telemetry.c src/common/cairo_utils.c src/common/layer_window.c
COMMON_OBJS = $(COMMON_SRCS:.c=.o)

TARGETS = bin/solaris-bg-engine \
          bin/solaris-bg-wifi \
          bin/solaris-bg-bluetooth \
          bin/solaris-bg-dock \
          bin/solaris-bg-activity \
          bin/solaris-bg-flightdeck \
          bin/solaris-bg-conduit

all: $(TARGETS)

bin/solaris-bg-engine: src/engine/main.o $(COMMON_OBJS)
	@mkdir -p bin
	$(CC) -o $@ $^ $(LDFLAGS)

bin/solaris-bg-wifi: src/wifi/main.o $(COMMON_OBJS)
	@mkdir -p bin
	$(CC) -o $@ $^ $(LDFLAGS)

bin/solaris-bg-bluetooth: src/bluetooth/main.o $(COMMON_OBJS)
	@mkdir -p bin
	$(CC) -o $@ $^ $(LDFLAGS)

bin/solaris-bg-dock: src/dock/main.o $(COMMON_OBJS)
	@mkdir -p bin
	$(CC) -o $@ $^ $(LDFLAGS)

bin/solaris-bg-activity: src/activity/main.o $(COMMON_OBJS)
	@mkdir -p bin
	$(CC) -o $@ $^ $(LDFLAGS)

bin/solaris-bg-flightdeck: src/flightdeck/main.o $(COMMON_OBJS)
	@mkdir -p bin
	$(CC) -o $@ $^ $(LDFLAGS)

bin/solaris-bg-conduit: src/conduit/main.o $(COMMON_OBJS)
	@mkdir -p bin
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*/*.o $(TARGETS)

install: all
	mkdir -p $(HOME)/.local/bin
	mkdir -p $(HOME)/.local/share/solaris/style
	cp -f style/solaris.css $(HOME)/.local/share/solaris/style/solaris.css
	cp -f $(TARGETS) $(HOME)/.local/bin/
	cp -f src/cli/solaris-bg $(HOME)/.local/bin/solaris-bg
	chmod +x $(HOME)/.local/bin/solaris-bg*

.PHONY: all clean install
