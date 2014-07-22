# yaft (yet another framebuffer terminal)

Last update: Thu Jul 10 20:07:09 JST 2014

## description

yaft is simple framebuffer terminal emulator for minimalist (living without X).
This software is being developed to replace Linux console for personal use.
<!-- Linux console can handle only 512 glyphs and doesn't support 256 color. -->

yaft supports UCS2 glyphs (including wide character) and 256 color.
What you need for build is only make and gcc (or bmake and clang).

Main target is Linux console, but yaft supports some other framebuffer platform, FreeBSD console and NetBSD/OpenBSD wscons (experimental).
And there are other (non framebuffer) ports, yaftx (X Window System) and yaft-android (Android).

This repository includes yaft, yaft-freebsd, yaft-netbsd, yaft-openbsd and yaftx.
yaft-android is found [here](https://github.com/uobikiemukot/yaft-android).

## download

(up-to-date)

~~~
$ git clone https://github.com/uobikiemukot/yaft
~~~
or

(maybe something old)

-	http://uobikiemukot.github.io/yaft/release/yaft-0.2.7.tar.gz

## features

+	recognizes most of escape sequences of vt102 and Linux console ([detail](http://uobikiemukot.github.io/yaft/escape.html))
+	supports various framebuffer types (8/15/16/24/32bpp)
+	supports (only) UTF-8 encoding and UCS2 gylphs with embedded fonts
+	supports 256 colors (same as xterm)
+	supports wallpaper
+	supports DRCS (DECDLD/DRCSMMv1) (experimental)
+	supports sixel (experimental)

## configuration

If you want to change configuration, rewrite conf.h

### color

This value is an index of color_list (see color.h)

+	DEFAULT_FG = 7,
+	DEFAULT_BG = 0,
+	ACTIVE_CURSOR_COLOR  = 2,
+	PASSIVE_CURSOR_COLOR = 1, /* effect only in X Window System and linux console */

### misc

+	DEBUG            = false,  /* write dump of input to stdout, debug message to stderr */
+	TABSTOP          = 8,      /* hardware tabstop */
+	BACKGROUND_DRAW  = false,  /* always draw even if vt is not active (maybe linux console only, useful for multi display) */
+	SUBSTITUTE_HALF  = 0x0020, /* used for missing glyph (single width): U+0020 (SPACE)) */
+	SUBSTITUTE_WIDE  = 0x3000, /* used for missing glyph (double width): U+3000 (IDEOGRAPHIC SPACE) */
+	REPLACEMENT_CHAR = 0x003F, /* used for malformed UTF-8 sequence    : U+003F (QUESTION MARK)  */

### terminal name and path

+	static char *term_name = "TERM=yaft-256color";
+	static char *fb_path   = "/dev/fb0";
+	static char *shell_cmd = "/bin/bash";

## how to use your favorite fonts

You can use tools/mkfont_bdf to create glyph.h

usage: tools/mkfont_bdf ALIAS_FILE BDF1 BDF2 BDF3 ... > glyph.h

-	ALIAS_FILE: glyph substitution rule file (see table/alias)
-	BDF1, BDF2, BDF3...:
	+	yaft supports only "monospace" bdf font
	+	yaft doesn't support bold fonts (yaft just brightens color at bold attribute)
	+	you can specify mulitiple bdf fonts (but these fonts MUST be the same size)
	+	if there are same glyphs in different bdf file,
		yaft use the glyph included in the last specified bdf file

change makefile like this

~~~
#./mkfont_bdf table/alias fonts/milkjf_k16.bdf fonts/milkjf_8x16r.bdf fonts/milkjf_8x16.bdf > glyph.h
./mkfont_bdf table/your_alias your/favorite/fonts.bdf > glyph.h
~~~

or change yaft.h

~~~
//#include "glyph.h"
#include "glyph_you_created.h"
~~~

## environment variable

~~~
$ FRAMEBUFFER="/dev/fb1" yaft # use another framebuffer device
$ idump /path/to/wallpaper.png; YAFT="wall" yaft # set wallpaper (see yaft_wall script)
~~~

## build and install (yaft)

Please check makefile and LANG environment variable before make
(yaft uses wcwidth in libc for calculating glyph width)

~~~
($ export LANG=en_US.UTF-8)
$ make
# make install
(or install manually)
~~~

## build and install (yaftx)

~~~
$ make yaftx
~~~

Please install manually

## terminfo/termcap

terminfo/termcap is found in info/ directory

## usage

~~~
$ yaft
~~~

For enabling wallpaper, you can use yaft_wall script (it requires [fbv](http://www.eclis.ch/fbv/))

~~~
$ yaft_wall /path/to/wallpaper.jpg
~~~

or you can use [idump](https://github.com/uobikiemukot/idump)

~~~
$ idump /path/to/wallpaper.jpg; YAFT="wall" yaft
~~~

## screenshot

![screenshot1](http://uobikiemukot.github.io/img/yaft-screenshot.png)

## license
The MIT License (MIT)

Copyright (c) 2012 haru (uobikiemukot at gmail dot com)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
