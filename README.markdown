# yaft
yet another framebuffer terminal

![yaft]

[yaft]: http://www.nak.ics.keio.ac.jp/~haru/yaft/yaft-blue.png

## download
- [yaft-0.1.6](http://www.nak.ics.keio.ac.jp/~haru/yaft/release/yaft-0.1.6.tar.gz)

## feature
+	linux framebuffer  
+	UTF-8 support
+	256 colors
+	wallpaper

## configuration
if you want to change configuration, rewrite conf.h

### path and terminal name

+	static char *font_path[] = { "/path/to/fonts0.yaft", "/path/to/fonts1.yaft", ..., NULL, };  
+	static char *glyph_alias = "/path/to/alias";  
+	static char *fb_path = "/dev/fb0";  
+	static char *shell_cmd = "/bin/bash";  
+	static char *term_name = "TERM=yaft-256color";  

### color
all color defined in color.h

+	DEFAULT_FG = 7,  
+	DEFAULT_BG = 0,  
+	CURSOR_COLOR = 2,  

### misc

+	DEBUG = false,  
+	TABSTOP = 8,  
+	SELECT_TIMEOUT = 20000,  

## install

~~~
$ make
# make install
~~~

## usage
yaft_wall requires [fbv]

~~~
$ yaft
~~~

OR

~~~
$ yaft_wall /path/to/wallpaper.jpg
~~~

[fbv]: http://www.eclis.ch/fbv/

## license
Copyright (c) 2012 haru (uobikiemukot at gmail dot com)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
