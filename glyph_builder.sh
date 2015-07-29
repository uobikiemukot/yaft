#!/bin/bash
# desc: bash script for creating yaft's glyph.h

# settings
YAFT_DIR=`pwd`
WORK_DIR=/tmp/glyph_builder
ALIAS_FILE=alias

# infomation of each fonts
# mplus
MPLUS_VERSION=2.2.4
MPLUS_NAME=mplus_bitmap_fonts-${MPLUS_VERSION}
MPLUS_FILE=${MPLUS_NAME}.tar.gz
MPLUS_URL=http://iij.dl.osdn.jp/mplus-fonts/5030/${MPLUS_FILE}

# efont
EFONT_VERSION=0.4.2
EFONT_NAME=efont-unicode-bdf-${EFONT_VERSION}
EFONT_FILE=${EFONT_NAME}.tar.bz2
EFONT_URL=http://openlab.ring.gr.jp/efont/dist/unicode-bdf/${EFONT_FILE}

# milkjf
# already included in yaft

# unifont
UNIFONT_VERSION=8.0.01
UNIFONT_NAME=unifont-${UNIFONT_VERSION}
UNIFONT_FILE=${UNIFONT_NAME}.bdf.gz
UNIFONT_URL=http://unifoundry.com/pub/${UNIFONT_NAME}/font-builds/${UNIFONT_FILE}

# dina font
DINA_VERSION=
DINA_NAME=Dina
DINA_FILE=${DINA_NAME}.zip
DINA_URL=http://www.donationcoder.com/Software/Jibz/${DINA_NAME}/downloads/${DINA_FILE}

# terminus font
TERMINUS_VERSION=4.39
TERMINUS_NAME=terminus-font-${TERMINUS_VERSION}
TERMINUS_FILE=${TERMINUS_NAME}.tar.gz
TERMINUS_URL=http://sourceforge.net/projects/terminus-font/files/${TERMINUS_NAME}/${TERMINUS_FILE}

# profont (pcf format)
PROFONT_VERSION=
PROFONT_NAME=profont-x11
PROFONT_FILE=${PROFONT_NAME}.zip
PROFONT_URL=http://tobiasjung.name/downloadfile.php?file=${PROFONT_FILE}

# tamsyn font (pcf format)
TAMSYN_VERSION=1.11
TAMSYN_NAME=tamsyn-font-${TAMSYN_VERSION}
TAMSYN_FILE=${TAMSYN_NAME}.tar.gz
TAMSYN_URL=http://www.fial.com/~scott/tamsyn-font/download/${TAMSYN_FILE}

# wqy-bitmapfont (mkfont_bdf cannot handle this font...)
WQY_VERSION=1.0.0-RC1
WQY_NAME=wqy-bitmapsong-bdf-${WQY_VERSION}
WQY_FILE=${WQY_NAME}.tar.gz
WQY_URL=http://jaist.dl.sourceforge.net/project/wqy/wqy-bitmapfont/${WQY_VERSION}/${WQY_FILE}

# misc functions
usage()
{
	echo -e "usage: ./glyph_builder.sh FONTS VARIATIONS"
	echo -e "\tavalable fonts: mplus, efont, milkjf, unifont, dina, terminus, profont, tamsyn"
	echo -e "depends on: wget, pcf2bdf"
}

generate()
{
	echo "./mkfont_bdf ./table/${ALIAS_FILE} ${@}  > ${YAFT_DIR}/glyph.h"
	./mkfont_bdf ./table/${ALIAS_FILE} ${@} > ${YAFT_DIR}/glyph.h
}

