PROJECT = midi-ocm
BUILD_DIR = bin

SHARED_DIR = ../common
CFILES = main.c
CFILES += $(notdir $(wildcard $(SHARED_DIR)/*.c))
# CFILES += ssd1306_128x32.c
# CFILES += tools.c

# DEVICE=stm32f411re
# OOCD_TARGET = stm32f4x
DEVICE = stm32f103c8t6
OOCD_TARGET = stm32f1x
OOCD_INTERFACE = stlink

# You shouldn't have to edit anything below here.
VPATH += $(SHARED_DIR)
INCLUDES += $(patsubst %,-I%, . $(SHARED_DIR))
OPENCM3_DIR=../libopencm3

include $(OPENCM3_DIR)/mk/genlink-config.mk
include ../rules.mk
include $(OPENCM3_DIR)/mk/genlink-rules.mk
