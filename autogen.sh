#!/bin/bash

dobuild=0
doprep=0
dotest=0
doinstall=0

srcdir=$(pwd)

while [ $# -gt 0 ]; do
    case "$1" in
        --build|-b)
            dobuild=1
            shift
        ;;

        --prep|-p)
            doprep=1
            shift
        ;;

        --test|-t)
            dotest=1
            shift
        ;;

        --install|-i)
            doinstall=1
            shift
        ;;

    esac
done

if [ ${dobuild} -eq 0 -a ${doprep} -eq 0 -a ${dotest} -eq 0 -a ${doinstall} -eq 0 ]; then
    dobuild=1
    doprep=1
    dotest=1
    doinstall=1
fi

# Rebuilds the entire foo in one go. One shot, one kill.
rm -rf build/
mkdir -p build
cd build
if [ ${doprep} -eq 1 ]; then
    cmake \
        -DCMAKE_VERBOSE_MAKEFILE=ON \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DLIB_INSTALL_DIR=/usr/lib64 \
        -DINCLUDE_INSTALL_DIR=/usr/include \
        -DUSE_LIBCALENDARING=ON \
        -DPHP_BINDINGS=ON \
        -DPYTHON_BINDINGS=ON \
        ..
fi

if [ ${dobuild} -eq 1 ]; then
    make
fi

if [ ${dotest} -eq 1 ]; then
    # Execute some tests?
    pushd tests
    ./benchmarktest
    ./calendaringtest
    ./formattest
    ./freebusytest
    ./icalendartest
    ./kcalconversiontest
    ./upgradetest
    popd
fi

if [ ${doinstall} -eq 1 ]; then
    make install DESTDIR=${TMPDIR:-/tmp}
fi

cd ..

git archive --prefix=libkolab-0.3.1/ HEAD | gzip -c > libkolab-0.3.1.tar.gz

cp libkolab-0.3.1.tar.gz `rpm --eval='%{_sourcedir}'`

