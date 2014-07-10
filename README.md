# yaft (yet another framebuffer terminal)

Last update: Thu Jul 10 20:07:09 JST 2014

yaft is simple framebuffer terminal for minimalist.
This software developed to replace linux console.

Most defferent points are supporting UTF-8 encoding (with embedded font) and 256 color mode.
yaft requires minimal dependency, what you need for build is only make and gcc (or clang).

Main target is linux console, but yaft supports some other framebuffer platform (freebsd console and netbsd wscons).
And there are other (non framebuffer) ports, yaftx (X Window System) and yaft-android (Android).

## download

~~~
$ git clone https://github.com/uobikiemukot/yaft
~~~
or

-	http://uobikiemukot.github.io/yaft/release/yaft-0.2.7.tar.gz

## features

+	recognizes most of escape sequences of vt102 and linux console ([detail](http://uobikiemukot.github.io/yaft/escape.html))
+	supports various framebuffer types (8/15/16/24/32bpp)
+	supports (only) UTF-8 encoding
+	supports 256 colors (same as xterm)
+	supports wallpaper
+	supports DRCS (DECDLD/DRCSMMv1) (experimental)
+	supports sixel (experimental)

## configuration

if you want to change configuration, rewrite conf.h
(non linux user should change include file in yaft.c) 

### path and terminal name

+	static char *fb_path   = "/dev/fb0";           /* defined in linux.h, freebsd.h, netbsd.h */
+	static char *shell_cmd = "/bin/bash";          /* defined in linux.h, freebsd.h, netbsd.h, x.h */
+	static char *term_name = "TERM=yaft-256color";

### color

the value is an index of color_list[] in color.h

+	DEFAULT_FG = 7,
+	DEFAULT_BG = 0,
+	ACTIVE_CURSOR_COLOR  = 2,
+	PASSIVE_CURSOR_COLOR = 1, /* effect only in X Window System and linux console */

### misc

+	DEBUG            = false,  /* write dump of input to stdout, debug message to stderr */
+	TABSTOP          = 8,      /* hardware tabstop */
+	LAZY_DRAW        = false,  /* don't draw when input data size is larger than BUFSIZE (experimental) */
+	BACKGROUND_DRAW  = false,  /* always draw even if vt is not active (linux console only) */
+	WALLPAPER        = false,  /* copy framebuffer before startup, and use it as wallpaper */
+	SUBSTITUTE_HALF  = 0x0020, /* used for missing glyph(single width): U+FFFD (REPLACEMENT CHARACTER)) */
+	SUBSTITUTE_WIDE  = 0x3013, /* used for missing glyph(double width): U+3013 (GETA MARK) */
+	REPLACEMENT_CHAR = 0x0020, /* used for malformed UTF-8 sequence   : U+FFFD (REPLACEMENT CHARACTER)  */

## environment variable

~~~
$ FRAMEBUFFER="/dev/fb1" yaft # use another framebuffer device
~~~

## install

please check makefile and LANG environment variable before make
(yaft uses wcwidth in libc for calculating glyph width)

~~~
($ export LANG=en_US.UTF-8)
$ make
# make install
~~~

## usage

~~~
$ yaft
~~~

for displaying wallpaper, you can use yaft_wall script (it requires [fbv])

~~~
$ yaft_wall /path/to/wallpaper.jpg
~~~

[fbv]: http://www.eclis.ch/fbv/

## screenshot

![screenshot1](http://uobikiemukot.github.io/img/yaft-screenshot.png)

## license
The MIT License (MIT)

Copyright (c) 2012 haru (uobikiemukot at gmail dot com)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
