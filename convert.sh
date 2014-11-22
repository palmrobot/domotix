#!/bin/bash

cp -v $1 $2
sed -i -e 's/^\s*//' $2
sed -i -e '/^$/d' $2
sed -i -e 's/>\s*</></g' $2


