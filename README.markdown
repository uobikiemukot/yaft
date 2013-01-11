% yaft
% haru
% Last update: 12/12/19

yet another framebuffer terminal

## download
-	[yaft-0.2.3](./release/yaft-0.2.3.tar.gz) (Linux console)
-	[yaft-x-0.2.3](./release/yaft-x-0.2.3.tar.gz) (X Window System)
-	[yaft-freebsd-0.2.3](./release/yaft-freebsd-0.2.3.tar.gz) (FreeBSD console)

## features
+	recognizes most of escape sequences of vt102 and linux console ([detail](escape.html))
+	supports various framebuffer types (8/15/16/24/32bpp)
+	supports (only) UTF-8 encoding
+	supports 256 colors (same as xterm)
+	supports wallpaper

## recent changes
for more detail, see [ChangeLog](./changelog.html)

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

## 2012-12-19
-	version 0.2.3
-	bug fix
	-	wrong behavior of bce (ED, EL, DL, IL, ICH, DCH, ECH) (reported by IWAMOTO Kouichi ([@ttdoda]))

[@ttdoda]: http://doda.teraterm.org/whoami.xhtm

## screenshot
![screenshot1](./img/yaft-screenshot.png)

<!--
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

+	DEBUG = false,
+	TABSTOP = 8,
+	SUBSTITUTE_HALF = 0x20, /* SPACE */
+	SUBSTITUTE_WIDE = 0x3000, /* IDEOGRAPHIC SPACE */

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
-->

## license
Copyright (c) 2012 haru (uobikiemukot at gmail dot com)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
