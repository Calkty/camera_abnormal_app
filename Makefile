CC ?= aarch64-mix210-linux-gcc
.DEFAULT_GOAL := all

# Unspecified switches use src/module_flags.h defaults.
MODULE_NAMES = INFER RING CLIP UPLOAD LEGACY_ALARM
MODULE_DEFS = $(foreach m,$(MODULE_NAMES),$(if $(filter undefined,$(origin ENABLE_$(m))),,-DCA_ENABLE_$(m)=$(ENABLE_$(m))))

PLATFORM ?= H9
PLAT_FLAG = $(PLATFORM)
HUMAN_DIR = vendor/human_detect_demo_v2

-include $(HUMAN_DIR)/src/hikflow/Rules_make_plat

COMMON_CFLAGS ?= -O2 -fPIC -ffunction-sections -fdata-sections -fno-aggressive-loop-optimizations -DDSP -DPLAT_FLAG=\"$(PLAT_FLAG)\"
CORE_CFLAGS ?= -Wall -Wextra $(COMMON_CFLAGS) -I./src
HUMAN_CFLAGS ?= -Wall -Werror -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable $(COMMON_CFLAGS)

CORE_INC = -I./src
HUMAN_INC = \
	-I$(HUMAN_DIR)/src/alarm \
	-I$(HUMAN_DIR)/src/audioplay \
	-I$(HUMAN_DIR)/src/config \
	-I$(HUMAN_DIR)/src/deps \
	-I$(HUMAN_DIR)/src/protocol/include \
	-I$(HUMAN_DIR)/src/hikflow/inc \
	-I$(HUMAN_DIR)/src/hikflow/code \
	-I$(HUMAN_DIR)/src/hikflow/custom_layer \
	-I$(HUMAN_DIR)/src/hikflow/output \
	-I./src \
	-I/heop/include \
	-I/heop/include/hikflow \
	-I/heop/include/bsc \
	-I/heop/include/scheduler \
	-I/heop/include/bll

BASE_LIBS = \
	-lpthread -lrt -lm -lstdc++ -ldl \
	-L/heop/lib/bll -lusr_trans -lnanomsg -lcurl -lssl -lcrypto -lcjson -lopdevsdk -lixml \
	-L/heop/lib/bsc -lbsc \
	-L/heop/lib/scheduler -lscheduler \
	-L/heop/lib/hikflow -lhikflow \
	-L/heop/lib -lhal_bsp \
	$(HUMAN_DIR)/src/hikflow/lib/libisfw.a \
	$(LIBFLGS)

EXTRA_LIBS ?=
LIBS = $(BASE_LIBS) $(EXTRA_LIBS)

TARGET = camera_abnormal_app
SRCS = \
	src/common.c \
	src/main.c \
	src/config.c \
	src/event_queue.c \
	src/ring_buffer.c \
	src/rtp_h26x.c \
	src/rtsp_client.c \
	src/clip_writer.c \
	src/uploader.c \
	src/infer_adapter.c

HUMAN_SRCS = \
	$(HUMAN_DIR)/src/alarm/alarm.c \
	$(HUMAN_DIR)/src/audioplay/audioplay.c \
	$(HUMAN_DIR)/src/config/config.c \
	$(HUMAN_DIR)/src/deps/cJSON.c \
	$(HUMAN_DIR)/src/deps/xmlextend.c \
	$(HUMAN_DIR)/src/hikflow/code/hikflow_demo.c \
	$(HUMAN_DIR)/src/hikflow/code/hikflow_proc_priv.c \
	$(HUMAN_DIR)/src/hikflow/code/json_proc.c \
	$(HUMAN_DIR)/src/hikflow/code/stack_mng_priv.c \
	$(HUMAN_DIR)/src/hikflow/custom_layer/custom_callback.c \
	$(HUMAN_DIR)/src/hikflow/custom_layer/custom_yolov8_custom_0_layer.c \
	$(HUMAN_DIR)/src/hikflow/custom_layer/custom_yolov8_custom_0_sub_0_layer.c \
	$(HUMAN_DIR)/src/hikflow/custom_layer/custom_yolov8_custom_0_sub_1_layer.c \
	$(HUMAN_DIR)/src/hikflow/custom_layer/opc_runtime_arm.c \
	$(HUMAN_DIR)/src/hikflow/custom_layer/yolov8_custom_0_forward.c \
	$(HUMAN_DIR)/src/hikflow/custom_layer/yolov8_custom_0_reshape.c \
	$(HUMAN_DIR)/src/hikflow/custom_layer/yolov8_custom_0_sub_0_forward.c \
	$(HUMAN_DIR)/src/hikflow/custom_layer/yolov8_custom_0_sub_0_reshape.c \
	$(HUMAN_DIR)/src/hikflow/custom_layer/yolov8_custom_0_sub_1_forward.c \
	$(HUMAN_DIR)/src/hikflow/custom_layer/yolov8_custom_0_sub_1_reshape.c \
	$(HUMAN_DIR)/src/protocol/protocol_cb.c \
	$(HUMAN_DIR)/src/protocol/protocol_target_detect_tree.c \
	$(HUMAN_DIR)/src/protocol/protocol_target_detection.c \
	$(HUMAN_DIR)/src/protocol/protocol_target_detection_do.c

OBJS = $(SRCS:.c=.o)
HUMAN_OBJS = $(HUMAN_SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS) $(HUMAN_OBJS)
	$(CC) $(COMMON_CFLAGS) -Wl,-Bsymbolic -o $@ $(OBJS) $(HUMAN_OBJS) $(LIBS)

# Rebuild when command-line module flags change, including reverting to defaults.
.PHONY: FORCE all clean install
FORCE:
.module-flags: FORCE
	@printf '%s\n' '$(MODULE_DEFS)' > .module-flags.tmp
	@cmp -s .module-flags.tmp $@ || cp .module-flags.tmp $@
	@rm -f .module-flags.tmp

$(OBJS) $(HUMAN_OBJS): src/module_flags.h .module-flags

src/%.o: src/%.c
	$(CC) $(MODULE_DEFS) $(CORE_CFLAGS) $(CORE_INC) -c $< -o $@

$(HUMAN_DIR)/src/%.o: $(HUMAN_DIR)/src/%.c
	$(CC) $(MODULE_DEFS) $(HUMAN_CFLAGS) $(HUMAN_INC) -c $< -o $@

install: $(TARGET)
	test -f /heop/lib/bll/libusr_trans.so
	cp -L /heop/lib/bll/libusr_trans.so APP/libusr_trans.so
	@for lib in /heop/lib/bll/libusr_trans.so.*; do \
		[ ! -f "$$lib" ] || cp -L "$$lib" APP/; \
	done
	cp $(TARGET) APP/
	cp app.conf APP/
	cp $(HUMAN_DIR)/src/hikflow/bin/* APP/
	cp $(HUMAN_DIR)/src/hikflow/debug/* APP/
	cp -r $(HUMAN_DIR)/src/hikflow/data APP/
	cp $(HUMAN_DIR)/src/hikflow/config/hikflow_attr.json APP/
	cp $(HUMAN_DIR)/src/hikflow/config/hikflow_config.json APP/
	cp $(HUMAN_DIR)/APP/test.wav APP/
	cp $(HUMAN_DIR)/APP/test.jpg APP/
	chmod +x APP/cameraAbnormal.sh

clean:
	rm -f $(TARGET) $(OBJS) $(HUMAN_OBJS)
	rm -f .module-flags .module-flags.tmp
	rm -f APP/cameraAbnormal_*.app
