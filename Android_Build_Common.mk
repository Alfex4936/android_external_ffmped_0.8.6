
LOCAL_MODULE := $(FF_LIBRARY_NAME)
LOCAL_ARM_MODE := arm
LOCAL_LDFLAGS += -lz -lm -llog
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH) $(LOCAL_PATH)/$(FF_LIBRARY_NAME)
LOCAL_SRC_FILES := $(FF_SOURCE_FILES)
ifeq ($(FF_ENABLE_AMR),yes)
LOCAL_STATIC_LIBRARIES := opencore-amrnb opencore-amrwb
endif
ifeq ($(FF_ENABLE_AAC),yes)
LOCAL_STATIC_LIBRARIES := vo-aacenc
endif

LOCAL_CFLAGS := -DHAVE_AV_CONFIG_H \
                -D_FILE_OFFSET_BITS=64 \
                -D_LARGEFILE_SOURCE \
                -DPIC \
                -std=c99 \
                -ftree-vectorize \
                -ffast-math \
                -fno-signed-zeros \
                -fomit-frame-pointer

LOCAL_CFLAGS += -D__LGE__ \
                -D__LGE_TAG_UDTA_AUTH__ \
                -D__LGE_S3D__ \
                -D__LGE_FIX_MEMLEAK_AVFILTER_PICTURE_POOL__ \
                -D__LGE_FIX_MEMLEAK_AVFORMAT_PRIV_DATA__

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    LOCAL_CFLAGS += \
        -DHAVE_ARMV6=1 \
        -DHAVE_ARMV6T2=1 \
        -DHAVE_ARMVFP=1 \
        -DHAVE_VFPV3=1 \
        -DHAVE_FAST_UNALIGNED=1 \
        -DAV_HAVE_FAST_UNALIGNED=1
    ifeq ($(FF_ENABLE_NEON),yes)
        LOCAL_CFLAGS += -DHAVE_NEON=1
        LOCAL_ARM_NEON := true
    else
        LOCAL_CFLAGS += -DHAVE_NEON=0
        LOCAL_ARM_NEON := false
    endif
else
    LOCAL_CFLAGS += \
        -DHAVE_ARMV6=0 \
        -DHAVE_ARMV6T2=0 \
        -DHAVE_ARMVFP=0 \
        -DHAVE_VFPV3=0 \
        -DHAVE_FAST_UNALIGNED=0 \
        -DAV_HAVE_FAST_UNALIGNED=0 \
        -DHAVE_NEON=0
    LOCAL_ARM_NEON := false
endif

ifeq ($(FF_ENABLE_AMR),yes)
    LOCAL_CFLAGS += \
        -DCONFIG_LIBOPENCORE_AMRNB_DECODER=1 \
        -DCONFIG_LIBOPENCORE_AMRWB_DECODER=1 \
        -DCONFIG_LIBOPENCORE_AMRNB_ENCODER=1
    LOCAL_CFLAGS += \
        -I$(LOCAL_PATH)/../opencore-amr/prebuilt/include
else
    LOCAL_CFLAGS += \
        -DCONFIG_LIBOPENCORE_AMRNB_DECODER=0 \
        -DCONFIG_LIBOPENCORE_AMRWB_DECODER=0 \
        -DCONFIG_LIBOPENCORE_AMRNB_ENCODER=0
endif

ifeq ($(FF_ENABLE_AAC),yes)
    LOCAL_CFLAGS += \
        -DCONFIG_LIBVO_AACENC=1 \
        -DCONFIG_LIBVO_AACENC_ENCODER=1
    LOCAL_CFLAGS += \
        -I$(LOCAL_PATH)/../vo-aacenc/prebuilt/include
else
    LOCAL_CFLAGS += \
        -DCONFIG_LIBVO_AACENC=0 \
        -DCONFIG_LIBVO_AACENC_ENCODER=0
endif

ifeq ($(FF_ENABLE_MPEG2),yes)
    LOCAL_CFLAGS += \
        -DCONFIG_MPEG2VIDEO_DECODER=1 \
        -DCONFIG_MPEG2VIDEO_ENCODER=1 \
        -DCONFIG_MPEG2DVD_MUXER=1 \
        -DCONFIG_MPEG2SVCD_MUXER=1 \
        -DCONFIG_MPEG2VIDEO_MUXER=1 \
        -DCONFIG_MPEG2VOB_MUXER=1
else
    LOCAL_CFLAGS += \
        -DCONFIG_MPEG2VIDEO_DECODER=0 \
        -DCONFIG_MPEG2VIDEO_ENCODER=0 \
        -DCONFIG_MPEG2DVD_MUXER=0 \
        -DCONFIG_MPEG2SVCD_MUXER=0 \
        -DCONFIG_MPEG2VIDEO_MUXER=0 \
        -DCONFIG_MPEG2VOB_MUXER=0
endif

ifeq ($(FF_ENABLE_MEMDEBUG),yes)
# to check memory leak, use custom memory allocator
LOCAL_CFLAGS += -DMALLOC_PREFIX=ffmpeg_memfunc_
endif

