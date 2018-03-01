
DEB_HOST_ARCH ?= $(shell dpkg-architecture -qDEB_HOST_ARCH)

gn_args=is_clang=false \
        clang_use_chrome_plugins=false \
        is_debug=false \
        use_sysroot=false \
        enable_nacl=false

# disabled features
ninja_args=is_debug=false \
		   is_clang=false \
		   clang_use_chrome_plugins=false \
           use_gtk3=false \
           use_ozone=false \
           use_gconf=false \
           use_sysroot=false \
           use_vulcanize=false \
           use_gnome_keyring=false \
           rtc_libvpx_build_vp9=false \
           treat_warnings_as_errors=false \
           enable_nacl=false \
           enable_nacl_nonsfi=false \
           enable_google_now=false \
           enable_widevine=false \
           enable_hangout_services_extension=false \
           enable_iterator_debugging=false \
           gold_path=\"\" \
           linux_use_bundled_binutils=false \

# enabled features
ninja_args+=use_gio=true \
            use_gold=true \
            use_pulseaudio=true \
            link_pulseaudio=true \
            proprietary_codecs=true \
            ffmpeg_branding=\"Chrome\" \
            fieldtrial_testing_like_official_build=true \


# set the appropriate cpu architecture
ifeq (i386,$(DEB_HOST_ARCH))
ninja_args+=host_cpu=\"x86\"
endif
ifeq (amd64,$(DEB_HOST_ARCH))
ninja_args+=host_cpu=\"x64\"
endif
ifeq (arm64,$(DEB_HOST_ARCH))
ninja_args+=host_cpu=\"arm64\"
endif
ifeq (armhf,$(DEB_HOST_ARCH))
ninja_args+=host_cpu=\"arm\" \
            arm_use_neon=false
endif

all: build-gn generate-gn-args

build-gn:
	cd src && \
		tools/gn/bootstrap/bootstrap.py -s --no-clean --no-rebuild --gn-gen-args="$(defines)"

generate-gn-args:
	cd src && \
		out/Release/gn gen out/Release --args="$(ninja_args)"

build-chrome:
	cd src && \
		ninja -C out/Release chrome chrome_sandbox

update-submodules:
	git submodule update --init --recursive
	git submodule update --remote
