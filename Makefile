CC := arm-vita-eabi-gcc
STRIP := arm-vita-eabi-strip
LDFLAGS := -lUVL_stub

all: modump.velf

modump.velf: modump.elf
	$(STRIP) -g $<
	vita-elf-create $< $@ $(JSONDB)

modump.elf: main.o
	$(CC) -Wl,-q -o $@ $^ $(LDFLAGS)
