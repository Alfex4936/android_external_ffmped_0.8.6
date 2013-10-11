ABI=arm-linux-androideabi
NDK_PLATFORM=linux-x86
ABI_VER=4.4.3
PREBUILT=$NDK_HOME/toolchains/$ABI-$ABI_VER/prebuilt/$NDK_PLATFORM
PLATFORM=$NDK_HOME/platforms/android-14/arch-arm

./configure \
    --target-os=linux \
    --arch=arm \
    --enable-cross-compile \
    --cc=$PREBUILT/bin/$ABI-gcc \
    --sysroot=$PLATFORM \
    --cross-prefix=$PREBUILT/bin/$ABI- \
    --disable-doc \
    --disable-ffmpeg \
    --disable-ffplay \
    --disable-ffprobe \
    --disable-ffserver \
    --disable-avdevice \
    --disable-network \
    --disable-devices \
    --disable-swscale-alpha \
    --enable-sram \
    --enable-optimizations \
    --enable-version3 \
    --enable-libopencore-amrnb \
    --enable-libopencore-amrwb \
    --enable-libvo-aacenc \
    --extra-ldflags='-L../opencore-amr/prebuilt/lib -L../vo-aacenc/prebuilt/lib/armeabi-v7a-neon' \
    --extra-cflags='-I../opencore-amr/prebuilt/include -I../vo-aacenc/prebuilt/include -marm -march=armv7-a -mfloat-abi=softfp -mfpu=neon' # ARMv7+NEON  
    #--extra-cflags='-I../opencore-amr/prebuilt/include -I../vo-aacenc/prebuilt/include  -marm -march=armv7-a -mfloat-abi=softfp -mfpu=vfp' --disable-neon # ARMv7+VFP
    #--extra-cflags='-I../opencore-amr/prebuilt/include -I../vo-aacenc/prebuilt/include  -marm -march=armv6   -mfloat-abi=softfp -mfpu=vfp' # ARMv6+VFP
    #--extra-cflags='-I../opencore-amr/prebuilt/include -I../vo-aacenc/prebuilt/include  -marm -march=armv6   -msoft-float' --disable-armvfp # ARMv6
    #--extra-cflags='-I../opencore-amr/prebuilt/include -I../vo-aacenc/prebuilt/include  -marm -march=armv5te -msoft-float' --disable-armvfp # ARMv5

    # disable mpeg2 due to the royalty issue
    #--disable-encoder=mpeg2video \
    #--disable-decoder=mpeg2_crystalhd \
    #--disable-decoder=mpeg2video \
    #--disable-muxer=mpeg2dvd \
    #--disable-muxer=mpeg2svcd \
    #--disable-muxer=mpeg2video \
    #--disable-muxer=mpeg2vob \
    #--disable-hwaccel=mpeg2_dxva2 \
    #--disable-hwaccel=mpeg2_vaapi \
    #--disable-hwaccel=mpeg2_vdpau \

# Disable the "restrict" keyword
sed 's/#define restrict restrict/#define restrict/g' config.h > config.h.new
mv config.h.new config.h

# Hide some private path
sed -e 's/--cc=\S* //g' \
    -e 's/--sysroot=\S* //g' \
    -e 's/--cross-prefix=\S* //g' \
    config.h > config.h.new
mv config.h.new config.h

# Comment out some HAVE_, CONFIG_ features. These will be configured in Android.mk
sed -e 's/#define HAVE_ARMV6 /\/\/#define HAVE_ARMV6 /g' \
    -e 's/#define HAVE_ARMV6T2 /\/\/#define HAVE_ARMV6T2 /g' \
    -e 's/#define HAVE_NEON /\/\/#define HAVE_NEON /g' \
    -e 's/#define HAVE_ARMVFP /\/\/#define HAVE_ARMVFP /g' \
    -e 's/#define HAVE_VFPV3 /\/\/#define HAVE_VFPV3 /g' \
    -e 's/#define HAVE_FAST_UNALIGNED /\/\/#define HAVE_FAST_UNALIGNED /g' \
    -e 's/#define CONFIG_LIBOPENCORE_AMRNB_DECODER /\/\/#define CONFIG_LIBOPENCORE_AMRNB_DECODER /g' \
    -e 's/#define CONFIG_LIBOPENCORE_AMRWB_DECODER /\/\/#define CONFIG_LIBOPENCORE_AMRWB_DECODER /g' \
    -e 's/#define CONFIG_LIBOPENCORE_AMRNB_ENCODER /\/\/#define CONFIG_LIBOPENCORE_AMRNB_ENCODER /g' \
    -e 's/#define CONFIG_LIBVO_AACENC /\/\/#define CONFIG_LIBVO_AACENC /g' \
    -e 's/#define CONFIG_LIBVO_AACENC_ENCODER /\/\/#define CONFIG_LIBVO_AACENC_ENCODER /g' \
    config.h > config.h.new
mv config.h.new config.h

# Comment out AV_HAVE_FAST_UNALIGNED feature. This will be confitured in Android.mk
sed 's/#define AV_HAVE_FAST_UNALIGNED /\/\/#define AV_HAVE_FAST_UNALIGNED /g' libavutil/avconfig.h > libavutil/avconfig.h.new
mv libavutil/avconfig.h.new libavutil/avconfig.h

# Comment out some MPEG2 features. These will be configured in Android.mk
sed -e 's/#define CONFIG_MPEG2VIDEO_DECODER /\/\/#define CONFIG_MPEG2VIDEO_DECODER /g' \
    -e 's/#define CONFIG_MPEG2VIDEO_ENCODER /\/\/#define CONFIG_MPEG2VIDEO_ENCODER /g' \
    -e 's/#define CONFIG_MPEG2DVD_MUXER /\/\/#define CONFIG_MPEG2DVD_MUXER /g' \
    -e 's/#define CONFIG_MPEG2SVCD_MUXER /\/\/#define CONFIG_MPEG2SVCD_MUXER /g' \
    -e 's/#define CONFIG_MPEG2VIDEO_MUXER /\/\/#define CONFIG_MPEG2VIDEO_MUXER /g' \
    -e 's/#define CONFIG_MPEG2VOB_MUXER /\/\/#define CONFIG_MPEG2VOB_MUXER /g' \
    config.h > config.h.new
mv config.h.new config.h