# each font generate function
mplus_create_bdf()
{
	# Install M+ BITMAP FONTS E
	echo -e "\nInstall M+ BITMAP FONTS E (iso8859-1)..."
	cd fonts_e

	awk '/^SWIDTH/{$2 += 80} /^DWIDTH/{$2 += 1} {print}' mplus_h12r.bdf \
	| sed 's/hlv/hlvw/'> mplus_h12rw.bdf
	cd -

	# Install M+ BITMAP FONTS EURO
	echo -e "\nInstall M+ BITMAP FONTS EURO (iso8859-15)..."
	cd fonts_e/euro
	for f in mplus_*.diff
	do
		b=`basename $f .diff`
		echo "$f $b"
		cp ../$b.bdf ./
		patch $b.bdf $b.diff
		mv $b.bdf $b-euro.bdf
	done

	awk '/^SWIDTH/{$2 += 80} /^DWIDTH/{$2 += 1} {print}' \
	mplus_h12r-euro.bdf | sed 's/hlv/hlvw/' > mplus_h12rw-euro.bdf
	cd -

	echo -e "\nInstall M+ BITMAP FONTS J..."
	cd fonts_j

	echo "create: mplus_j10b.bdf"
	echo -n "wait a minute..."
	../mkbold -r -R mplus_j10r.bdf | sed 's/medium/bold/' \
	> mplus_j10b.bdf &&
	echo " done"

	echo "create: mplus_j12b.bdf"
	echo -n "wait a minute..."
	../mkbold -r -R mplus_j12r.bdf | sed 's/medium/bold/' \
	> mplus_j12b.bdf &&
	echo " done"

	cp mplus_j10r-iso-W4 mplus_j10r-iso.bdf
	echo "select: mplus_j10r-iso.bdf [W5]"
	patch mplus_j10r-iso.bdf mplus_j10r-iso-W5.diff
	#echo "select: mplus_j10r-iso.bdf [W4]"

	for f in mplus_[!j]*-jisx0201.diff
	do
		b=`basename $f -jisx0201.diff`
		echo "create: $b-jisx0201.bdf"
		cp ../fonts_e/$b.bdf ./
		patch $b.bdf $b-jisx0201.diff
		mv $b.bdf $b-jisx0201.bdf
	done

	for f in mplus_j*-jisx0201.diff
	do
		b=`basename $f -jisx0201.diff`
		echo "create: $b-jisx0201.bdf"
		cp $b-iso.bdf $b-jisx0201.bdf
		patch $b-jisx0201.bdf $b-jisx0201.diff
	done

	# some bug fix
	sed -i 's|STARTCHAR 0x2455n|STARTCHAR 0x2455|' mplus_j12r.bdf
	sed -i 's|STARTCHAR 0x2455n|STARTCHAR 0x2455|' mplus_j12b.bdf

	cd -

	touch created
}

mplus()
{
	echo -ne "creating glyph.h from mplus fonts...\n"
	wget -q -nc ${MPLUS_URL}

	if test ! -d ${MPLUS_NAME}; then
		bsdtar xf ${MPLUS_FILE}
	fi

	cd ${MPLUS_NAME}
	ln -sf ${YAFT_DIR}/mkfont_bdf .
	ln -sf ${YAFT_DIR}/table .

	if test ! -f created; then
		mplus_create_bdf > /dev/null 2>&1
	fi

	case "$1" in
	j10*)
		FILES=`find fonts_j -type f -name "*${1}*.bdf" | tr "\n" " "`;;
	j12*)
		PATTERN=`echo -n $1 | tail -c3`
		FILES=`find . -type f -name "*[fj]${PATTERN}*.bdf" | tr "\n" " "`;;
	f10*|f12*)
		FILES=`find fonts_e -type f -name "*${1}*.bdf" | tr "\n" " "`;;
	*)
		echo -ne "avalable font variations: [fj](10|12)[rb]\n"
		exit -1;;
	esac

	generate ${FILES}
}

efont()
{
	echo -ne "creating glyph.h from efont unicode...\n"
	wget -q -nc ${EFONT_URL}

	if test ! -d ${EFONT_NAME}; then
		bsdtar xf ${EFONT_FILE}
	fi

	cd ${EFONT_NAME}
	ln -sf ${YAFT_DIR}/mkfont_bdf .
	ln -sf ${YAFT_DIR}/table .

	case "$1" in
	1[0246]*|24*)
		FILES=`find . -type f -name "*${1}.bdf" | tr "\n" " "`;;
	*)
		echo -ne "avalable font variations: (10|12|14|16|24)(_b)?\n"
		exit -1;;
	esac

	generate ${FILES}
}

milkjf()
{
	echo -ne "creating glyph.h from milkjf font...\n"

	ln -sf $YAFT_DIR/mkfont_bdf .
	ln -sf $YAFT_DIR/table .
	ln -sf $YAFT_DIR/fonts .

	FILES=`find -L fonts -type f -name "*.bdf" | tr "\n" " "`

	generate ${FILES}
}

unifont()
{
	echo -ne "creating glyph.h from unifont...\n"
	wget -q -nc ${UNIFONT_URL}

	if test ! -f ${UNIFONT_NAME}.bdf; then
		gunzip ${UNIFONT_FILE}
	fi

	ln -sf $YAFT_DIR/mkfont_bdf .
	ln -sf $YAFT_DIR/table .

	FILES="${UNIFONT_NAME}.bdf"

	generate ${FILES}
}

