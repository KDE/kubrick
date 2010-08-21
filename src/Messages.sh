#!/bin/sh
$EXTRACTRC *.rc >> rc.cpp
$XGETTEXT *.cpp -o $podir/kubrick.pot
