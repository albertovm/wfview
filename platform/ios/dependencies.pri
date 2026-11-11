# Dependency paths are supplied by scripts/configure-ios.sh.
isEmpty(WFVIEW_IOS_ROOT): error("Set WFVIEW_IOS_ROOT to the prepared iOS dependency directory")
isEmpty(WFVIEW_IOS_PLATFORM): WFVIEW_IOS_PLATFORM = iphoneos
isEmpty(WFVIEW_IOS_ARCH): WFVIEW_IOS_ARCH = arm64
WFVIEW_IOS_PREFIX = $$WFVIEW_IOS_ROOT/install/$$WFVIEW_IOS_PLATFORM-$$WFVIEW_IOS_ARCH
!exists($$WFVIEW_IOS_PREFIX/lib/libopus.a): error("Build Opus for iOS first")
INCLUDEPATH += $$quote($$WFVIEW_IOS_PREFIX/include/opus) \
               $$quote($$WFVIEW_IOS_ROOT/src/eigen-3.4.0) \
               $$quote($$WFVIEW_IOS_ROOT/src/r8brain-free-src-version-6.5) \
               $$quote($$WFVIEW_IOS_ROOT/src/qcustomplot-source)
LIBS -= -lopus
# Passing the archive through LIBS makes qmake's Xcode generator turn it into
# a -lopus search and, with Xcode 27, emit a stray bare -L. Feed the absolute
# archive path directly to the linker instead.
QMAKE_LFLAGS += $$WFVIEW_IOS_PREFIX/lib/libopus.a
# Compile QCustomPlot into the application, rather than linking a desktop dylib.
DEFINES -= QCUSTOMPLOT_USE_LIBRARY
SOURCES += $$quote($$WFVIEW_IOS_ROOT/src/qcustomplot-source/qcustomplot.cpp)
HEADERS += $$quote($$WFVIEW_IOS_ROOT/src/qcustomplot-source/qcustomplot.h)
QMAKE_IOS_DEPLOYMENT_TARGET = 16.0

# Use Apple's native multimedia integration on iOS. Automatically importing
# the FFmpeg plugin also requires separate FFmpeg libraries in static builds.
QTPLUGIN.multimedia = darwinmediaplugin

# Qt 6.8.3's qyieldcpu.h uses the ARM ACLE __yield intrinsic without pulling
# in its declaration. Clang 21 (Xcode 27) diagnoses that as an error, so make
# the platform intrinsic declarations available before any Qt header.
equals(WFVIEW_IOS_ARCH, arm64): QMAKE_CXXFLAGS += -include arm_acle.h
