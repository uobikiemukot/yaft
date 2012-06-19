# yaft - yet another framebuffer terminal

X環境なしで使うことができるコンソール用のターミナルエミュレータです．
豊富な機能はありません．その辺を割り切って使える人向けです．

vt102やLinux consoleをベースにしていますが，完全な互換性はありません．
例えば代替文字セットの使用は今のところできません．
一方でxtermと互換の256色の指定やパレットの変更などは可能です．

## feature
-	UTF-8対応
	というか他のエンコードが一切使えません．Unicode BMPの範囲のグリフを表示可能です(フォントに依存)．
-	East Asian Widthへの対処
	常にビットマップの幅を参照するので文字幅のズレは発生しません．
-	256色
	xtermと同様の256色指定のエスケープシーケンスに対応しています．また，OSC 4とOSC 104も使えます．
-	壁紙表示
	ppm形式のファイルを用いて端末の背景に画像を表示することができます．

## configuration
conf.hに設定可能な変数が列挙されています．

### path

static char *wall_path = NULL;
:	壁紙のパスを指定します(絶対パス)．形式はppmのみ．無効にする場合はNULLにしてください
static char *font_path = "./fonts/test.yaft";
:	fontのパスを設定します(絶対パス)．フォントの形式は後述します
static char *fb_path = "/dev/fb0";
:	framebuffer deviceのパスを設定します．通常はこのままで問題ありません
static char *shell_cmd = "/bin/bash";
:	端末から起動するshellの設定

### color
色は0xFFFFFF形式(RGBの順で各8bitずつ)かcolor.hでdefineされている色の名前を使用できます．

DEFAULT_FG = GRAY,
:	デフォルトの前景色
DEFAULT_BG = BLACK,
:	デフォルトの背景色
CURSOR_COLOR = GREEN,
:	カーソルの色の設定

### misc
DUMP = false,
:	端末に送られてきたデータを標準出力にdumpします(デバッグ用)
DEBUG = false,
:	parseの結果を標準エラー出力に表示します(デバッグ用)
LAZYDRAW = true,
:	描画をサボるかどうか．見掛け上の描画速度が向上します
OFFSET_X = 0,
:	画面内のどこに端末を表示するかのオフセット値
OFFSET_Y = 0,
:	同上
TERM_WIDTH = 1280,
:	端末のサイズ．通常は画面のサイズと同じにします
TERM_HEIGHT = 1024,
:	同上
TABSTOP = 8,
:	ハードウェアタブの幅
INTERVAL = 1000000, /* polling interval(usec) */
:	pollingの間隔をマイクロ秒単位で設定できます

### font

## install
外部ライブラリは未使用です．
ある程度新しいgcc/g++があればコンパイルできると思います．

	開発は以下のバージョンのgccを使っています．
	gcc (GCC) 4.7.0 20120505 (prerelease)
	g++ (GCC) 4.7.0 20120505 (prerelease)

conf.hを環境に合わせて修正した後，
makeをして実行ファイルをパスの通っている場所に置いてください．

~~~
$ make
$ cp yaft /usr/local/bin/
~~~

## usage

~~~
$ yaft
~~~

## TODO
