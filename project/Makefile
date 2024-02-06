PROJECT = midi-ocm
BUILD_DIR = bin

CFILES = $(wildcard *.c)

OOCD_INTERFACE = stlink
# DEVICE=stm32f411re
# OOCD_TARGET = stm32f4x
DEVICE = stm32f103c8t6
OOCD_TARGET = stm32f1x

# You shouldn't have to edit anything below here.
VPATH += $(SHARED_DIR)
INCLUDES += $(patsubst %,-I%, . $(SHARED_DIR)) 
INCLUDES += -I../u8g2/csrc
OPENCM3_DIR=../libopencm3

include $(OPENCM3_DIR)/mk/genlink-config.mk
include ../rules.mk
include $(OPENCM3_DIR)/mk/genlink-rules.mk
