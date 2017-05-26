LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
#APP_DEBUG := false
#COV_ENABLE := false

LOCAL_SRC_FILES:=                               \
        main.cpp                                \
        src/Configuration.cpp                   \
        src/capture/DisplayManager.cpp          \
        src/capture/ScreenEncoder.cpp           \
        src/capture/ScreenEncoderConsumer.cpp   \
        src/capture/Parser.cpp                  \
        src/capture/Statistics.cpp              \
        src/capture/Rnr.cpp                     \
        src/capture/EvtFrameRecorder.cpp        \
        src/capture/RnRVideoFileReporter.cpp    \
        src/capture/RecordHandler.cpp           \
        src/capture/StreamHandler.cpp           \
        src/capture/FrameIndexFile.cpp          \
        src/capture/VirtualKeys.cpp             \
        src/com/ChannelTcp.cpp                  \
        src/com/Channel.cpp                     \
        src/com/DumpsysWrapper.cpp              \
        src/com/ViewHierarchyParser.cpp         \
        src/com/WindowBarParser.cpp             \
        src/utils/ConvUtils.cpp                 \
        src/utils/MediaCodecUtils.cpp           \
        src/utils/StringUtil.cpp                \
        src/replay/ReplayHandler.cpp            \
        src/replay/Device.cpp                   \
        src/replay/MonkeyService.cpp            \
        src/replay/MonkeyBroker.cpp             \
        src/replay/QAction.cpp                  \
        src/replay/QScript.cpp                  \
        src/replay/TouchAction.cpp              \
        src/replay/ShellAction.cpp              \
        src/replay/InstallerAction.cpp          \
        src/replay/APKStarterAction.cpp         \
        src/replay/TraceReplayer.cpp

LOCAL_SHARED_LIBRARIES := \
	libstagefright liblog libutils libbinder libstagefright_foundation \
	libmedia libgui libcutils libui

LOCAL_C_INCLUDES:= \
	frameworks/av/media/libstagefright             \
	$(TOP)/frameworks/native/include/media/openmax \
	bionic                                         \
	$(LOCAL_PATH)/include

# Get code version
GIT_VERSION := $(shell git --git-dir $(LOCAL_PATH)/.git describe --abbrev=5 --dirty --always --tags)
LOCAL_CFLAGS += -DHSC_VERSION=\"$(GIT_VERSION)\"

LOCAL_CFLAGS += -Wno-multichar

ifeq ($(TARGET_ARCH),x86)
LOCAL_CFLAGS += -DTARGET_ARCH_X86
else ifeq ($(TARGET_ARCH), arm64)
LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON := true
LOCAL_CFLAGS += -DTARGET_ARCH_ARM64
else ifeq ($(TARGET_ARCH), arm)
LOCAL_CFLAGS += -DTARGET_ARCH_ARMV7ABI
LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON := true
LOCAL_CFLAGS += -mfpu=neon
endif

ifeq ($(COV_ENABLE), true)
LOCAL_CFLAGS += -g -O0 -fprofile-arcs -ftest-coverage
LOCAL_LDFLAGS += -lgcov
LOCAL_CFLAGS += -DCOV_ENABLE
endif

ifeq ($(PLATFORM_SDK_VERSION),17)
LOCAL_CFLAGS += -DAOSP17
else ifeq ($(PLATFORM_SDK_VERSION),18)
LOCAL_CFLAGS += -DAOSP18
else ifeq ($(PLATFORM_SDK_VERSION),19)
LOCAL_CFLAGS += -DAOSP19
else ifeq ($(PLATFORM_SDK_VERSION),21)
LOCAL_CFLAGS += -DAOSP21
else ifeq ($(PLATFORM_SDK_VERSION),22)
LOCAL_CFLAGS += -DAOSP22
else ifeq ($(PLATFORM_SDK_VERSION),23)
LOCAL_CFLAGS += -DAOSP23
else
$(warning $(PLATFORM_SDK_VERSION))
endif

ifeq ($(APP_DEBUG), true)
LOCAL_CFLAGS += -DAPP_DEBUG
LOCAL_CFLAGS += -g -O0
endif

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= camera_less

include $(BUILD_EXECUTABLE)
