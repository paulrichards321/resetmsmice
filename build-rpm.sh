#!/bin/sh
VERSION="resetmsmice-1.2.1"
BDIR=`pwd`
echo "$BDIR" | grep -q -i resetmsmice
if [ $? -eq 0 ]
then
  autoreconf -i .
  if [ $? -ne 0 ]
  then
    echo "autoreconf failed. Stopping."
    exit 1
  fi
  mkdir -p ~/rpmbuild/SOURCES
  TEMP_DIR=`mktemp -d -p .. resetmsmice.build.XXXX`
  if [ -n "$TEMP_DIR" -a -d "$TEMP_DIR" ]
  then
    mkdir $TEMP_DIR/$VERSION
    cp -a . $TEMP_DIR/$VERSION
    cd $TEMP_DIR
    tar -czf $VERSION.tar.gz $VERSION
    cp $VERSION.tar.gz ~/rpmbuild/SOURCES
    cd $BDIR
    rpmbuild -ba resetmsmice-fedora.spec
    rm -f $TEMP_DIR/$VERSION.tar.gz
    rm -rf $TEMP_DIR/$VERSION
    rmdir $TEMP_DIR
    echo
    echo "Package results in ~/rpmbuild/RPMS"
  else
    echo "Failed to create temporary directory!"
    exit 1
  fi
else
  echo "Must be in resetmsmice source directory!"
  exit 1
fi
cd $BDIR
