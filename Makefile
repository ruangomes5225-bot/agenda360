.RECIPEPREFIX := >
DEVKITXENON ?= /usr/local/xenon
CC := $(DEVKITXENON)/bin/xenon-gcc
CFLAGS := -O2 -std=gnu99 -mcpu=cell -mtune=cell -m32 -fno-pic -mpowerpc64 -DXENON \
  -I$(DEVKITXENON)/usr/include
LDFLAGS := -n -Wl,-T,$(DEVKITXENON)/app.lds -L$(DEVKITXENON)/usr/lib \
  -L$(DEVKITXENON)/xenon/lib/32
LIBS := -lxenon -lm

all: agenda360.elf32

agenda360.elf32: main.c
> $(CC) $(CFLAGS) main.c -o agenda360.elf $(LDFLAGS) $(LIBS)
> $(DEVKITXENON)/bin/xenon-objcopy -O elf32-powerpc --adjust-vma 0x80000000 agenda360.elf agenda360.elf32
> $(DEVKITXENON)/bin/xenon-strip agenda360.elf32

clean:
> rm -f agenda360.elf agenda360.elf32
