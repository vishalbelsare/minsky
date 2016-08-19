#!/bin/bash
rm -rf doc/minsky
(cd doc; sh makedoc.sh)

target=gui-tk/library/help
git rm -rf $target/*
mkdir -p $target/minsky
find doc/minsky \( -name "*.html" -o -name "*.css" -o -name "*.png" \) -exec cp {} $target/minsky \;
cp -r -f doc/minsky.html $target
for i in $target/minsky/*; do git add $i; done
perl makeRefDb.pl doc/minsky/labels.pl >GUI/helpRefDb.tcl
git commit -m "Making doc consistent in git"
