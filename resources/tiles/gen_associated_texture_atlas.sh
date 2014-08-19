#!/bin/bash

echo -n > $1

for FILE in ${@:2}
do
    while read LINE
    do
        echo "$LINE: ${FILE%.job}.png" >> $1
    done < $FILE
done