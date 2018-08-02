include Makefile.param

# target
TARGET := ldcam 

# target source
SRC := ldcam.c

# header
INC := -I.\
    -I/usr/include/gstreamer-1.0/gst \
    -I/usr/include/gstreamer-1.0 \
    -I/usr/include/glib-2.0 \
    -I/usr/lib/arm-linux-gnueabi/glib-2.0/include

# share libraries
LIBS := \
	-lgstreamer-1.0 \
	-lgobject-2.0 \
	-lglib-2.0

CFLAGS += $(INC)

OBJS := $(SRC:%.c=%.o)

.PHONY: all
all: $(OBJS) $(TARGET)

%.o: %.c
	@$(if $(strip $(Q)), echo '  CC    $@')
	$(Q)$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJS)
	@$(if $(strip $(Q)), echo '  LD    $@')
	$(Q)$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $@
	@$(if $(strip $(Q)), echo '  STRIP $@')
	$(Q)$(STRIP) $@

.PHONY: clean
clean:
	-rm -f $(TARGET) $(OBJS)
