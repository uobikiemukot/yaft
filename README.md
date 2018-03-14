# yaft (yet another framebuffer terminal)

- master: [![CircleCI](https://circleci.com/gh/uobikiemukot/yaft.svg?style=svg)](https://circleci.com/gh/uobikiemukot/yaft)
- develop: [![CircleCI](https://circleci.com/gh/uobikiemukot/yaft/tree/develop.svg?style=svg)](https://circleci.com/gh/uobikiemukot/yaft/tree/develop)

Last update: Wed Mar 14 18:44:27 JST 2018

## description

Yet Another Framebuffer Terminal (aka "yaft") is simple terminal emulator for minimalist.

Features:

+	various framebuffer types (8/15/16/24/32bpp)
+	compatible with vt102 and Linux console ([detail](http://uobikiemukot.github.io/yaft/escape.html))
+	UTF-8 encoding and UCS2 gylphs
+	256 colors (same as xterm)
+	wallpaper
+	DRCS (DECDLD/DRCSMMv1) (experimental)
+	sixel (experimental)

There are Several ports:

-	yaft for framebuffer console
	-	Linux console
	-	FreeBSD console
	-	NetBSD/OpenBSD wscons (experimental)
-	yaftx for X Window System
-	[yaft-android](https://github.com/uobikiemukot/yaft-android) for Android

## download

-	[yaft-0.2.9.tar.gz](https://github.com/uobikiemukot/yaft/archive/v0.2.9.tar.gz)

## configuration

If you want to change configuration, rewrite "conf.h".

## environment variables

-	FRAMEBUFFER=/dev/fb0: specify farmebuffer device
-	SHELL=/bin/bash: specify shell command
-	YAFT="wall": use current background as wallpaper (need [idump](https://github.com/uobikiemukot/idump) or fbv)

~~~
$ idump /path/to/wallpaper.png; tput civis; YAFT="wall" FRAMEBUFFER="/dev/fb1" SHELL="/bin/csh" yaft
~~~

## how to use your favorite fonts

You can use tools/mkfont_bdf to create "glyph.h".

usage: tools/mkfont_bdf ALIAS_FILE BDF1 BDF2 BDF3 ... > glyph.h

-	ALIAS_FILE: glyph substitution rule file (see table/alias)
-	BDF1, BDF2, BDF3...: bdf files
	+	yaft supports only "monospace" bdf font
	+	you can specify mulitiple bdf fonts (but these fonts MUST be the same size)
		+	If there is more than one glyph of the same codepoint, the glyph included in the FIRST bdf file is choosed

~~~
$ ./mkfont_bdf table/your_alias your/favorite/fonts.bdf > glyph.h
~~~

## build and install

*BSD users should check README.bsd

~~~
$ export LANG=en_US.UTF-8 # yaft uses libc's wcwidth for calculating glyph width
$ make
# make install
or
$ make yaftx
# make installx
~~~

## how to use your favorite fonts

You can use tools/mkfont_bdf to create "glyph.h".

usage: tools/mkfont_bdf ALIAS_FILE BDF1 BDF2 BDF3 ... > glyph.h

-	ALIAS_FILE: glyph substitution rule file (see table/alias)
-	BDF1, BDF2, BDF3...: bdf files
	+	yaft supports only "monospace" bdf font
	+	you can specify mulitiple bdf fonts (but these fonts MUST be the same size)
		+	If there is more than one glyph of the same codepoint, the glyph included in the FIRST bdf file is choosed

~~~
$ ./mkfont_bdf table/your_alias your/favorite/fonts.bdf > glyph.h
~~~

Or try glyph_builder.sh (bash script for creating yaft's glyph.h)

-	supported fonts: mplus, efont, milkjf, unifont, dina, terminus, profont, tamsyn

## screenshot

![screenshot1](http://uobikiemukot.github.io/img/yaft-screenshot.png)

## license
The MIT License (MIT)

Copyright (c) 2012 haru (uobikiemukot at gmail dot com)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