dina()
{
	echo -ne "creating glyph.h from dina font...\n"
	wget -q -nc ${DINA_URL}

	if test ! -d BDF; then
		bsdtar xf ${DINA_FILE}
	fi

	ln -sf $YAFT_DIR/mkfont_bdf .
	ln -sf $YAFT_DIR/table .

	case "$1" in
	r*)
		FILES=`find BDF -type f -name "*${1}.bdf" | tr "\n" " "`;;
	*)
		echo -ne "avalable font variations: r400-(6|8|9|10), r700-(8|9|10)\n"
		exit -1;;
	esac

	generate ${FILES}
}

terminus()
{
	echo -ne "creating glyph.h from terminus font...\n"
	wget -q -nc ${TERMINUS_URL}

	if test ! -d ${TERMINUS_NAME}; then
		bsdtar xf ${TERMINUS_FILE}
	fi

	cd ${TERMINUS_NAME}
	ln -sf $YAFT_DIR/mkfont_bdf .
	ln -sf $YAFT_DIR/table .

	case "$1" in
	u*)
		FILES=`find . -type f -name "*${1}.bdf" | tr "\n" " "`;;
	*)
		echo -ne "avalable font variations: u(14|16)v, u(12|14|16|18|20|22|24|28|32)[nb]\n"
		exit -1;;
	esac

	generate ${FILES}
}

profont()
{
	echo -ne "creating glyph.h from profont...\n"
	wget -q -nc ${PROFONT_URL} -O ${PROFONT_FILE}

	if test ! -d ${PROFONT_NAME}; then
		bsdtar xf ${PROFONT_FILE}
	fi

	cd ${PROFONT_NAME}
	ln -sf $YAFT_DIR/mkfont_bdf .
	ln -sf $YAFT_DIR/table .

	if test ! -f created; then
		for i in *.pcf; do
			pcf2bdf -o `basename $i .pcf`.bdf $i 
		done
		touch created
	fi

	case "$1" in
	1*|2*)
		FILES=`find . -type f -name "*${1}.bdf" | tr "\n" " "`;;
	*)
		echo -ne "avalable font variations: (10|11|12|15|17|22|29)\n"
		exit -1;;
	esac

	generate ${FILES}
}

tamsyn()
{
	echo -ne "creating glyph.h from tamsyn font...\n"
	wget -q -nc ${TAMSYN_URL}

	if test ! -d ${TAMSYN_NAME}; then
		bsdtar xf ${TAMSYN_FILE}
	fi

	cd ${TAMSYN_NAME}
	ln -sf $YAFT_DIR/mkfont_bdf .
	ln -sf $YAFT_DIR/table .

	if test ! -f created; then
		for i in *.pcf; do
			pcf2bdf -o `basename $i .pcf`.bdf $i 
		done
		touch created
	fi

	case "$1" in
	*[rb])
		FILES=`find . -type f -name "*${1}.bdf" | tr "\n" " "`;;
	*)
		echo -ne "avalable font variations: (5x9|6x12|7x13|7x14|8x15|8x16|10x20)[rb]\n"
		exit -1;;
	esac

	generate ${FILES}
}

wqy()
{
	echo -ne "creating glyph.h from tamsyn font...\n"
	wget -q -nc ${WQY_URL}

	if test ! -d ${WQY_NAME}; then
		bsdtar xf ${WQY_FILE}
	fi

	cd wqy-bitmapsong
	ln -sf $YAFT_DIR/mkfont_bdf .
	ln -sf $YAFT_DIR/table .

	case "$1" in
	*pt|*px)
		FILES=`find . -type f -name "*${1}.bdf" | tr "\n" " "`;;
	*)
		echo -ne "avalable font variations: (9|10|11|12)pt, 13px\n"
		exit -1;;
	esac

	generate ${FILES}
}

# main
echo "LANG: $LANG"
mkdir -p $WORK_DIR
cd $WORK_DIR

case "$1" in 
"mplus")
	shift
	mplus $1;;
"efont")
	shift
	efont $1;;
"milkjf")
	milkjf;;
"unifont")
	unifont;;
"dina")
	shift
	dina $1;;
"terminus")
	shift
	terminus $1;;
"profont")
	shift
	profont $1;;
"tamsyn")
	shift
	tamsyn $1;;
*)
	usage;;
esac
