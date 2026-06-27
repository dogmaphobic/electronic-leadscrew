C2000_CGT_ROOT ?= /home/gus/ti/c2000-cgt-18.12.1
BUILD ?= release
NEXTION_UI ?= 1
NEXTION_USE_LINA ?= 1

THIS_MAKEFILE := $(lastword $(MAKEFILE_LIST))
CL2000 := $(C2000_CGT_ROOT)/bin/cl2000

BUILD_DIR := build/c2000/$(BUILD)
TARGET := $(BUILD_DIR)/els-f280049c.out
MAP_FILE := $(BUILD_DIR)/els-f280049c.map
LINK_INFO := $(BUILD_DIR)/els-f280049c_linkInfo.xml

APP_CPP_SRCS := \
	ControlPanel.cpp \
	Core.cpp \
	Debug.cpp \
	EEPROM.cpp \
	Encoder.cpp \
	Nextion/NextionPanel.cpp \
	SPIBus.cpp \
	StepperDrive.cpp \
	Tables.cpp \
	UserInterface.cpp \
	main.cpp

DEVICE_C_SRCS := \
	device_support_f28004x/common/source/f28004x_adc.c \
	device_support_f28004x/common/source/f28004x_cputimers.c \
	device_support_f28004x/common/source/f28004x_defaultisr.c \
	device_support_f28004x/common/source/f28004x_dma.c \
	device_support_f28004x/common/source/f28004x_gpio.c \
	device_support_f28004x/common/source/f28004x_piectrl.c \
	device_support_f28004x/common/source/f28004x_pievect.c \
	device_support_f28004x/common/source/f28004x_spi.c \
	device_support_f28004x/common/source/f28004x_sysctrl.c \
	device_support_f28004x/headers/source/f28004x_globalvariabledefs.c

DEVICE_ASM_SRCS := \
	device_support_f28004x/common/source/f28004x_codestartbranch.asm \
	device_support_f28004x/common/source/f28004x_dcsm_z1otp.asm \
	device_support_f28004x/common/source/f28004x_dcsm_z2otp.asm \
	device_support_f28004x/common/source/f28004x_usdelay.asm

SRCS := $(APP_CPP_SRCS) $(DEVICE_C_SRCS) $(DEVICE_ASM_SRCS)

obj = $(BUILD_DIR)/$(subst /,_,$(basename $(1))).obj
OBJS := $(foreach src,$(SRCS),$(call obj,$(src)))

COMMON_FLAGS := \
	-v28 \
	-ml \
	-mt \
	--cla_support=cla1 \
	--float_support=fpu32 \
	--tmu_support=tmu0 \
	--vcu_support=vcu0 \
	--fp_mode=strict \
	--diag_warning=225 \
	--diag_wrap=off \
	--display_error_number \
	--abi=coffabi \
	--include_path=. \
	--include_path=device_support_f28004x/common/include \
	--include_path=device_support_f28004x/headers/include \
	--include_path=$(C2000_CGT_ROOT)/include

ifeq ($(BUILD),release)
BUILD_FLAGS := --define=_INLINE --define=_FLASH --opt_level=3 --opt_for_speed=3
LINKER_CMD := 28004x_generic_flash_lnk.cmd
else ifeq ($(BUILD),debug)
BUILD_FLAGS := -g
LINKER_CMD := 28004x_generic_ram_lnk.cmd
else
$(error BUILD must be release or debug)
endif

COMPILE_FLAGS := $(COMMON_FLAGS) $(BUILD_FLAGS)

ifeq ($(NEXTION_UI),1)
COMPILE_FLAGS += --define=USE_NEXTION_PANEL
COMPILE_FLAGS += --define=NEXTION_USE_LINA=$(NEXTION_USE_LINA)
endif

LINK_FLAGS := \
	$(COMPILE_FLAGS) \
	-z \
	-m$(MAP_FILE) \
	--heap_size=0x100 \
	--stack_size=0x200 \
	--warn_sections \
	-i$(C2000_CGT_ROOT)/lib \
	-i$(C2000_CGT_ROOT)/include \
	--reread_libs \
	--xml_link_info=$(LINK_INFO) \
	--rom_model \
	-o $(TARGET)

.PHONY: all release debug clean

all: $(TARGET)

release:
	$(MAKE) -f $(THIS_MAKEFILE) BUILD=release

debug:
	$(MAKE) -f $(THIS_MAKEFILE) BUILD=debug

$(TARGET): $(OBJS) $(LINKER_CMD) device_support_f28004x/headers/cmd/f28004x_headers_nonbios.cmd
	@mkdir -p $(@D)
	$(CL2000) $(LINK_FLAGS) $(OBJS) $(LINKER_CMD) device_support_f28004x/headers/cmd/f28004x_headers_nonbios.cmd -llibc.a

define compile_template
$(call obj,$(1)): $(1)
	@mkdir -p $$(@D)
	$(CL2000) $(COMPILE_FLAGS) --output_file=$$@ $$<
endef

$(foreach src,$(SRCS),$(eval $(call compile_template,$(src))))

clean:
	rm -rf build/c2000
