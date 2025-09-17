TITLE := wavedemo
BUILD := 3dsx
SOURCES := $(wildcard src/*.c)

# Convert PNG -> T3X (romfs)
GFXDIR := gfx
ROMFSDIR := romfs
TEX3DS := tex3ds

PNGFILES := $(wildcard $(GFXDIR)/*.png)
T3XFILES := $(PNGFILES:$(GFXDIR)/%.png=$(ROMFSDIR)/%.t3x)

# Pattern rule: convert png -> t3x
$(ROMFSDIR)/%.t3x: $(GFXDIR)/%.png
	@mkdir -p $(ROMFSDIR)
	@echo "Converting $< -> $@"
	$(TEX3DS) -f rgba8888 -o $@ $<

# Ensure t3x assets exist before building
prebuild: $(T3XFILES)

all: prebuild
	@$(MAKE) -C . -f Makefile.build all

# This expects a standard devkitPro 3DS Makefile include (3ds_rules).
# Keep Makefile.build minimal to include the usual rules
