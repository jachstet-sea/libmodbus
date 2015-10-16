LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := modbus

LOCAL_SRC_FILES := \
	../src/modbus-data.c \
	../src/modbus-rtu.c \
	../src/modbus-tcp.c \
	../src/modbus.c \
	

include $(BUILD_SHARED_LIBRARY)
