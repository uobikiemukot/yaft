# yaft - yet another framebuffer terminal

framebufferを用いたターミナルエミュレータです．

vt102やLinux consoleを参考に作っていますが，完全な互換性はありません．  
詳細はinfo/yaft.infoや，後述するcontrol sequence listを参照してください．

![yaft]

[yaft]: https://github.com/uobikiemukot/yaft/raw/27ba1df66490b921636de13ef354149a640e9dd7/yaft.png

## feature
+	UTF-8対応
	というか他のエンコードが一切使えません．Unicode BMPの範囲のグリフを表示可能です(フォントに依存)

+	East Asian Width
	ビットマップの幅を参照するので端末が文字幅を誤認することはありません

+	256色:
	xtermと同様の256色指定のエスケープシーケンスに対応しています．  
	また，OSC 4とOSC 104も使用できます

+	壁紙表示:
	pnm形式のファイルを用いて端末の背景に画像を表示することができます(後述)

## configuration
コンパイル前にconf.hを編集して適切な設定に書き換えてください．

### path and terminal name

+	static char *wall_path = "~/.yaft/karmic-gray.ppm";
	壁紙のpathを指定します．無効にする場合はNULLにしてください

+	static char *font_path = "~/.yaft/milk.yaft";
	fontのpathを設定します．フォントの形式は後述します

+	static char *fb_path = "/dev/fb0";
	framebuffer deviceのpathを設定します．通常はこのままで問題ありません

+	static char *shell_cmd = "/bin/bash";
	端末から起動するshellを設定します

+	static char *term_name = "yaft";
	環境変数TERMの値を設定します．yaft以外にしても良いことはないと思います

pathは全て絶対pathで記述します．  
wall_pathとfont_pathの指定では，$HOMEのパスを省略して~と書くことができます．

### color
色は0xFFFFFF形式(RGBの順で各8bitずつ)かcolor.hでdefineされている色の名前を使用できます．

+	DEFAULT_FG = GRAY,
	デフォルトの前景色

+	DEFAULT_BG = BLACK,
	デフォルトの背景色

+	CURSOR_COLOR = GREEN,
	カーソルの色の設定

ANSIの色設定を変えたい場合はcolor.hの最初のほうの定義を書き換えてください．

### misc
+	DUMP false,
	端末に送られてきたデータを標準出力にdumpします(デバッグ用)

+	DEBUG = false,
	parseの結果を標準エラー出力に表示します(デバッグ用)

+	LAZYDRAW = true,
	描画をサボるかどうか．見掛け上の描画速度が向上します

+	OFFSET_X = 0,
	画面内のどこに端末を表示するかのオフセット値

+	OFFSET_Y = 0,
	同上

+	TERM_WIDTH = 1280,
	端末のサイズ．通常は画面のサイズと同じにします

+	TERM_HEIGHT = 1024,
	同上

+	TABSTOP = 8,
	ハードウェアタブの幅

+	INTERVAL = 1000000,
	pollingの間隔をマイクロ秒単位で設定できます

画面よりも端末のサイズを大きくすることはできません．

## install

1.	conf.hを環境に合わせて修正
2.	makeして実行ファイルを生成
3.	ticコマンドでterminfoをコンパイル
4.	font/wallpaperをconf.hで指定した場所に移動

~~~
$ tic info/yaft.info
$ make
$ sudo make install
~~~
sampleとして[efont]のb16.bdfを変換したフォントと，  
xubuntu/karmicの[wallpaper]をpnmに変換したものを同封しています．

efontのライセンスについてはlisence/efont以下のファイルを参照してください．  
wallpaperのライセンスは[Creative Commons Attribution-ShareAlike 3.0 License]です．

[efont]: http://openlab.ring.gr.jp/efont/unicode/
[wallpaper]: https://wiki.ubuntu.com/Xubuntu/Artwork/Karmic?action=AttachFile&do=view&target=karmic-wallpaper-final.tar.gz
[Creative Commons Attribution-ShareAlike 3.0 License]: http://creativecommons.org/licenses/by-sa/3.0/

## usage

~~~
$ yaft
~~~

コマンドラインオプションは存在しません．

### font
BDFを簡略化したフォント形式を使っています．  
各グリフは以下の情報を持っています．

	code
	width height
	bitmap...

codeはUCS2のコードを10進で表記したものです．  
widthとheightはフォントの横幅と高さです(pixel)．

bitmapにはBDFと同様にフォントのビットマップ情報が列挙されます(16進)．  
BDFと異なり，一行には1バイト分のビットマップ情報しか書けません．  
バイト境界に合わない場合はMSB側に0をパディングします．

また，バウンディングボックスの指定がないので，  
ビットマップ情報としては常にwidth * height分の情報を記述しないといけません．

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

~~~
0x00	0x0000  # NULL
0x01	0x0001  # START OF HEADING
0x02	0x0002  # START OF TEXT
0x03	0x0003  # END OF TEXT
0x04	0x0004  # END OF TRANSMISSION
0x05	0x0005  # ENQUIRY
...
~~~

ペアの区切りはタブでなければいけません．  
先頭が#の行はコメントと見なされます．  
3つめ以降のフィールドは無視されます．

~~~
$ bdf2yaft BDF
$ cat BDF1 BDF2 ... | ./bdf2yaft
~~~

既にUnicodeのBDFはテーブルを指定する必要はありません．  
また2番目のように複数のUnicodeのBDFをcatしてからbdf2yaftに渡すと，  
複数のBDFをmergeして1つのフォントを生成することができます．

## wallpaper
[pnm]に含まれるportable pixmap format(P6)の画像ファイルを背景画像として指定できます．

~~~
$ head -3 wall.ppm
P6
1920 1440
255
~~~

このようにファイルの先頭3行を表示したとき，  
1行目がP6，3行目が255であることを確認してください．

imagemagick等を使って画像を変換し，  
conf.hのwall_pathを設定することで画像が表示されます．

~~~
$ convert wall.png wall.ppm
$ grep wall_path conf.h
static char *wall_path = /path/to/wall.ppm;

~~~

[pnm]: http://ja.wikipedia.org/wiki/PNM_(%E7%94%BB%E5%83%8F%E3%83%95%E3%82%A9%E3%83%BC%E3%83%9E%E3%83%83%E3%83%88)

## control sequence list
listにないコントロールシーケンスは無視されます．  
記載されているものでも，完全には対応していない場合もあります．

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

### osc sequence

~~~
ESC ] *
~~~

-	4   set_palette
-	104 reset_palette

## TODO

-	32bpp以外の環境で動作するようにする
-	スクロールバックの実装
-	BDFを直接読めるようにする
-	pnmの他の形式も読めるようにする ( or fbtermのように壁紙を指定できるようにする)
-	control sequence listをもうちょっと真面目に書く
