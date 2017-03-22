#!/bin/sh
VERSION="resetmsmice-1.1.2"
BDIR=`pwd`
cd .. 
rm $VERSION.tar.gz
tar -cvzf $VERSION.tar.gz $VERSION
cp $VERSION.tar.gz ~/rpmbuild/SOURCES
cd $BDIR
rpmbuild -ba resetmsmice-fedora.spec
