# yaft - yet another framebuffer terminal

linuxのframebufferを用いたvt102系ターミナルエミュレータです．

![yaft]

[yaft]: https://github.com/uobikiemukot/yaft/raw/master/img/yaft-blue.png

## download
- [latest](https://github.com/uobikiemukot/yaft/tarball/master)

## feature
+	framebuffer  
	consoleで動作します

+	UTF-8対応  
	※他の文字コードが一切使えません

+	256色  
	xtermと同様の256色指定のエスケープシーケンスに対応しています

+	壁紙表示  
	起動直前のframebufferの内容を壁紙として取り込むことができます

## configuration
コンパイル前にconf.hを編集して適切な設定に書き換えてください．

### path and terminal name

+	static char *font_path[] = { ".fonts/shnm-jis0208.yaft", ".fonts/shnm-jis0201.yaft", ".fonts/shnm-iso8859.yaft", NULL, };  
	fontを指定します

+	static char *glyph_alias = ".fonts/ambiguous-wide.alias";  
	グリフのaliasを定義するファイルのpathです

+	static char *fb_path = "/dev/fb0";  
	framebuffer deviceのpathを設定します

+	static char *shell_cmd = "/bin/bash";  
	端末から起動するshellを設定します

+	static char *term_name = "TERM=yaft-256color";  
	環境変数TERMの値を設定します

pathは絶対path(or 実行ディレクトリからの相対path)で記述します．  

fontは複数指定可能で，後ろに指定したフォントのグリフが優先的に使われます．  
font_pathの配列の最後にはNULLを入れないといけません．

glyph_aliasを使わない場合にはNULLを指定します．  
(特に良いことはありません．)

### color

+	DEFAULT_FG = 7,  
	デフォルトの前景色

+	DEFAULT_BG = 0,  
	デフォルトの背景色

+	CURSOR_COLOR = 2,  
	カーソルの色の設定

色はcolor.hで定義されているcoloro_palette[]のインデックスを指定します．

### misc
+	DUMP = false,  
	端末に送られてきたデータを標準出力にdumpします(デバッグ用)

+	DEBUG = false,  
	parseの結果を標準エラー出力に表示します(デバッグ用)

+	LAZYDRAW = true,  
	描画をサボるかどうか．見かけ上の描画速度が向上します

+	WALLPAPER = false,  
	直前のframebufferの内容を壁紙として取り込むかどうか

+	OFFSET_X = 0,  
	画面内のどこに端末を表示するかのオフセット値

+	OFFSET_Y = 0,  
	同上

+	TERM_WIDTH = 0,  
	端末のサイズ．0を指定すると画面のサイズを使用します

+	TERM_HEIGHT = 0,  
	同上

+	TABSTOP = 8,  
	ハードウェアタブの幅

+	SELECT_TIMEOUT = 1000000,  
	select()のタイムアウトの指定

端末サイズやオフセットの値が不正だった場合，全画面の起動を試みます．

## install

~~~
$ make
$ make install-font
$ sudo make install
~~~

外部ライブラリは必要ありません．  
gccとglibcがあればコンパイルできるはずです．

make installを使わなくても構いません．  
その場合は以下のコマンドでterminfoをinstallしてください．

~~~
$ tic info/yaft.src
~~~

また，フォント(とaliasの定義ファイル)をconf.hで設定した場所に忘れずに移動させてください．

## usage

~~~
$ yaft
~~~

コマンドラインオプションは存在しません．

~~~
$ yaft_wall /path/to/wallpaper.jpg
~~~

conf.hのWALLPAPERをtrueにしてcompileした上で，  
上記のように起動すると壁紙を使うことができます．

[fbv]とyaftがpathの通っているところにインストールされている必要があります．

[fbv]: http://www.eclis.ch/fbv/

## license
MIT Licenseです．

Copyright (c) 2012 haru

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
