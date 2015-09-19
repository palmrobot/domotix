#!/bin/bash

LIST=`ls *.html | awk -F "." {'print $1'}`
for i in $LIST; do
cp -v ${i}.html ${i}.htm
sed -i -e 's/^\s*//' ${i}.htm
sed -i -e '/^$/d' ${i}.htm
sed -i -e 's/>\s*</></g' ${i}.htm
done

cp index.htm debug.htm
patch -p0 < patch_debug.htm
rm -f debug.htm.rej
rm -f debug.htm.orig

echo "Convertion Done!"

