LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := modbus

LOCAL_SRC_FILES := \
	../src/modbus-data.c \
	../src/modbus-rtu.c \
	../src/modbus-tcp.c \
	../src/modbus.c \
	
LOCAL_CFLAGS += -DHAVE_BYTESWAP_H=1


include $(BUILD_SHARED_LIBRARY)
