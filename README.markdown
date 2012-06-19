# yaft - yet another framebuffer terminal

framebufferを用いたターミナルエミュレータです．

vt102やLinux consoleを参考に作っていますが，完全な互換性はありません．

## feature
-	UTF-8対応:
	というか他のエンコードが一切使えません．Unicode BMPの範囲のグリフを表示可能です(フォントに依存)
-	East Asian Width:
	ビットマップの幅を参照するので文字幅のズレは発生しません
-	256色:
	xtermと同様の256色指定のエスケープシーケンスに対応しています．また，OSC 4とOSC 104も使えます
-	壁紙表示:
	pnm形式のファイルを用いて端末の背景に画像を表示することができます(後述)

## configuration
conf.hに設定可能な変数が列挙されています．

### path

-	static char *wall_path = NULL:
	壁紙のパスを指定します(絶対パス)．形式はpnmのみ．無効にする場合はNULLにしてください
-	static char *font_path = "~/fonts/efont.yaft":
	fontのパスを設定します(絶対パス)．$HOMEの変わりに~が使えます．フォントの形式は後述します
-	static char *fb_path = "/dev/fb0":
	framebuffer deviceのパスを設定します．通常はこのままで問題ありません
-	static char *shell_cmd = "/bin/bash":
	端末から起動するshellを設定します

### color
色は0xFFFFFF形式(RGBの順で各8bitずつ)かcolor.hでdefineされている色の名前を使用できます．

-	DEFAULT_FG = GRAY:
	デフォルトの前景色
-	DEFAULT_BG = BLACK:
	デフォルトの背景色
-	CURSOR_COLOR = GREEN:
	カーソルの色の設定

### misc
-	DUMP = false:
	端末に送られてきたデータを標準出力にdumpします(デバッグ用)
-	DEBUG = false:
	parseの結果を標準エラー出力に表示します(デバッグ用)
-	LAZYDRAW = true:
	描画をサボるかどうか．見掛け上の描画速度が向上します
-	OFFSET_X = 0:
	画面内のどこに端末を表示するかのオフセット値
-	OFFSET_Y = 0:
	同上
-	TERM_WIDTH = 1280:
	端末のサイズ．通常は画面のサイズと同じにします
-	TERM_HEIGHT = 1024:
	同上
-	TABSTOP = 8:
	ハードウェアタブの幅
-	INTERVAL = 1000000:
	pollingの間隔をマイクロ秒単位で設定できます

## install
conf.hを環境に合わせて修正した後，
makeをして実行ファイルをパスの通っている場所に置いてください．

フォントや画像ファイルをconf.hで指定した場所に移動させるのも忘れずに．

~~~
$ make
$ cp fonts/efont.yaft ~/.fonts/
$ cp yaft /usr/local/bin/
~~~

## usage

~~~
$ yaft
~~~

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
$ bdf2yaft TABLE BDF1 BDF2 ...
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
また2番目のように複数のUnicodeのBDFをcatしてからbdf2yaftに渡すと
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

## TODO
