Building via qmake
==================

The following commands assume the following environment variables are set:

 * `$srcdir`: The directory into which the source has been downloaded and
   unpacked.
 * `$pkgdir`: The directory into which installable files are places. This is
   `/` for local installations, or can be different directory when preparing a
   redistributable package.

Either set them accordingly, or replace them in the below commands as desired.

The following sources should be downloaded and unpacked into `$srcdir`:

  * This repository (either `master` or a specific tag).
  * `git clone https://chromium.googlesource.com/breakpad/breakpad breakpad`
  * `git clone https://chromium.googlesource.com/linux-syscall-support breakpad-lss`
  * telegramdesktop.desktop :
    `wget https://aur.archlinux.org/cgit/aur.git/plain/telegramdesktop.desktop?h=telegram-desktop -O telegramdesktop.desktop`
  * tg.protocol :
    `wget https://aur.archlinux.org/cgit/aur.git/plain/tg.protocol?h=telegram-desktop -O tg.protocol`

Preparation
-----------
    cd $srcdir

    wget http://xkbcommon.org/download/libxkbcommon-0.6.1.tar.xz
    tar -xvf libxkbcommon-0.6.1.tar.xz
    cd libxkbcommon-0.6.1
    ./configure --prefix=$srcdir/libxkbcommon --disable-x11 CFLAGS=-fPIC
    make install
    export PKG_CONFIG_PATH=$srcdir/libxkbcommon/lib/pkgconfig


    pushd "$srcdir/tdesktop"
    git clone git://code.qt.io/qt/qt5.git

    pushd qt5

    git checkout 5.6
    perl init-repository --module-subset=qtbase,qtimageformats
    git checkout v5.6.0

    pushd qtimageformats
    git checkout v5.6.0
    popd

    pushd qtbase
    git checkout v5.6.0
    git apply $srcdir/tdesktop/Telegram/Patches/qtbase_5_6_0.diff

    popd
    popd

    mkdir -p $srcdir/Libraries
    ln -s "$srcdir/breakpad" "$srcdir/Libraries/breakpad"

    rm -rf $srcdir/Libraries/breakpad/src/third_party/lss
    ln -s "$srcdir/breakpad-lss" "$srcdir/Libraries/breakpad/src/third_party/lss"

Building
--------


    # Build patched Qt
    cd "$srcdir/tdesktop/qt5"
    ./configure -prefix "$srcdir/qt" -release -opensource -confirm-license -qt-zlib \
                -qt-libpng -qt-libjpeg -qt-freetype -qt-harfbuzz -qt-pcre -qt-xcb \
                -qt-xkbcommon-x11 -no-opengl -static -nomake examples -nomake tests

    make module-qtbase module-qtimageformats
    make module-qtbase-install_subtargets module-qtimageformats-install_subtargets

    export PATH="$srcdir/qt/bin:$PATH"

    # Build breakpad
    cd "$srcdir/Libraries/breakpad"
    ./configure
    make

    # Build codegen_style
    mkdir -p "$srcdir/tdesktop/Linux/obj/codegen_style/Release"
    cd "$srcdir/tdesktop/Linux/obj/codegen_style/Release"
    qmake CONFIG+=release ../../../../Telegram/build/qmake/codegen_style/codegen_style.pro
    make

    # Build codegen_numbers
    mkdir -p "$srcdir/tdesktop/Linux/obj/codegen_numbers/Release"
    cd "$srcdir/tdesktop/Linux/obj/codegen_numbers/Release"
    qmake CONFIG+=release ../../../../Telegram/build/qmake/codegen_numbers/codegen_numbers.pro
    make

    # Build MetaLang
    mkdir -p "$srcdir/tdesktop/Linux/DebugIntermediateLang"
    cd "$srcdir/tdesktop/Linux/DebugIntermediateLang"
    qmake CONFIG+=release "../../Telegram/MetaLang.pro"
    make

    # Build Telegram Desktop
    mkdir -p "$srcdir/tdesktop/Linux/ReleaseIntermediate"
    cd "$srcdir/tdesktop/Linux/ReleaseIntermediate"
    qmake QT_TDESKTOP_VERSION=5.6.0 QT_TDESKTOP_PATH=$srcdir/qt CONFIG+=release "../../Telegram/Telegram.pro"
    make

Installation
------------


    install -dm755 "$pkgdir/usr/bin"
    install -m755 "$srcdir/tdesktop/Linux/Release/Telegram" "$pkgdir/usr/bin/telegram-desktop"

    install -d "$pkgdir/usr/share/applications"
    install -m644 "$srcdir/telegramdesktop.desktop" "$pkgdir/usr/share/applications/telegramdesktop.desktop"

    install -d "$pkgdir/usr/share/kde4/services"
    install -m644 "$srcdir/tg.protocol" "$pkgdir/usr/share/kde4/services/tg.protocol"

    local icon_size icon_dir
    for icon_size in 16 32 48 64 128 256 512; do
      icon_dir="$pkgdir/usr/share/icons/hicolor/${icon_size}x${icon_size}/apps"

      install -d "$icon_dir"
      install -m644 "$srcdir/tdesktop/Telegram/SourceFiles/art/icon${icon_size}.png" "$icon_dir/telegram-desktop.png"
    done

Notes
-----

These instructions are based on the [ArchLinux package][arch-package] for
telegram-desktop.

In case these instructions are at some point out of date, the above may serve
as an update reference.

[arch-package]: https://aur.archlinux.org/packages/telegram-desktop/
