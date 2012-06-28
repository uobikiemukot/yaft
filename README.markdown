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
その場合は手動でticコマンドでterminfoをinstallしてください．  
また，フォント(とaliasのファイル)をconf.hで設定した場所に忘れずに移動させてください．

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

## troubleshooting

### フォントがない！

~~~
$ ./yaft
/path/to/shnm.yaft
fopen: No such file or directory
~~~

フォントとaliasを定義するファイルが適切な場所にあるかを確認してください．

### グリフがない！

~~~
DEFAULT_CHAR(U+20) not found or invalid cell size x:0 y:0
~~~

フォントにU+20(SPACE)が存在しているか確認してください．  
SPACEのグリフが端末のセルサイズとして使われているのでないと起動できません．

### framebufferがない！

~~~
/dev/fb0
open: No such file or directory
~~~

/dev/fb0の作り方はディストリビューションごとのframebufferの利用法を調べてください．

### screen上で色がおかしい！
screenの仕様でTERMの値がrxvt/xtermでない場合にANSI Colorの明色(8 ~ 15番目)が正常に表示されません．

その場合，rxvt-256colorという名前のsymbolic linkでyaftのterminfoを指して，  
TERM=rxvt-256color screenと起動すれば上手く表示されるかもしれません．

~~~
$ export TERMINFO="$HOME/.terminfo/"
$ tic info/yaft.src
$ mkdir -p ~/.terminfo/r
$ ln -s ~/.terminfo/y/yaft ~/.terminfo/r/rxvt-256color
$ TERM=rxvt-256color screen
~~~

## font

### sample
sampleとして[shinonome font]，[mplus font]，それに[efont]を変換したyaft用のフォントを同封しています．  
自分用のフォントを生成したい場合はmisc/bdf2yaftを使ってください(additionalに説明があります)．

-	shnm-*.yaft  
	shinonome font(16dot)

-	mplus-*.yaft  
	mplus font(12dot)

-	ambiguous-half.yaft  
	efont(16dot)のうち，文字幅がambiguousのグリフ(半角)

-	ambiguous-half.alias
-	ambiguous-wide.alias  
	代用グリフの設定ファイル

各フォントのライセンスについてはlisence/以下のファイルを参照してください．

[shinonome font]: http://openlab.ring.gr.jp/efont/shinonome/
[mplus font]: http://mplus-fonts.sourceforge.jp/mplus-bitmap-fonts/
[efont]: http://openlab.ring.gr.jp/efont/

### format
BDFを簡略化したフォント形式を使っています．  
各グリフは以下の情報を持っており，それがグリフの数だけ並んでいます．

	code
	width height
	bitmap...

codeはUCS2のコードを10進で表記したものです．  
widthとheightはフォントの横幅と高さです(pixel)．  
bitmapにはBDFと同様にグリフのビットマップ情報が16進で列挙されます．  
(バイト境界よりもwidthが小さい場合にはBDFと同じくLSB側に0をパディングします．)

バウンディングボックスの指定がないので，  
ビットマップ情報としては常にwidth * height分の情報を記述しないといけません．

### glyph width
グリフの幅はUCS2/UCS4から求めるのではなく，以下のようになっています．

-	グリフがある場合: グリフを幅をそのまま使う
-	グリフがない場合: 幅は0

存在しないグリフが強制的に幅0になってしまうとまずいので，  
グリフが存在しない場合に代用として使うグリフを指定することができます．  
fonts/以下に2つのaliasの定義ファイルが含まれています．

-	fonts/ambiguous-half.alias
-	fonts/ambiguous-wide.alias

上記のファイルをconf.hのglyph_aliasに指定すると，  
グリフが存在しない場合，半角のグリフはSPACE(U+20)，  
全角のグリフはIDEOGRAPHIC SPACE(U+3000)に置き換えられます．

-halfと-wideはambiguous widthのグリフを全角にするか半角にするかで使いわけてください．  
(全角・半角の判定には[mk_wcwidth()]を使っています．)

[mk_wcwidth()]: http://www.cl.cam.ac.uk/~mgk25/ucs/wcwidth.c

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

## additional
補足のようなもの．

### sgr

~~~
ESC [ Ps ... m
~~~

文字の色や属性を指定するescape sequenceです．  
terminfoが読める場合には以下を見ればわかると思います．

~~~
setaf=\E[%?%p1%{8}%<%t3%p1%d%e%p1%{16}%<%t9%p1%{8}%-%d%e38;5;%p1%d%;m,
setab=\E[%?%p1%{8}%<%t4%p1%d%e%p1%{16}%<%t10%p1%{8}%-%d%e48;5;%p1%d%;m,
sgr=\E[0%?%p6%t;1%;%?%p2%t;4%;%?%p1%p3%|%t;7%;%?%p4%t;5%;m,
op=\E[39;49m,
sgr0=\E[m,
~~~

0 ~ 255までの色番号は以下のように指定します．  
これはxterm/rxvtと全く同じです．

-	0 ~ 7(ANSI Color)  
	ESC [ 3* m (前景色)  
	ESC [ 4* m (背景色)

	*の部分には色番号(0 ~ 7)がそのまま入ります

-	8 ~ 15(ANSI Colorを明るくした色)  
	ESC [ 9* m (前景色)  
	ESC [ 10* m (背景色)

	*の部分には色番号から8を引いた数(0 ~ 7)が入ります

-	16 ~ 255(256色拡張)  
	ESC [ 38;5;* m (前景色)  
	ESC [ 48;5;* m (背景色)

	*の部分には色番号(16 ~ 255)がそのまま入ります

-	前景/背景色のリセット  
	ESC [ 39 m (前景色)  
	ESC [ 49 m (背景色)

	文字属性はリセットされません

文字属性の指定は以下のようになっています．  
異なる文字属性は重複可能です．

-	0(Reset)  
	ESC [ m

	全ての文字属性をリセットします

-	1(Bold)  
	ESC [ 1 m

	前景色がAnsi Color(0 ~ 7)だった場合，その明色(8 ~ 17)に変更します

-	4(Underline)  
	ESC [ 4 m

	文字に下線を引きます

-	5(Blink)  
	ESC [ 5 m

	背景色がAnsi Color(0 ~ 7)だった場合，その明色(8 ~ 17)に変更します

-	7(Reverse)  
	ESC [ 7 m

	前景色と背景色を入れ変えます

色番号8 ~ 15はterminfoでは以下のものを使用するように指示しています．

	ESC [ 9* m (前景色)
	ESC [ 10* m (背景色)

これはBold/Blinkを指定しても同様の色になります．

	ESC [ 1;* m (前景色)
	ESC [ 5;* m (背景色)

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

1番目のようにテーブルを指定しないとUnicodeのBDFであると見なされます．  
(この場合，複数のBDFを指定することはできません．)  
また2番目のように複数のUnicodeのBDFをcatしてからbdf2yaftに渡すと，  
複数のBDFをmergeして1つのフォントを生成することができます．

## TODO

-	32bpp以外の環境でも動作するようにする
-	スクロールバックの実装
-	コピペの実装
-	BDFを直接読めるようにする
-	control sequence listをもうちょっと真面目に書く
-	ウィンドウサイズ・位置を変更するESCに対応する
-	sixelを実装する
-	yaft用のinput methodを作る
-	vttestの精度を上げる
-	screen/dwmのような機能を付ける

## license
MIT Licenseです．

Copyright (c) 2012 haru

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
