#!/bin/sh
VERSION="resetmsmice-1.1.2"
TARGET=`echo $VERSION | tr - _`
BDIR=`pwd`
cd .. 
rm $VERSION.tar.gz
tar -cvzf $VERSION.tar.gz $VERSION
cp $VERSION.tar.gz $TARGET.orig.tar.gz
cd $BDIR
dpkg-buildpackage
