% yaft
% haru
% Last update: 12/11/13

yet another framebuffer terminal

## download
-	[yaft-0.2.0](http://www.nak.ics.keio.ac.jp/~haru/yaft/release/yaft-0.2.0.tar.gz)
-	[yaft-freebsd-0.1.9](http://www.nak.ics.keio.ac.jp/~haru/yaft/release/yaft-freebsd-0.1.9.tar.gz)

## features
+	recognizes most of escape sequences of vt102 and linux console ([all sequences](escape.html))
+	supports various framebuffer types (8/15/16/24/32bpp)
+	supports (only) UTF-8 encoding
+	supports 256 colors (same as xterm)
+	supports wallpaper

## recent changes
for more detail, see [ChangeLog](./release/yaft-current/ChangeLog)

### version 0.1.9 (2012-10-18)

-	supported virtual console switching
-	removed all optios without "YAFT=wall"

### version 0.2.0 (2012-11-09)

-	suppported BDF

~~~
	$ ./mkfont alias /your/favorite/bdf > glyph.h
	$ make yaft
~~~

## screenshot
![](http://www.nak.ics.keio.ac.jp/~haru/yaft/img/yaft-screenshot.png)

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

## license
Copyright (c) 2012 haru (uobikiemukot at gmail dot com)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
-->
