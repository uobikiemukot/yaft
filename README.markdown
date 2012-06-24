# yaft - yet another framebuffer terminal

framebufferを用いたvt102系のターミナルエミュレータです．

![yaft]

[yaft]: https://github.com/uobikiemukot/yaft/raw/master/img/yaft-blue.png

## download
- [latest](https://github.com/uobikiemukot/yaft/tarball/master)

## feature
+	framebuffer  
	consoleで動作します

+	UTF-8対応  
	というか他のエンコードが一切使えません  

+	256色  
	xtermと同様の256色指定のエスケープシーケンスに対応しています  

+	壁紙表示  
	起動直前のframebufferの内容を壁紙として取り込みます  

## configuration
コンパイル前にconf.hを編集して適切な設定に書き換えてください．

### path and terminal name

+	static char *font_path = "./fonts/shnm.yaft";  
	fontのpathを設定します

+	static char *fb_path = "/dev/fb0";  
	framebuffer deviceのpathを設定します

+	static char *shell_cmd = "/bin/bash";  
	端末から起動するshellを設定します

+	static char *term_name = "TERM=yaft-256color";  
	環境変数TERMの値を設定します

pathは絶対path(or 実行ディレクトリからの相対path)で記述します．

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

+	INTERVAL = 1000000,  
	pollingの間隔をマイクロ秒単位で設定できます

端末サイズやオフセットの値が不正だった場合，フルスクリーンでの起動を試みます．

## install

~~~
$ make
$ make install-font
$ sudo make install
~~~

gccとglibcがあればコンパイルできるはずです．

make installを使わなくても構いません．  
その場合は手動でticコマンドでterminfoをinstallしてください．  
また，フォントをconf.hで設定した場所に忘れずに移動させてください．

sampleとして[shinonome font]と[mplus font]を変換したyaft用のフォントを同封しています．  
各フォントのライセンスについてはlisence/以下のファイルを参照してください．

[shinonome font]: http://openlab.ring.gr.jp/efont/shinonome/
[mplus font]: http://mplus-fonts.sourceforge.jp/mplus-bitmap-fonts/

## usage
コマンドラインオプションは存在しません．

~~~
$ yaft
~~~

## troubleshooting

### フォントがない！

~~~
$ ./yaft
/path/to/shnm.yaft
fopen: No such file or directory
~~~

フォントが適切な場所にあるかを確認してください．

### グリフがない！

~~~
DEFAULT_CHAR(U+20) not found or invalid cell size x:0 y:0
~~~

フォントにU+20(SPACE)が存在しているか確認してください．  
SPACEのグリフが端末のセルサイズとして使われているのでないと起動できません．  

SPACEが存在しな場合にはALL 0のビットマップで良いので，  
以下のように手動でエントリを追加してみてください(8x16のフォントの場合)．

~~~
20
8 16
00
00
00
00
00
00
00
00
00
00
00
00
00
00
00
00
~~~

### framebufferがない！

~~~
/dev/fb0
open: No such file or directory
~~~

/dev/fb0がない場合，Linuxではgrubのkernelオプションにvga=773等と書くと良いかもしれません．  
(/dev/fb0の作り方はディストリビューションごとのframebufferの利用法を調べてください．)

通常，framebufferの書き込みにはvideo groupのメンバである必要があります．  
hogeというユーザをvideo groupに追加するには以下のようにします．  

~~~
$ sudo gpasswd -a hoge video
~~~

変更を反映させるには一度logoutをする必要があります．

### screen上で色がおかしい！
TERMの値がrxvt/xtermでない場合にANSIカラーの8 ~ 15が正常に表示されない場合があります．  
例えばrxvt-256colorという名前のsymbolic linkでyaftのterminfoを指して，  
TERM=rxvt-256color screenと起動すれば上手く表示されるかもしれません．

~~~
$ export TERMINFO="$HOME/.terminfo/"
$ tic info/yaft.src
$ mkdir -p ~/.terminfo/r
$ ln -s ~/.terminfo/y/yaft ~/.terminfo/r/rxvt-256color
$ TERM=rxvt-256color screen
~~~

## font
BDFを簡略化したフォント形式を使っています．  
各グリフは以下の情報を持っており，それがグリフの数だけ並んでいます．

	code
	width height
	bitmap...

codeはUCS2のコードを10進で表記したものです．  
widthとheightはフォントの横幅と高さです(pixel)．  
bitmapにはBDFと同様にグリフのビットマップ情報が列挙されます(16進)．  

バウンディングボックスの指定がないので，  
ビットマップ情報としては常にwidth * height分の情報を記述しないといけません．

### bdf2yaft
misc/bdf2yaft.cppというプログラムを用いると，  
等幅BDFをyaftで用いているフォント形式に変換できます．  

その際，変換テーブルを指定するとUnicode以外のBDFも変換できます．

~~~
$ cd misc
$ make
$ ./bdf2yaft TABLE BDF1 BDF2 ...
~~~

複数のフォントに同じグリフが存在する場合，  
後ろで指定したフォントのものが使われます．

変換テーブルの形式は変換元と変換先の文字コードを16進で列挙したものです．  
以下のような書式になっています．

-	ペアの区切りはタブでなければいけません
-	先頭が#の行はコメントと見なされます
-	3つめ以降のフィールドは無視されます

~~~
0x00	0x0000  # NULL
0x01	0x0001  # START OF HEADING
0x02	0x0002  # START OF TEXT
0x03	0x0003  # END OF TEXT
0x04	0x0004  # END OF TRANSMISSION
0x05	0x0005  # ENQUIRY
...
~~~

以下のような使い方もできます．

~~~
$ bdf2yaft BDF
$ cat BDF1 BDF2 ... | ./bdf2yaft
~~~

1番目のようにテーブルを指定しないと既にUnicodeのBDFであると見なされます．  
(この場合，複数のBDFを指定することはできません．)  
また2番目のように複数のUnicodeのBDFをcatしてからbdf2yaftに渡すと，  
複数のBDFをmergeして1つのフォントを生成することができます．

### yaftmerge

~~~
$ ./yaftmerge BDF1 BDF2 ...
~~~

yaftmergeは変換済みのフォントをmergeすることができます．  
bdf2yaftと同様に複数のフォントに同じグリフが存在する場合，  
後ろで指定したフォントのものが使われます．

## wallpaper
背景画像を表示したい場合にはconf.hのWALLPAPERをtrueにしてコンパイルした上で，  
yaft_wallを以下のように実行してください．

~~~
$ ./yaft_wall /path/to/wallpaper.jpg
~~~

yaftがpathの通っている場所にインストールされている必要があります．  
画像の表示には[fbv]を利用しています．  
framebuffer上で画像が表示できるプログラムなら他のものでも構いません．

[fbv]: http://www.eclis.ch/fbv/

## control sequence list
listにないコントロールシーケンスは無視されます．  
記載されているものでも，完全には対応していない場合があります．

### control character
0x00 - 0x1Fの範囲の制御コード

-	0x08 bs
-	0x09 tab
-	0x0A nl
-	0x0B nl
-	0x0C nl
-	0x0D cr
-	0x1B enter_esc

### escape sequence
ESC(0x1B)ではじまるシーケンスのうち，CSIでもOSCでもないもの

~~~
ESC *
~~~

-	7 save_state
-	8 restore_state
-	D nl
-	E crnl
-	H set_tabstop
-	M reverse_nl
-	Z identify
-	[ enter_csi
-	] enter_osc
-	c ris

### csi sequence

~~~
ESC [ *
~~~

-	@ insert_blank
-	A curs_up
-	B curs_down
-	C curs_forward
-	D curs_back
-	E curs_nl
-	F curs_pl
-	G curs_col
-	H curs_pos
-	J erase_display
-	K erase_line
-	L insert_line
-	M delete_line
-	P delete_char
-	X erase_char
-	a curs_forward
-	c identify
-	d curs_line
-	e curs_down
-	f curs_pos
-	g clear_tabstop
-	h set_mode
-	l reset_mode
-	m set_attr
-	n status_report
-	r set_margin
-	s save_state
-	u restore_state
-	` curs_col

## TODO

-	32bpp以外の環境でも動作するようにする
-	スクロールバックの実装
-	BDFを直接読めるようにする
-	control sequence listをもうちょっと真面目に書く

## license
MIT Licenseです．

Copyright (c) 2012 haru

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
