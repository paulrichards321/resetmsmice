#!/bin/sh
VERSION="resetmsmice-1.2.1"
BDIR=`pwd`
if [ -z "$1" ]
then
  echo "Not signing the deb package. Specify email after build-deb.sh to sign with a gpg key."
  OPTION="--no-sign"
else
  OPTION="-k$1"
fi
TARGET=`echo $VERSION | tr - _`

echo "$BDIR" | grep -q -i resetmsmice
if [ $? -eq 0 ]
then
  autoreconf -i .
  if [ $? -ne 0 ]
  then
    echo "autoreconf failed. Stopping."
    exit 1
  fi
  TEMP_DIR=`mktemp -d -p .. resetmsmice.build.XXXX`
  if [ -n "$TEMP_DIR" -a -d "$TEMP_DIR" ]
  then
    mkdir $TEMP_DIR/$VERSION
    cp -a . $TEMP_DIR/$VERSION
    cd $TEMP_DIR
    tar -czf $VERSION.tar.gz $VERSION
    cp $VERSION.tar.gz $TARGET.orig.tar.gz
    cd $VERSION
    dpkg-buildpackage $OPTION
    echo 
    echo "Package results in $TEMP_DIR"
  else
    echo "Failed to create temporary directory!"
    exit 1
  fi
else
  echo "Must be in resetmsmice source directory!"
  exit 1
fi
cd $BDIR
