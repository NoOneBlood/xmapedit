#!/bin/bash

oggbase=https://downloads.xiph.org/releases/ogg
oggfile=libogg-1.3.2.tar.gz
oggfilesum=e19ee34711d7af328cb26287f4137e70630e7261b17cbe3cd41011d73a654692
vorbisbase=https://downloads.xiph.org/releases/vorbis
vorbisfile=libvorbis-1.3.5.tar.gz
vorbisfilesum=6efbcecdd3e5dfbf090341b485da9d176eb250d893e3eb378c428a2db38301ce

oggurl=$oggbase/$oggfile
vorbisurl=$vorbisbase/$vorbisfile

# Inherit build architectures from the environment given by Xcode.
arches=()
test -n "$ARCHS" && arches=($ARCHS)
archflags="${arches[*]/#/-arch }"

destdir=$(cd $(dirname ${BASH_SOURCE[0]}) && pwd)

# Determine how many CPU cores the system has so we can parallelise compiles.
ncpus=$(sysctl -q -n hw.ncpu)
makeflags=-j${ncpus:-1}

export MAKE="xcrun make"
export CC="xcrun clang"
export CXX="xcrun clang++"
export LD="xcrun ld"
export AR="xcrun ar"
export RANLIB="xcrun ranlib"
export STRIP="xcrun strip"
export CFLAGS="$archflags -mmacosx-version-min=10.9"

check_tools() {
    echo "+++ Checking build tools"
    if ! xcrun -f make &>/dev/null; then
        echo "Error: could not execute 'make'. Giving up."
        exit 1
    fi
}

check_file() {
    local shasum=$1
    local filename=$2
    shasum -a 256 -c -s - <<EOT
$shasum  $filename
EOT
}

if test ! -f $destdir/out/lib/libogg.a; then
    check_tools

    mkdir libogg-build

    echo "+++ Fetching and unpacking $oggurl"
    if [ -f $oggfile ]; then
        check_file $oggfilesum $oggfile || rm -f $oggfile
    fi
    if [ ! -f $oggfile ]; then
        curl -sL $oggurl -o $oggfile || exit
        check_file $oggfilesum $oggfile || rm -f $oggfile
    fi
    (cd libogg-build; tar zx --strip-components 1) < $oggfile || exit

    echo "+++ Configuring libogg"
    (cd libogg-build; ./configure --prefix=/out) || exit

    echo "+++ Building libogg"
    (cd libogg-build; $MAKE $makeflags) || exit

    echo "+++ Installing libogg to $destdir"
    (cd libogg-build; $MAKE DESTDIR=$destdir install) || exit

    # echo "+++ Cleaning up libogg"
    # rm -rf libogg-build
fi

if test ! -f $destdir/out/lib/libvorbisfile.a; then
    check_tools

    mkdir libvorbis-build

    echo "+++ Fetching and unpacking $vorbisurl"
    if [ -f $vorbisfile ]; then
        check_file $vorbisfilesum $vorbisfile || rm -f $vorbisfile
    fi
    if [ ! -f $vorbisfile ]; then
        curl -sL $vorbisurl -o $vorbisfile || exit
        check_file $vorbisfilesum $vorbisfile || rm -f $vorbisfile
    fi
    (cd libvorbis-build; tar zx --strip-components 1) < $vorbisfile || exit

    echo "+++ Configuring libvorbis"
    (cd libvorbis-build; PKG_CONFIG=/usr/bin/false ./configure --prefix=/out --with-ogg=$destdir/out) || exit

    echo "+++ Building libvorbis"
    (cd libvorbis-build; $MAKE $makeflags) || exit

    echo "+++ Installing libvorbis to $destdir"
    (cd libvorbis-build; $MAKE DESTDIR=$destdir install) || exit

    # echo "+++ Cleaning up libvorbis"
    # rm -rf libvorbis-build
fi
