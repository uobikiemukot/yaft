# yet another framebuffer terminal
Last update: Fri May 16 16:50:52 JST 2014

## download

~~~
$ git clone https://github.com/uobikiemukot/yaft
~~~

## features
+	recognizes most of escape sequences of vt102 and linux console ([detail](http://uobikiemukot.github.io/yaft/escape.html))
+	supports various framebuffer types (8/15/16/24/32bpp)
+	supports (only) UTF-8 encoding
+	supports 256 colors (same as xterm)
+	supports wallpaper
+	supports DRCS (DECDLD/DRCSMMv1) (experimental)

## recent changes
for more detail, see [ChangeLog](http://uobikiemukot.github.io/yaft/changelog.html) (Japanese)

### 2014-05-16
-	version 0.2.5
-	supported DRCS (DECDLD/DRCSMMv1)

### 2012-11-09
-	version 0.2.0
-	suppported BDF

~~~
$ ./mkfont alias-file your/favorite/bdf > glyph.h
$ make yaft
~~~

### 2012-11-30
-	version 0.2.1
-	bug fix
	-	single CSI u (not following CSI s) causes Segmentation fault (reported by saitoha ([@kefir_]))

[@kefir_]: http://twitter.com/kefir_

### 2012-12-02
-	version 0.2.2
-	improved UTF-8 parser (now valid for [UTF-8 decoder capability and stress test])
-	bug fix
	-	not working glyph substitution

[UTF-8 decoder capability and stress test]: http://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt

### 2012-12-19
-	version 0.2.3
-	bug fix
	-	wrong behavior of bce (ED, EL, DL, IL, ICH, DCH, ECH) (reported by IWAMOTO Kouichi ([@ttdoda]))

[@ttdoda]: http://doda.teraterm.org/whoami.xhtm

## screenshot
![screenshot1](http://uobikiemukot.github.io/img/yaft-screenshot.png)

## configuration
if you want to change configuration, rewrite conf.h

### path and terminal name

+	static char *fb_path = "/dev/fb0";
+	static char *shell_cmd = "/bin/bash";
+	static char *term_name = "TERM=yaft-256color";

### color
the value is an index of color_list[] in color.h

+	DEFAULT_FG = 7,
+	DEFAULT_BG = 0,
+	CURSOR_COLOR = 2,

### misc

+	DEBUG = false,             /* write dump of input to stdout, debug message to stderr */
+	TABSTOP = 8,               /* hardware tabstop */
+	LAZY_DRAW = false,         /* reduce drawing when input data size is larger than BUFSIZE */
+	SUBSTITUTE_HALF = 0x20,    /* used for missing glyph(single width): SPACE (U+20) */
+	SUBSTITUTE_WIDE = 0x3013,  /* used for missing glyph(double width): GETA MARK (U+3000) */
+	REPLACEMENT_CHAR = 0x3F,   /* used for malformed UTF-8 sequence: QUESTION MARK (U+3F) */

## environment variable

~~~
$ FRAMEBUFFER="/dev/fb1" yaft # use another framebuffer device
$ YAFT="wallpaper" yaft # enable wallpaper, see yaft_wall for detail
$ YAFT="background" yaft # enable background drawing, useful for multi display
~~~

## install

~~~
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

## tools
some useful tools (bdf to yaft font converter etc) are found in tools/ directory

## license
The MIT License (MIT)

Copyright (c) 2012 haru (uobikiemukot at gmail dot com)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
