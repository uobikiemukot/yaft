/****************************************************************************
 *
 * Copyright(c) 2012-2012, John Forkosh Associates, Inc. All rights reserved.
 *           http://www.forkosh.com   mailto: john@forkosh.com
 * --------------------------------------------------------------------------
 * This file is part of gifsave89, which is free software.
 * You may redistribute and/or modify gifsave89 under the terms of the
 * GNU General Public License, version 3 or later, as published by the
 * Free Software Foundation.
 *      gifsave89 is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY, not even the implied warranty of MERCHANTABILITY.
 * See the GNU General Public License for specific details.
 *      By using gifsave89, you warrant that you have read, understood
 * and agreed to these terms and conditions, and that you possess the legal
 * right and ability to enter into this agreement and to use gifsave89
 * in accordance with it.
 *      Your gifsave89.zip distribution file should contain the file
 * COPYING, an ascii text copy of the GNU General Public License,
 * version 3. If not, point your browser to  http://www.gnu.org/licenses/
 * or write to the Free Software Foundation, Inc., 59 Temple Place,
 * Suite 330,  Boston, MA 02111-1307 USA.
 * --------------------------------------------------------------------------
 *
 * Purpose:   o	gifsave89 generates gif images in memory.
 *		It's based on the original gifsave
 *		by Sverre H. Huseby,
 *		  http://shh.thathost.com/pub-unix/#gifsave
 *		Also see
 *		  http://www.forkosh.com/gifsave89.html
 *		for further details about this version.
 *
 * Source:    o	gifsave89.c
 *
 * Functions: o	The following "table of contents" lists each function
 *		comprising gifsave89 in the order it appears in this file.
 *		See individual function entry points for specific comments
 *		about purpose, calling sequence, side effects, etc.
 *		=============================================================
 *		+---
 *		| user-callable entry points (a complete example is shown
 *		| under the newgif() Notes section below, and also see
 *		| main() test driver for more detailed comments and examples)
 *		+------------------------------------------------------------
 *		void  (*gs=)    *newgif(gifimage,width,height,colors,bgindex)
 *		  animategif(gs,nrepetitions,delay,tcolor,disposal)*optional*
 *		 plaintxtgif(gs,left,top,width,height,fg,bg,data)  *optional*
 *		  controlgif(gs,tcolor,delay,userinput,disposal)   *optional*
 *		putgif(gs,pixels) or                             *short form*
 *		fputgif(gs,left,top,width,height,pixels,colors)   *long form*
 *		int  nbytes_in_gifimage = endgif(gs)
 *		  --- alternative "short form" for a single image gif ---
 *		void  (*gifimage=)  *makegif(&nbytes_in_gifimage,
 *		                          width,height,pixels,colors,bgindex)
 *		+---
 *		| gif helper functions
 *		+------------------------------------------------------------
 *		newgifstruct(gifimage,width,height) malloc and init GS struct
 *		debuggif(dblevel,dbfile)          set static msglevel,msgfile
 *		fprintpixels(gs,format,pixels)           ascii dump of pixels
 *		plainmimetext(expression,width,height)   pixel image for expr
 *		overlay(pix1,w1,h1,pix2,w2,h2,col1,row1,bg,fg) pix2 onto pix1
 *		+---
 *		| write gif colortable and (help write) other gif blocks
 *		+------------------------------------------------------------
 *		putgifcolortable(gs,colors)              write gif colortable
 *		putgifapplication(gs,ga)  write gif application extension hdr
 *		+---
 *		| lzw encoder
 *		+------------------------------------------------------------
 *		encodelzw(gs,codesize,nbytes,data)  lzw-encode nbytes of data
 *		clearlzw(gs,codesize)                 clear lzw string tables
 *		putlzw(gs,index,byte)  add prefix_index+byte to string tables
 *		getlzw(gs,index,byte) find prefix_index+byte in string tables
 *		+---
 *		| low-level routines to store image data in memory buffer
 *		+------------------------------------------------------------
 *		putblkbytes(bk,bytes,nbytes)   nbytes from bytes to bk buffer
 *		putblkbyte(bk,byte)                  single byte to bk buffer
 *		putblkword(bk,word)two bytes from word in little-endian order
 *		putsubblock(sb,bits,nbits)     nbits from bits to sb subblock
 *		putsubbytes(sb,bytes,nbytes) nbytes from bytes to sb subblock
 *		flushsubblock(sb)  write subblock to block, preceded by count
 *		+---
 *		| user utility functions
 *		+------------------------------------------------------------
 *		gifwidth(gs)                                returns gs->width
 *		gifheight(gs)                              returns gs->height
 *		pixgraph(ncols,nrows,f,n)    generate pixels of function f[n]
 *		+---
 *		| test driver (compile with -DGSTESTDRIVE)
 *		+------------------------------------------------------------
 *		#if defined(GSTESTDRIVE)
 *		 main(argc,argv)                                  test driver
 *		 f(fparam,n)           sample waveform to be graphed by tests
 *		 writefile(buffer,nbytes,file)     nbytes from buffer to file
 *		#endif
 *
 * --------------------------------------------------------------------------
 *
 * Notes:     o	gifsave89 is based on the original gifsave
 *		by Sverre H. Huseby,
 *		  http://shh.thathost.com/pub-unix/#gifsave
 *		Also see
 *		  http://www.forkosh.com/gifsave89.html
 *		for further details about this version.
 *		Additional information about the GIF89a file format
 *		was gathered from
 *		  http://www.matthewflickinger.com/lab/whatsinagif/
 *		  http://www.fileformat.info/format/gif/egff.htm
 *		  http://www.w3.org/Graphics/GIF/spec-gif89a.txt
 *		(several discrepancies between spec-gif89a.txt and
 *		egff.htm are pointed out and resolved below).
 *		If you prefer printed books, see Chapter 26 in
 *		  The File Formats Handbook, Gunter Born,
 *		  ITP Publishing 1995, ISBN 1-850-32128-0.
 *		From its publication date, you can see it's old,
 *		but its gif format treatment remains unchanged.
 *	      o	See individual function entry points for specific comments
 *		concerning the purpose, calling sequence, side effects, etc
 *		of each gifsave89 function listed above.
 *	      o	Compile as
 *		  cc -DGSTESTDRIVE gifsave89.c -lm -o gifsave89
 *		for an executable image with test driver, or as
 *		  cc yourprogram.c gifsave89.c -o yourprogram
 *		for your own usage.
 *	      o	See Notes: section under main() function for
 *		further usage and test driver instructions.
 *
 * --------------------------------------------------------------------------
 * Revision History:
 * 09/11/12	J.Forkosh	Installation.
 * 10/10/12	J.Forkosh	Most recent revision (also see REVISIONDATE)
 * See  http://www.forkosh.com/gifsave89changelog.html  for further details.
 *
 ***************************************************************************/

/* -------------------------------------------------------------------------
standard headers...
-------------------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#if defined(GSTESTDRIVE)
  /* --- only needed for main() test driver below --- */
#include <math.h>
#endif

/* -------------------------------------------------------------------------
Program id and messages...
-------------------------------------------------------------------------- */
/* --- program id --- */
#define	PROGRAMID     "gifsave89"	/* program name */
#define	VERSION       "1.00"	/* version number */
#define REVISIONDATE  "  10 Oct 2012  "	/* date of most recent revision */
#define COPYRIGHTTEXT "Copyright(c) 2012-2012, John Forkosh Associates, Inc"
/* --- program messages --- */
static char *copyright =		/* gnu/gpl copyright notice */
	"+-----------------------------------------------------------------------+\n"
	"|" PROGRAMID " ver" VERSION ", " COPYRIGHTTEXT "|\n"
	"|               (  current revision:" REVISIONDATE
	")                    |\n"
	"+-----------------------------------------------------------------------+\n"
	"|gifsave89 is free software, licensed to you under terms of the GNU/GPL,|\n"
	"|           and comes with absolutely no warranty whatsoever.           |\n"
	"|     See http://www.forkosh.com/gifsave89.html for further details.    |\n"
	"+-----------------------------------------------------------------------+";

/* -------------------------------------------------------------------------
url of cgi to pixelize plaintext expressions (see plainmimetext() below)...
-------------------------------------------------------------------------- */
/* --- pixelizing cgi --- */
#if !defined(MIMETEX)
#define MIMETEX "http://www.forkosh.com/mimetex.cgi"
#endif
/* --- local /path/to/wget to run the cgi (-DWGET to supply path/to) --- */
#if !defined(WGET)
#define WGET "wget"
#endif

/* -------------------------------------------------------------------------
gifsave89 data and structures...
-------------------------------------------------------------------------- */
/* ---
 * gifsave89 datatypes
 * ---------------------- */
typedef uint8_t BYTE;			/* one byte (8 bits) */
typedef uint16_t WORD;			/* used for two byte integers */

/* ---
 * gif image buffer (note: ptr to caller's buffer address is BYTE **block)
 * ----------------------------------------------------------------------- */
#define	BLOCK struct _Block
#define	BK BLOCK				/* shorthand for funcs and args */
#define	MAXBLKBYTES  (1024)		/* initial gif buffer allocation */
BLOCK {
	BYTE **block;				/* block buffer */
	int nblkbytes,				/* #bytes already in our block */
	 maxblkbytes;				/* #bytes currently (re)allocated */
};								/* --- end-of-struct BLOCK --- */

/* ---
 * data subblock (as per gif standard)
 * -------------------------------------- */
#define	SUBBLOCK struct _SubBlock
#define	SB SUBBLOCK				/* shorthand for funcs and args */
#define	SUBBLOCKSIZE (255)		/* maximum gif subblock size */
#define	BITSPERBYTE    (8)		/* #bits per byte */
SUBBLOCK {
	BYTE subblock[SUBBLOCKSIZE + 1];	/* subblock buffer */
	BLOCK *block;				/* parent block to flush subblock */
	int nsubbytes,				/* #bytes already in our subblock */
	 nsubbits;					/* #bits  already in current byte */
	int index;					/* >=0 writes subblock index# byte */
};								/* --- end-of-struct SUBBLOCK --- */

/* ---
 * LZW parameters
 * ----------------- */
#define	POW2(n)   ((int)(1<<(n)))	/* 2**n */
#define RESCODES  (2)
#define HASHFREE  (0xFFFF)
#define NEXTFIRST (0xFFFF)
#define CODEBITS  (12)
#define NSTRINGS  POW2((CODEBITS))
#define HASHSIZE  (9973)
#define HASHSTEP  (2039)
#define HASH(index,byte) ( (((byte) << 8) ^ (index)) % (HASHSIZE) )

/* ---
 * "global" gifsave89 data
 * Note: gifsave89 is reentrant by keeping all variables that need to
 * be maintained from call to call in this struct, i.e., no globals.
 * --------------------------------------------------------------------- */
#define	GIFSAVE89 struct _GifSave89
#define	GS GIFSAVE89			/* shorthand for funcs and args */
GIFSAVE89 {
	/* ---
	 * gif image block and subblock buffers
	 * --------------------------------------- */
	BLOCK gifimage;				/* buffer for gif image block */
	SUBBLOCK gifsubblock;		/* buffer for gifimage subblock */
	/* ---
	 * LZW string table
	 * ------------------- */
	BYTE strbyte[NSTRINGS];
	int strnext[NSTRINGS], strhash[HASHSIZE], nstrings;
	/* ---
	 * additional control data
	 * -------------------------- */
	int version;				/* 87=GIF87a, 89=GIF89a */
	int width, height;			/* #row, #col pixels for screen */
	int ncolorbits_gct,			/* #bits/index (global color table) */
	 ncolorbits;				/* local(if given) or global #bits */
	int bgindex;				/* global&local colortable bgindex */
	int npixels;				/* width*height of current image */
	int ncontrol;				/* #controlgif calls before putgif */
	int isanimated,				/* true if animategif() called */
	 delay, tcolor, disposal;	/* animation frame defaults */
	/* ---
	 * plaintext control data
	 * ------------------------- */
	int isplaintext;			/* plaintext flag, 1or2 if present */
	int pt_left, pt_top;		/* col,row of top-left corner */
	int pt_bg, pt_fg;			/* bg,fg colortable indexes */
	int pt_width, pt_height;	/* width,height of pixelized data */
	char pt_data[1024];			/* local copy of user text data */
	BYTE *pt_pixels;			/* pixelized data (kept if flag=2) */
	/* ---
	 * debugging control data
	 * ------------------------- */
	int msglevel;				/* desired verbosity (0=none) */
	FILE *msgfp;				/* debug file ptr */
};								/* --- end-of-struct GIFSAVE89 --- */

/* -------------------------------------------------------------------------
additional gifsave89 parameters and macros...
-------------------------------------------------------------------------- */
/* ---
 * bitfield macros (byte_bits=76543210, with lsb=bit#0 and 128=bit#7set)
 * --------------------------------------------------------------------- */
#define getbit(x,bit)   ( ((x)>>(bit)) & 1 )	/* get   bit-th bit of x */
#define setbit(x,bit)   (  (x) |=  (1<<(bit)) )	/* set   bit-th bit of x */
#define clearbit(x,bit) (  (x) &= ~(1<<(bit)) )	/* clear bit-th bit of x */
#define putbit(x,bit,val) \
            if(((int)(val))==0) clearbit((x),(bit)); else setbit((x),(bit))
#define	bitmask(nbits)	((1<<(nbits))-1)	/* a mask of nbits 1's */
#define getbitfield(x,bit1,nbits) (((x)>>(bit1)) & (bitmask(nbits)))
#define	putbitfield(x,bit1,nbits,val)	/* x:bit1...bit1+nbits-1 = val */ \
	if ( (nbits)>0 && (bit1)>=0 ) {	/* check input */ \
	  (x) &=      (~((bitmask((nbits))) << (bit1))); /*set field=0's*/ \
	  (x) |= (((val)&(bitmask((nbits)))) << (bit1)); /*set field=val*/ \
	  } /* let user supply final ; */
/* ---
 * debugging, etc
 * ----------------- */
/* --- debugging --- */
#define ismsgprint(gs,n) (gs==NULL?0:gs->msglevel>=(n)&&gs->msgfp!=NULL)
#define	OKAY ((status)>=0?1:0)	/*shortcut for funcs to check status */
static int msglevel =			/* message level / verbosity */
#if defined(MSGLEVEL)			/* if compiled with -DMSGLEVEL=n */
	MSGLEVEL;					/*  use default msglevel=n */
#else							/* otherwise */
	0;							/*  use 0 */
#endif
static char msgfile[132] = "\000";	/* file for msgs (stdout if "\0") */
#define	MSGFP stdout			/* default msgfp */
/* --- etc --- */
#define	min2(x,y) ((x)<=(y)?(x):(y))	/* lesser of two values */
#define	max2(x,y) ((x)>=(y)?(x):(y))	/* larger of two values */
#define	absval(x) ((x)>=0?(x):(-(x)))	/* absolute value */

/* -------------------------------------------------------------------------
 * Comments and structs below provide detailed GIF89a format documentation
 * -------------------------------------------------------------------------
 * Structs for gif87a/89a block formats...
 *       from: http://www.matthewflickinger.com/lab/whatsinagif/
 *             http://www.fileformat.info/format/gif/egff.htm
 *   also see: http://www.w3.org/Graphics/GIF/spec-gif89a.txt
 * Several discrepancies between spec-gif89a.txt and egff.htm
 * are "Note:"'ed below.
 * If you prefer printed books, see Chapter 26 in
 *   The File Formats Handbook, Gunter Born, ITP Publishing 1995,
 *   ISBN 1-850-32128-0. From its publication date, you can see
 *   it's old, but its gif format treatment remains unchanged.
 * -------------------------------------------------------------------------
 * gif file layout:
 * Illustrated below is the basic gif file layout.
 * o Each file begins with a Header and Logical Screen Descriptor.
 *   An optional Global Color Table follows the Logical Screen Descriptor.
 *   Each of these blocks is always found at the same offset in the file.
 * o Each image stored in the file contains a Local Image Descriptor, an
 *   optional Local Color Table, a single byte containing the LZW
 *   minimum codesize, image data comprised of one or more sub-blocks,
 *   followed by a terminating 0 byte.
 * o The detailed byte-by-byte (and bit-by-bit where necessary) internal
 *   structure for each type of block is laid out in the structs and
 *   their accompanying comments below.
 * o The last field in every gif file is a Terminator character (hex 3B).
 * -------------------------------------------------------------------------
 *                     +---
 *  Header and Color   | Header
 *  Table Information  | Logical Screen Descriptor
 *                     | Global Color Table
 *                     +---
 *                     +---
 *  Optional Extension | Comment Extension
 *  information (valid | Application Extension
 *  only for gif89a)   | Graphic Control Extension
 *                     +---
 *                      Either...
 *                        +---
 *  (gif89a only)         | Plain Text Extension
 *                        +---
 *                      Or...
 *                        +---
 *                        | Local Image Descriptor
 *       Image 1          | Local Color Table
 *                        | Image Data (preceded by LZW min codesize byte)
 *                        +---
 *                          ... optional extensions for ...
 *                          ... Image 2, etc ...
 *                     +---
 *  Optional Extension | Comment Extension
 *  information (valid | Application Extension
 *  only for gif89a)   | Graphic Control Extension
 *                     +---
 *                      Either...
 *                        +---
 *  (gif89a only)         | Plain Text Extension
 *                        +---
 *                      Or...
 *                        +---
 *                        | Local Image Descriptor
 *       Last Image       | Local Color Table
 *                        | Image Data (preceded by LZW min codesize byte)
 *                        +---
 *                     +---
 *  Trailer            | Trailer Byte (always hex 3B)
 *                     +---
 * -------------------------------------------------------------------------
 * Notes:
 *    o	WORD's are 16-bit (2-byte) integers, ordered little-endian,
 *	regardless of cpu architecture byte ordering.
 *    o	Image Data is preceded by a byte containing the minimum LZW codesize,
 *	as stated above, and must also be followed by a byte containing 0
 *	to terminate the sequence of sub-blocks.
 *    o	gif89a added "Control Extensions" to gif87a, to control the
 *	rendering of image data. 87a only allowed one image at a time
 *	to be displayed in "slide show" fashion. Control Extensions allow
 *	both textual and graphical data to be displayed, overlaid, and
 *	deleted in "animated multimedia presentation" fashion.
 *    o	Control Extension blocks may appear almost anywhere following the
 *	Global Color Table. Extension blocks begin with the Extension
 *	Introducer hex 21, followed by a Block Label (hex 00 to FF)
 *	identifying the type of extension.
 * -------------------------------------------------------------------------
 * Flow diagram for assembling gif blocks...
 * ------------------+------------------------------------------------------
 * key: +----------+ |           +--------+
 *      | required | |           | Header |
 *      +----------+ |           +--------+
 *                   |                |
 *      +- - - - - + |  +---------------------------+
 *      | optional | |  | Logical Screen Descriptor |
 *      + - - - - -+ |  +---------------------------+
 * ------------------+                |
 *                          +- - - - - - - - - - +
 *                          | Global Color Table |
 *                          + - - - - - - - - - -+
 *                                    |
 *                                    |<--------------<------------------+
 *                                    |                                  |
 *                 +------------------+---------------+                  |
 *                 |                                  |                  |
 *                 |                         +- - - - - - - - -+         |
 *                 |                         | Graphic Control |         ^
 *                 |                         |    extension    |         |
 *                 |                         + - - - - - - - - +         |
 *                 |                                  |                  |
 *                 |                         +--------+--------+         |
 *                 |                         |                 |         ^
 *                 |                         |          +------------+   |
 *                 |                         |          |   Image    |   |
 *                 |                         |          | Descriptor |   |
 *         +-------+------+                  |          +------------+   |
 *         |              |                  |                 |         ^
 *  +-------------+ +-----------+     +------------+    +- - - - - - +   |
 *  | Application | |  Comment  |     | Plain Text |    |    Local   |   |
 *  |  extension  | | extension |     |  extension |    | Color Table|   |
 *  +-------------+ +-----------+     +------------+    + - - - - - -+   |
 *         |              |                  |                 |         ^
 *         +-------+------+                  |             +-------+     |
 *                 |                         |             | Image |     |
 *                 |                         |             |  Data |     |
 *                 |                         |             +-------+     |
 *                 |                         |                 |         ^
 *                 |                         +--------+--------+         |
 *                 |                                  |                  |
 *                 +---------------+------------------+                  |
 *                                 |                                     |
 *                                 +------------------>------------------+
 *                                 |
 *                            +---------+
 *                            | Trailer |
 *                            +---------+
 * -------------------------------------------------------------------------
 * PS:
 *    o The structs defined below,
 *	  GIFHEADER, GIFCOLORTABLE, GIFIMAGEDESC,
 *	  GIFGRAPHICCONTROL, GIFPLAINTEXT, GIFAPPLICATION, GIFCOMMENT
 *	aren't actually used by the gifsave89 (except for GIFAPPLICATION).
 *	They're provided mostly to help document the gif file format.
 *	But they're not actually used because structs can't typically
 *	just be output as, e.g.,
 *	  memcpy((void *)image,(void *)&struct,sizeof(struct));
 *	because we can't usually be sure structs are packed in memory.
 *	Syntax for specifying packed structs to C compilers isn't
 *	completely portably possible, so gifsave89 outputs each
 *	member element separately.
 * ----------------------------------------------------------------------- */

/* ---
 * Gif header and logical screen descriptor:
 * -------------------------------------------- */
#define	GIFHEADER struct _GifHeader
GIFHEADER {
	/* ---
	 * Header
	 * --------- */
	BYTE Signature[3];			/* Header Signature (always "GIF") */
	BYTE Version[3];			/* GIF format version ("87a" or "89a") */
	/* ---
	 * Logical Screen Descriptor
	 * ---------------------------- */
	WORD ScreenWidth;			/* Width of Display Screen in Pixels */
	WORD ScreenHeight;			/* Height of Display Screen in Pixels */
	/* ScreenHeight and Width are the minimum resolution to display
	   the image (if the display device does not support this resolution,
	   some sort of scaling is done to display the image). */
	BYTE Packed;				/* Screen and Color Map Information */
	/* Packed contains four subfields, with bit 0 the
	   least significant bit, or LSB...
	   Bits 0-2: Size of each Global Color Table entry
	   the number of bits in each Global Color Table entry minus 1,
	   e.g., 7 if the image contains 8 bits per pixel. And the
	   number of Global Color Table entries is then
	   #GlobalColorTableEntries = 1 << (SizeOfTheGlobalColorTable + 1)
	   Bit    3: Global Color Table Sort Flag (always 0 for 87a)
	   if 1, then Global Color Table entries are sorted from the most
	   important (most frequently occurring) to the least (sorting
	   aids applications in choosing colors to use with displays that
	   have fewer colors than the image). Valid only for 89a, 0 for 87a.
	   Bits 4-6: Color Resolution
	   the number of bits in an entry of the original color palette
	   minus 1 (e.g., 7 if the image originally contained 8 bits per
	   primary color)
	   Bit    7: Global Color Table Flag
	   1 if a Global Color Table is present in the GIF file, and 0 if
	   not (if present, Global Color Table data always follows the
	   Logical Screen Descriptor header) */
	BYTE BackgroundColor;		/* Background Color Index */
	/* the Global Color Table index of the color to use for the image's
	   border and background */
	BYTE AspectRatio;			/* Pixel Aspect Ratio (usually 0) */
	/* not used if 0, else the aspect ratio value of image pixels
	   (pixel width divided by the height, in the range of 1 to 255,
	   used as PixelAspectRatio = (AspectRatio+15)/64) */
};								/* --- end-of-struct GIFHEADER --- */

/* ---
 * Gif color table:
 * the number of entries in the Global Color Table is a power of two
 * (2, 4, 8, 16, etc), up to 256. Its size in bytes is calculated using
 * bits 0, 1, and 2 in the Packed field of the Logical Screen Descriptor:
 * ColorTableSize(in bytes) = 3 * (1 << (SizeOfGlobalColorTable + 1))
 * ----------------------------------------------------------------------- */
#define	GIFCOLORTABLE struct _GifColorTable
GIFCOLORTABLE {
	BYTE Red;					/* Red Color Element */
	BYTE Green;					/* Green Color Element */
	BYTE Blue;					/* Blue Color Element */
};								/* --- end-of-struct GIFCOLORTABLE --- */

/* ---
 * Gif local image descriptor:
 * the Local Image Descriptor appears before each section of image data
 * and has the following structure...
 * ----------------------------------------------------------------------- */
#define GIFIMAGEDESC struct _GifImageDescriptor
GIFIMAGEDESC {
	BYTE Separator;				/* Image Descriptor identifier */
	/* hex value 2C denoting the beginning of the Image Descriptor */
	WORD Left;					/* X position of image on the display */
	WORD Top;					/* Y position of image on the display */
	/* Left, Top are pixel coords of the upper-left corner of the image
	   on the logical screen (the screen's upper-left corner is 0,0) */
	WORD Width;					/* Width of the image in pixels */
	WORD Height;				/* Height of the image in pixels */
	/* Width, Height are the image size in pixels */
	BYTE Packed;				/* Image and Color Table Data Information */
	/* Packed contains five subfields, with bit 0 the LSB...
	   (Note/warning: egff.htm has this documented "backwards",
	   whereas spec-gif89a.txt is correct. The correct version is below.)
	   Bits 0-2: Size of each Local Color Table Entry
	   the number of bits in each Local Color Table entry minus 1
	   Bits 3-4: Reserved
	   Bit    5: Local Color Table Sort Flag (always 0 for 87a)
	   1 if entries in the Local Color Table are sorted in order of
	   importance, or 0 if unsorted (valid only for 89a, 0 for 87a)
	   Bit    6: Interlace Flag
	   1 if the image is interlaced, or 0 if it is non-interlaced
	   Bit    7: Local Color Table Flag
	   1 if a Local Color Table is present, or 0 to use
	   the Global Color Table */
};								/* --- end-of-struct GIFIMAGEDESC --- */

/* ---
 * Gif graphics control extension block:
 * A Graphics Control Extension block modifies how (either bitmap or text)
 * data in the Graphics Rendering block that immediately follows it is
 * displayed, e.g., whether the graphic is overlaid transparent or opaque
 * over another graphic, whether it is to be restored or deleted, whether
 * user input is expected before continuing with the display of data, etc.
 * A Graphics Control block must occur before the data it modifies, and
 * only one Graphics Control block may appear per Graphics Rendering block.
 * ----------------------------------------------------------------------- */
#define GIFGRAPHICCONTROL struct _GifGraphicsControlExtension
GIFGRAPHICCONTROL {
	BYTE Introducer;			/* Extension Introducer (always hex 21) */
	BYTE Label;					/* Graphic Control Label (always hex F9) */
	BYTE BlockSize;				/* Size of remaining fields (always hex 04) */
	BYTE Packed;				/* Method of graphics disposal to use */
	/* Packed contains four subfields, with bit 0 the LSB...
	   Bit    0: Transparent Color Flag
	   if 1, the ColorIndex field contains a color transparency index
	   Bit    1: User Input Flag
	   if 1, user input (key press, mouse click, etc) is expected
	   before continuing to the next graphic sequence
	   Bits 2-4: Disposal Method
	   (Note: spec-gif89a.txt says values may be 0,1,2,3 rather
	   than 0,1,2,4 in egff.htm. 0,1,2,3 appears to be correct.)
	   Indicates how the graphic is to be disposed of after display...
	   hex 00 = disposal method not specified, hex 01 = do not dispose
	   of graphic, hex 02 = overwrite graphic with background color,
	   and hex 03 = restore with previous graphic
	   Bits 5-7: Reserved */
	WORD DelayTime;				/* Hundredths of seconds to wait */
	/* hundredths of a second before graphics presentation continues,
	   or 0 for no delay */
	BYTE ColorIndex;			/* Transparent Color Index */
	/* color transparency index (if the Transparent Color Flag is 1) */
	BYTE Terminator;			/* Block Terminator (always 0) */
};								/* --- end-of-struct GIFGRAPHICCONTROL --- */

/* ---
 * Gif plain text extension block:
 * Plain Text Extension blocks allow mixing ASCII text, displayed as
 * graphics, with bitmapped image data, e.g., captions that are not
 * part of the bitmapped image may be overlaid on it.
 * ----------------------------------------------------------------------- */
#define GIFPLAINTEXT struct _GifPlainTextExtension
GIFPLAINTEXT {
	BYTE Introducer;			/* Extension Introducer (always hex 21) */
	BYTE Label;					/* Extension Label (always hex 01) */
	BYTE BlockSize;				/* Size of Extension Block (always hex 0C) */
	WORD TextGridLeft;			/* X position of text grid in pixels */
	WORD TextGridTop;			/* Y position of text grid in pixels */
	/* X,Y coords of the text grid with respect to the upper-left
	   corner of the display screen (0,0) */
	WORD TextGridWidth;			/* Width of the text grid in pixels */
	WORD TextGridHeight;		/* Height of the text grid in pixels */
	/* the size of the text grid in pixels */
	BYTE CellWidth;				/* Width of a grid cell in pixels */
	BYTE CellHeight;			/* Height of a grid cell in pixels */
	/* the size in pixels of each character cell in the grid */
	BYTE TextFgColorIndex;		/* Text foreground color index value */
	BYTE TextBgColorIndex;		/* Text background color index value */
	/* indexes into the (Global or Local) Color Table for the color
	   of the text, and for the color of the background */
	BYTE *PlainTextData;		/* The Plain Text data */
	/* the text to be rendered as a graphic, comprised of one or more
	   sub-blocks, each beginning with a byte containing the number of
	   text bytes (from 1 to 255) that follow */
	BYTE Terminator;			/* Block Terminator (always 0) */
};								/* --- end-of-struct GIFPLAINTEXT --- */

/* ---
 * Gif application extension block:
 * Application Extension blocks store data understood only by the application
 * reading the gif file (similar to how tags are used in TIFF and TGA image
 * file formats), e.g., instructions on changing video modes, on applying
 * special processing to displayed image data, etc.
 * ----------------------------------------------------------------------- */
#define GIFAPPLICATION struct _GifApplicationExtension
GIFAPPLICATION {
	BYTE Introducer;			/* Extension Introducer (always hex 21) */
	BYTE Label;					/* Extension Label (always hex FF) */
	BYTE BlockSize;				/* Size of Extension Block (always hex 0B) */
	BYTE Identifier[8];			/* Application Identifier */
	/* up to eight printable 7-bit ASCII chars, used to identify the
	   application that wrote the Application Extension block */
	BYTE AuthentCode[3];		/* Application Authentication Code */
	/* a value used to uniquely identify a software application that
	   created the Application Extension block, e.g., a serial number,
	   a version number, etc */
	BYTE *ApplicationData;		/* Point to Application Data sub-blocks */
	/* the data used by the software application, comprised of a series
	   of sub-blocks identical to data in Plain Text Extension blocks */
	BYTE Terminator;			/* Block Terminator (always 0) */
};								/* --- end-of-struct GIFAPPLICATION --- */

/* ---
 * Gif comment extension block:
 * data stored in Comment Extension blocks is to be read only by the user
 * examining a gif file, and should be ignored by the application.
 * ----------------------------------------------------------------------- */
#define GIFCOMMENT struct _GifCommentExtension
GIFCOMMENT {
	BYTE Introducer;			/* Extension Introducer (always hex 21) */
	BYTE Label;					/* Comment Label (always hex FE) */
	/*no BYTE BlockSize; *//* Size of Extension Block (always hex ??) */
	BYTE *CommentData;			/* Pointer to Comment Data sub-blocks */
	/* one or more sub-blocks of ASCII string data (strings not
	   required to be NULL-terminated */
	BYTE Terminator;			/* Block Terminator (always 0) */
};								/* --- end-of-struct GIFCOMMENT --- */


/* ==========================================================================
 * Functions:	newgif(gifimage,width,height,colors,bgindex)
 *		  animategif(gs,nrepetitions)                      *optional*
 *		 plaintxtgif(gs,left,top,width,height,fg,bg,data)  *optional*
 *		  controlgif(gs,tcolor,delay,userinput,disposal)   *optional*
 *		putgif(gs,pixels) or                             *short form*
 *		fputgif(gs,left,top,width,height,pixels,colors)   *long form*
 *		endgif(gs)
 *		--- alternative "short form" for a single gif image ---
 *		makegif(nbytes,width,height,pixels,colors,bgindex)*short form*
 * Purpose:	construct gif -- user-callable entry points for gifsave89
 * --------------------------------------------------------------------------
 * Arguments:
 *	For newgif(), called once to intialize gif...
 *		gifimage (O)	(void **) where ptr to buffer (that will
 *				be malloc'ed by gifsave89) containing
 *				constructed gif image is stored
 *		width (I)	int containing screen (total image)
 *				width (#columns) in pixels
 *		height (I)	int containing screen (total image)
 *				height (#rows) in pixels
 *		colors (I)	int * to r,g,b, r,g,b, ..., -1 containing
 *				image colors for gif global color table
 *				(see putgifcolortable() comments for more
 *				details)
 *		bgindex (I)	int containing colors[] index (0=first color)
 *				for background
 *	For all subsequent function() calls...
 *		gs (I)		void * returned by newgif() is always the
 *				first argument to every other function
 *	For putgif(), called once or looped to render each frame of gif...
 *		+------------------------------------------------------------
 *		| Note: several variables removed as putgif() args; the more
 *		| detailed entry point fputgif, below, includes top,left,
 *		| width,height,colors args, to provide "full" functionality
 *		| putgif() takes top,left of frame, relative to screen
 *		| origin, as 0,0, and takes width,height as screen dimensions
 *		+------------------------------------------------------------
 *		gs (I)		as described above
 *		pixels (I)	unsigned char * to width*height bytes,
 *				from upper-left to lower-right,
 *				each byte containing a colors[] index
 *      For fputgif(), "full" putgif with additional args...
 *		top, left (I)	ints containing row,col of upper-left
 *				corner pixel, with 0,0=upper-left of screen
 *		width, height (I) ints containing width,height of pixels
 *				image (which must be less than width,height
 *				of screen if top,left>0,0)
 *		colors (I)	int * to local color table for this frame,
 *				or NULL to use global color table
 *	For endgif(), called once after all frames rendered...
 *		gs (I)		as described above
 *	   For animategif(), optionally called once after newgif...
 *		gs (I)		as described above
 *		nrepetitions (I) int containing 0 to loop continuously,
 *				or containing #repetitions, 0 to 65535.
 *	   For plaintxtgif(), optionally called preceding put & control...
 *		+------------------------------------------------------------
 *		| Note: issues wget to mimeTeX to render image of plaintext
 *		+------------------------------------------------------------
 *	   For controlgif(), optionally called immediately before putgif...
 *		gs (I)		as described above
 *		tcolor (I)	int containing colors[] index of
 *				transparent color, or -1 to not use
 *		delay (I)	int containing delay in hundredths-of-second
 *				between frames, or 0 for no delay
 *		userinput (I)	int containing 1 to wait for user keystroke
 *				or mouseclick before displaying next frame,
 *				or containing 0 if no user input required
 *		disposal (I)	int containing disposition of frame after
 *				display
 *		void (*gifimage=) *makegif(&nbytes_in_gifimage,
 *		                          width,height,pixels,colors,bgindex)
 * --------------------------------------------------------------------------
 * Returns:	( void * )	newgif() returns a ptr that must be passed
 *				as the first argument to every subsequent
 *				gifsave89 function called, 
 *		( void * )	makegif() returns (unsigned char *)gifimage
 *				or both return NULL for any error.
 *		( int )		putgif() returns current #bytes in gifimage,
 *				endgif() returns total   #bytes in gifimage,
 *				all other functions (extension block funcs)
 *				return 1 if successful,
 *				and all return -1 for any error.
 * --------------------------------------------------------------------------
 * Notes:     o	sequence of gifsave function calls...
 *		  First, call gs=newgif(&gifimage,etc) once.
 *		  Next, optionally call animategif(gs,nrepetitions) once.
 *		  Now, once or in a loop {
 *		    optionally call controlgif(gs,etc)
 *		    call putgif(gs,pixels) }
 *		  Finally, call nbytes=endgif(gs) once.
 *		  Do what you want with gifimage[nbytes], then free(gifimage)
 *	      o	minimal usage example/template
 *		(see main() Notes for additional info)...
 *		  void *gs=NULL, *newgif();
 *		  int  nbytes=0, putgif(), endgif();
 *		  int  width=255, height=255,
 *		       *colors = { 255,255,255, 0,0,0, -1 },
 *		       bgindex=0;
 *		  unsigned char *pixels = { 0,1,0,1, ..., width*height };
 *		  unsigned char *gifimage = NULL;
 *		  gs = newgif(&gifimage,width,height,colors,bgindex);
 *		  if ( gs != NULL ) {
 *		    putgif(gs,pixels);
 *		    nbytes = endgif(gs); }
 *		  if ( nbytes > 0 ) {
 *		    do_what_you_want_with(gifimage,nbytes);
 *		    free(gifimage); }
 * ======================================================================= */
/* ---
 * entry point
 * -------------- */
void *newgif(void **gifimage, int width, int height,
			 int *colors, int bgindex)
{
/* ---
 * allocations and declarations
 * ------------------------------- */
	GS *newgifstruct(),			/*allocate and init gif data struct */
	*gs = newgifstruct((void **) gifimage, width, height);	/*new gif data */
	int version = 89;			/* default is GIF89a */
	int putgifcolortable(),		/* emit (global) color table */
	 ncolorbits = putgifcolortable(NULL, colors);	/*#bits for color index */
	BYTE Signature[4] = "GIF",	/* header always "GIF" */
		Version[4] = "89a";		/* and either 87a or 89a */
	BYTE Packed = (BYTE) 0;		/* packed byte */
	int AspectRatio = 0;		/* aspect ratio unused */
	BK *bk = (gs == NULL ? NULL : &(gs->gifimage));	/* ptr to gif image block */
	int putblkbytes(), putblkbyte(), putblkword();	/* write to image block */
	int status = 1;				/* init okay status */
/* ---
 * check input
 * -------------- */
	if (gs == NULL)
		goto end_of_job;		/* failed to malloc gif data struct */
	width = min2(9999, max2(1, width));	/* 1 <= width  <= 9999 */
	height = min2(9999, max2(1, height));	/* 1 <= height <= 9999 */
	gs->width = width;
	gs->height = height;		/* reset values stored in gs */
	bgindex = (max2(0, bgindex)) % 256;	/* 0<=bgindex<=255 */
	gs->bgindex = bgindex;		/* global&local colortable bgindex */
	gs->version = (version == 87 ? 87 : 89);	/* default to 89a if not 87 */
	if (version == 87)
		memcpy(Version, "87a", 4);	/* reset to 87a if it is 87 */
	if (!OKAY)
		goto end_of_job;		/* quit if input check failed */
/* ---
 * header and screen descriptor
 * ------------------------------- */
	Packed = 0;					/* zero out Packed byte */
	if (ncolorbits > 0) {		/* have colors[] color table */
		putbitfield(Packed, 0, 3, ncolorbits - 1);	/* #bits for color index - 1 */
		clearbit(Packed, 3);	/* clear color table sort flag bit */
		putbitfield(Packed, 4, 3, 8 - 1);	/* assume 8-bit color resolution */
		setbit(Packed, 7);		/* set global color table bit */
	}
/* --- Header ---- */
	status = putblkbytes(bk, Signature, 3);
	if (OKAY)
		status = putblkbytes(bk, Version, 3);
/* --- Logical Screen Descriptor --- */
	if (OKAY)
		status = putblkword(bk, width);
	if (OKAY)
		status = putblkword(bk, height);
	if (OKAY)
		status = putblkbyte(bk, Packed);
	if (OKAY)
		status = putblkbyte(bk, bgindex);
	if (OKAY)
		status = putblkbyte(bk, AspectRatio);
	if (!OKAY)
		goto end_of_job;
/* ---
 * global color table
 * --------------------- */
	gs->ncolorbits_gct =		/* store #colorbits in global table */
		gs->ncolorbits = max2(0, ncolorbits);	/* and default if no local table */
	if (ncolorbits > 0) {		/* caller provided global colors[] */
		putgifcolortable(gs, colors);
	}							/* emit global color table */
  end_of_job:
	if (!OKAY)					/* something fouled up */
		if (gs != NULL) {		/* but we have an allocated gs */
			free((void *) gs);
			gs = NULL;
		}						/* free it and signal error */
	return (((void *) gs));		/* return data struct as "blob" */
}								/* --- end-of-function newgif() --- */

/* ---
 * entry point
 * -------------- */
int animategif(GS * gs, int nrepetitions,
			   int delay, int tcolor, int disposal)
{
/* --- allocations and declarations --- */
	int status = (-1);			/* init for error */
	int putgifapplication();	/* emit application extension */
	GIFAPPLICATION gifapplication, *ga = (&gifapplication);
	BK *bk = (gs == NULL ? NULL : &(gs->gifimage));	/* ptr to gif image block */
	int putblkbytes(), putblkbyte(), putblkword();	/* write to image block */
/* ---
 * check input
 * -------------- */
	if (bk == NULL)
		goto end_of_job;		/* no gif data struct supplied */
	if (gs->version == 87)
		goto end_of_job;		/* no extensions for GIF87a */
	if (gs->isanimated > 0)
		goto end_of_job;		/* just one animategif() per gif */
	if (nrepetitions < 0 || nrepetitions > 65535)
		nrepetitions = 0;		/*0=continuous */
/* ---
 * construct graphic control extension and emit it
 * -------------------------------------------------- */
	memset((void *) ga, 0, sizeof(GIFAPPLICATION));	/* zero out the whole thing */
	memcpy(ga->Identifier, "NETSCAPE", 8);	/* netscape */
	memcpy(ga->AuthentCode, "2.0", 3);	/* 2.0 */
	ga->ApplicationData = NULL;	/* no data */
	status = putgifapplication(gs, ga);	/* emit application extension hdr */
/* --- emit the data and terminator ourselves (for the time being) --- */
	if (OKAY)
		status = putblkbyte(bk, 3);	/* 3 more bytes of data */
	if (OKAY)
		status = putblkbyte(bk, 1);	/* data sub-block index (always 1) */
	if (OKAY)
		status = putblkword(bk, nrepetitions);	/* #repetitions */
	if (OKAY)
		status = putblkbyte(bk, 0);	/* sub-block sequence terminator */
	if (OKAY) {
		gs->delay = delay;		/* set default frame delay */
		gs->tcolor = tcolor;	/* set default transparent index */
		gs->disposal = disposal;	/* set default disposal method */
		gs->isanimated++;
	}							/* set isanimated flag true */
  end_of_job:
	return (status);
}								/* --- end-of-function animategif() --- */

/* ---
 * entry point
 * -------------- */
int plaintxtgif(GS * gs, int left, int top, int width, int height,
				int fg, int bg, char *data)
{
/* --- allocations and declarations --- */
	int status = (-1);			/* init signalling error */
	int Introducer = 0x21;		/* always 0x21 */
	int Label = 0x01;			/* always 0x01 */
	int BlockSize = 0x0C;		/* always 0x0C */
	int Terminator = 0x00;		/* always 0x00 */
	BK *bk = (gs == NULL ? NULL : &(gs->gifimage));	/* ptr to gif image block */
	SB *sb = (gs == NULL ? NULL : &(gs->gifsubblock));	/* ptr to gif subblock */
	int putblkbytes(), putblkbyte(), putblkword();	/* write to image block */
	int putsubbytes(), flushsubblock();	/* data subblock */
	int nchars = (data == NULL ? 0 : strlen(data));	/* #plaintext chars */
	int cellwidth = 0, cellheight = height;	/* #pixels per char */
	int ismimetex = 1;			/* true to use mimetex for text */
/* ---
 * check input
 * -------------- */
	if (bk == NULL || sb == NULL)
		goto end_of_job;		/*no gif data struct supplied */
	if (gs->version == 87)
		goto end_of_job;		/* no extensions for GIF87a */
	cellwidth = (nchars < 1 ? 0 : width / nchars);	/* #pixels/char width */
	if (!ismimetex) {			/* irrelevant args if using mimetex */
		if (cellwidth < 2 || height < 2)
			goto end_of_job;	/* won't work */
		if (nchars < 1)
			goto end_of_job;
	}
	/* no data won't work, either */
	/* ---
	 * reset (turn off) any preceding "persistent" plaintext
	 * -------------------------------------------------------- */
	if (ismimetex)				/* but for mimeTeX... */
		if (nchars < 1) {		/* ...no data signals reset */
			gs->isplaintext = 0;	/* reset plaintext flag */
			if (gs->pt_pixels != NULL) {	/* have previous allocated image */
				free(gs->pt_pixels);
				gs->pt_pixels = NULL;
			}					/* free it and reset ptr */
			gs->pt_left = gs->pt_top = 0;	/* reset top-left corner col;row */
			gs->pt_bg = gs->pt_fg = 0;	/* reset bg,fg colortable indexes */
			gs->pt_width = gs->pt_height = 0;	/*reset pixelized text width,height */
			memset(gs->pt_data, 0, 1023);	/* reset text data */
			status = 1;			/* set successful status */
			goto end_of_job;	/* plaintext reset */
		}
	/* --- end-of-if(nchars<1) --- */
	/* ---
	 * save mimetex data for subsequent putgif()
	 * -------------------------------------------- */
	if (ismimetex) {			/* use mimeTeX to render plaintext */
		gs->isplaintext = 1;	/* set plaintext flag */
		if (width < 0 || height < 0) {	/* set persistent text flag */
			gs->isplaintext = 2;	/* display same text on all frames */
			width = absval(width);
			height = absval(height);
		}						/* reset neg signal */
		gs->pt_left = left;
		gs->pt_top = top;		/* save top-left corner col;row */
		gs->pt_bg = bg;
		gs->pt_fg = fg;			/* save bg,fg colortable indexes */
		gs->pt_width = gs->pt_height = 0;	/*reset pixelized text width,height */
		nchars = min2(1023, nchars);	/* don't overflow data[] buffer */
		strncpy(gs->pt_data, data, nchars);	/* local copy of text data */
		(gs->pt_data)[nchars] = '\000';	/* make sure it's null-terminated */
		if (gs->pt_pixels != NULL) {	/* have previous allocated image */
			free(gs->pt_pixels);
			gs->pt_pixels = NULL;
		}						/* free it and reset ptr */
		status = 1;				/* set successful status */
	}
	/* --- end-of-if(ismimetex) --- */
	/* ---
	 * or construct plain text extension and emit it
	 * ------------------------------------------------ */
	if (!ismimetex) {			/* emit gif89a plaintent extension */
		status = putblkbyte(bk, Introducer);
		if (OKAY)
			status = putblkbyte(bk, Label);
		if (OKAY)
			status = putblkbyte(bk, BlockSize);
		if (OKAY)
			status = putblkword(bk, left);
		if (OKAY)
			status = putblkword(bk, top);
		if (OKAY)
			status = putblkword(bk, width);
		if (OKAY)
			status = putblkword(bk, height);
		if (OKAY)
			status = putblkbyte(bk, cellwidth);
		if (OKAY)
			status = putblkbyte(bk, cellheight);
		if (OKAY)
			status = putblkbyte(bk, fg);
		if (OKAY)
			status = putblkbyte(bk, bg);
		if (OKAY)
			status = putsubbytes(sb, (BYTE *) data, nchars);
		if (OKAY)
			status = flushsubblock(sb);
		if (OKAY)
			status = putblkbyte(bk, Terminator);
	}							/* --- end-of-if(!ismimetex) --- */
  end_of_job:
	return (status);
}								/* --- end-of-function plaintxtgif() --- */

/* ---
 * entry point
 * -------------- */
int controlgif(GS * gs, int tcolor, int delay, int userinput, int disposal)
{
/* --- allocations and declarations --- */
	int status = (-1);			/* init signalling error */
	int Introducer = 0x21;		/* always 0x21 */
	int Label = 0xF9;			/* always 0xF9 */
	int BlockSize = 0x04;		/* always 0x04 */
	int Terminator = 0x00;		/* always 0x00 */
	BYTE Packed = (BYTE) 0;		/* packed byte */
	BK *bk = (gs == NULL ? NULL : &(gs->gifimage));	/* ptr to gif image block */
	int putblkbytes(), putblkbyte(), putblkword();	/* write to image block */
/* ---
 * check input
 * -------------- */
	if (bk == NULL)
		goto end_of_job;		/* no gif data struct supplied */
	if (gs->version == 87)
		goto end_of_job;		/* no extensions for GIF87a */
	if (gs->ncontrol > 0)
		goto end_of_job;		/* just one control per image */
	if (tcolor < 0 || tcolor > 255)
		tcolor = (-1);			/* not transparent if illegal */
	if (delay < 0 || delay > 65535)
		delay = 0;				/* 0 if illegal (<0 or >2**16-1) */
	userinput = (userinput <= 0 ? 0 : 1);	/* "yes" if positive */
	if (disposal != 1 && disposal != 2	/* 0,1,2,3 are okay */
		&& disposal != 3)
		disposal = 0;			/* default to 0 otherwise */
/* ---
 * construct graphic control extension and emit it
 * -------------------------------------------------- */
	putbit(Packed, 0, (tcolor >= 0 ? 1 : 0));	/* set transparent color flag */
	putbit(Packed, 1, userinput);	/* set user input flag */
	putbitfield(Packed, 2, 3, disposal);	/* set requested disposal method */
	putbitfield(Packed, 5, 3, 0);	/* clear reserved bits */
	status = putblkbyte(bk, Introducer);
	if (OKAY)
		status = putblkbyte(bk, Label);
	if (OKAY)
		status = putblkbyte(bk, BlockSize);
	if (OKAY)
		status = putblkbyte(bk, Packed);
	if (OKAY)
		status = putblkword(bk, delay);
	if (OKAY)
		status = putblkbyte(bk, (tcolor < 0 ? 0 : tcolor));
	if (OKAY)
		status = putblkbyte(bk, Terminator);
	if (OKAY)
		gs->ncontrol++;			/* count (another?) graphic control */
  end_of_job:
	return (status);
}								/* --- end-of-function controlgif() --- */

/* ---
 * entry point
 * -------------- */
int putgif(GS * gs, void *pixels)
{
/* --- allocations and declarations --- */
	int status = (-1), fputgif();	/* fputgif() does all the work */
/* --- call fputgif() --- */
	if (gs != NULL && pixels != NULL)	/* caller passed required args */
		status = fputgif(gs, 0, 0, gs->width, gs->height, pixels, NULL);
	return (status);			/* return current #bytes in image */
}								/* --- end-of-function putgif() --- */

/* ---
 * entry point
 * -------------- */
int fputgif(GS * gs, int left, int top, int width, int height,
			void *pixels, int *colors)
{
/* --- allocations and declarations --- */
#if 0
	int width = (gs == NULL ? 0 : gs->width),	/* default  width: image = screen */
		height = (gs == NULL ? 0 : gs->height);	/* default height: image = screen */
	int left = 0, top = 0;		/* default left,top coords */
#endif
	int Separator = 0x2C;		/* image descrip separator is 0x2C */
	BYTE Packed = (BYTE) 0;		/* packed byte */
	BK *bk = (gs == NULL ? NULL : &(gs->gifimage));	/* ptr to gif image block */
	int npixels = (gs == NULL ? (-1) : gs->npixels);	/* width*height */
	int putblkbytes(), putblkbyte(), putblkword();	/* write to image block */
	int codesize;				/* write min lzw codesize byte */
	int nbytes = 0, encodelzw();	/* lzw encode the image pixels */
	int putgifcolortable(),		/* emit (local) color table */
	 ncolorbits = putgifcolortable(NULL, colors);	/*#bits for color index */
	BYTE *plainmimetext(),		/* pixelize plaintext data, */
	*overpix = NULL, *overlay();	/* and overlay that text on pixels */
	int controlgif();			/* to emit animation controlgif() */
	int isextensionallowedbetween = 0;	/* true for descrip+extension+image */
	int fprintpixels();			/* user-requested debug display */
	int status = (-1);			/* init for error */
/* ---
 * check input
 * ----------- */
	if (bk == NULL)
		goto end_of_job;		/* no gs data provided */
	if (!isextensionallowedbetween)	/*allow extension between descrip/image? */
		if (pixels == NULL || width < 1 || height < 1)	/* if not allowed, then */
			goto end_of_job;	/*signal error for any missing args */
/* --- debug pixel display --- */
	if (ismsgprint(gs, 32))		/* msglevel >= 32 */
		fprintpixels(gs, (gs->msglevel >= 99 ? 2 : 1), pixels);	/* hex format if >=99 */
/* ---
 * emit default controlgif() for animations if not already done by user
 * ----------------------------------------------------------------------- */
	if (gs->isanimated > 0)		/* rendering gif animation */
		if (gs->ncontrol < 1) {	/* but no graphic control for frame */
			status = controlgif(gs,	/* default graphic control... */
								gs->tcolor,	/* default animation transparency */
								gs->delay,	/* default animation frame delay */
								0,	/* userinput always 0 */
								gs->disposal);	/* default animation disposal */
			if (!OKAY)
				goto end_of_job;	/* quit if failed */
			gs->ncontrol++;
		}
	/* count controlgif() call */
	/* ---
	 * image descriptor
	 * ------------------- */
	if (width >= 1 && height >= 1) {
		gs->npixels = npixels = width * height;	/* set for encodelzw() */
		Packed = (BYTE) 0;		/* zero out Packed byte */
		putbit(Packed, 7, max2(0, ncolorbits));	/* set local color table bit */
		clearbit(Packed, 6);	/* clear interlace flag bit */
		clearbit(Packed, 5);	/* clear sort flag bit */
		putbitfield(Packed, 3, 2, 0);	/* clear reserved bits */
		if (ncolorbits > 0) {	/* have colors[] color table */
			putbitfield(Packed, 0, 3, ncolorbits - 1);
		}						/* #bits for color index - 1 */
		status = putblkbyte(bk, Separator);
		if (OKAY)
			status = putblkword(bk, left);
		if (OKAY)
			status = putblkword(bk, top);
		if (OKAY)
			status = putblkword(bk, width);
		if (OKAY)
			status = putblkword(bk, height);
		if (OKAY)
			status = putblkbyte(bk, Packed);
		if (!OKAY)
			goto end_of_job;
		/* ---
		 * local color table
		 * -------------------- */
		if (ncolorbits > 0) {	/* caller provided local colors[] */
			putgifcolortable(gs, colors);	/* emit local color table */
			gs->ncolorbits = ncolorbits;
		} /* and set #bits for local table */
		else
			gs->ncolorbits = gs->ncolorbits_gct;	/*else default to global table */
	}
	/* end-of-if(width>1&&height>1) --- */
	/* ---
	 * lzw-encode the pixels
	 * ------------------------ */
	if (pixels != NULL && npixels > 0) {	/* have pixels to be encoded */
		/* ---
		 * first overlay plaintext, if present
		 * ------------------------------------- */
		if (gs->isplaintext > 0) {	/* have plaintext data */
			if (gs->pt_pixels == NULL)	/* but need new pixel image */
				gs->pt_pixels =
					plainmimetext(gs->pt_data, &gs->pt_width,
								  &gs->pt_height);
			if (gs->pt_pixels != NULL)	/* got new image or still have old */
				overpix = overlay(pixels, gs->width, gs->height,	/* so overlay it */
								  gs->pt_pixels, gs->pt_width,
								  gs->pt_height, gs->pt_left, gs->pt_top,
								  gs->pt_bg, gs->pt_fg);
			if (overpix != NULL)	/*got text pixels overlaid on image */
				pixels = overpix;	/* so that's what we want to see */
		}
		/* --- end-of-if(gs->isplaintext>0) --- */
		/* ---
		 * LZW minimum codesize byte
		 * ------------------------- */
		codesize = max2(2, (gs->ncolorbits /*-1*/ ));	/*local/global table, but >=2 */
		status = putblkbyte(bk, codesize);	/* emit min codesize byte */
		if (!OKAY)
			goto end_of_job;	/* quit if failed */
		/* ---
		 * lzw encode the image
		 * ----------------------- */
		nbytes = encodelzw(gs, codesize, npixels, (BYTE *) pixels);	/* encode pixels */
		status = (nbytes < 1 ? (-1) : putblkbyte(bk, 0));	/* 0 terminates sub-blocks */
		/* ---
		 * reset for next image
		 * ----------------------- */
		gs->npixels = (-1);		/* new image descrip for next image */
		gs->ncontrol = 0;		/* no controlgif() calls issued yet */
		/* --- reset (plain)text overlay --- */
		if (overpix != NULL)
			free((void *) overpix);	/*free text overlay pixels */
		if (gs->isplaintext == 1) {	/*plaintext was for this frame only */
			if (gs->pt_pixels != NULL) {	/*should have allocated pixel image */
				free((void *) (gs->pt_pixels));	/* ...free it */
				gs->pt_pixels = NULL;
			}					/* ...and reset its ptr */
			gs->isplaintext = 0;
		}						/* always reset plaintext flag */
	}							/* --- end-of-if(pixels!=NULL&&npixels>0) --- */
  end_of_job:
	return ((OKAY ? bk->nblkbytes : status));	/* return current #bytes in image */
}								/* --- end-of-function fputgif() --- */

/* ---
 * entry point
 * -------------- */
int endgif(GS * gs)
{
	BK *bk = (gs == NULL ? NULL : &(gs->gifimage));	/* ptr to gif image block */
	SB *sb = (gs == NULL ? NULL : &(gs->gifsubblock));	/* ptr to gif subblock */
	int nblkbytes = (-1);		/* init for error */
	int status = (-1), flushsubblock(), putblkbyte();	/* final write */
	int Trailer = 0x3B;			/* gif trailer byte */
	if (bk != NULL) {			/* have buffers */
		status = flushsubblock(sb);	/* flush anything remaining */
		if (OKAY)
			status = putblkbyte(bk, Trailer);	/* write 0x3B trailer byte */
		if (OKAY)
			nblkbytes = bk->nblkbytes;
	}							/*return tot #bytes in gifimage */
	if (gs != NULL) {			/* have gs */
		if (gs->pt_pixels != NULL)	/* have allocated text pixel image */
			free((void *) (gs->pt_pixels));	/* so free it */
		if (gs->msgfp != NULL && gs->msgfp != stdout) {	/* have open message file */
			fclose(gs->msgfp);	/* so close it */
			*msgfile = '\000';	/* and reset message file */
			msglevel = 0;
		}						/* and message level */
		free((void *) gs);
	}
	return (nblkbytes);			/* return total #bytes or -1=error */
}								/* --- end-of-function endgif() --- */

/* ---
 * entry point
 * -------------- */
void *makegif(int *nbytes, int width, int height, void *pixels,
			  int *colors, int bgindex)
{
/* --- allocations and declarations --- */
	void *gs = NULL, *newgif(),	/* gifsave89 data structure */
		*gifimage = NULL;		/* constructed gif back to caller */
	int ngifbytes = 0, putgif(), endgif();	/* add pixel image to gif */
	int tcolor = (bgindex < 0 ? -bgindex : -1);	/* optional transparent color */
/* --- check input --- */
	if (width < 1 || height < 1 || pixels == NULL	/* quit if no image */
		|| colors == NULL)
		goto end_of_job;		/* or no colors */
	bgindex = absval(bgindex);	/* make sure it's positive (or 0) */
/* --- construct the gif --- */
	if ((gs = newgif(&gifimage, width, height, colors, bgindex))	/* init gsave89 */
		!=NULL) {				/* check if init succeeded */
		if (tcolor >= 0)		/*transparent color index requested */
			controlgif(gs, tcolor, 0, 0, 0);	/* set requested tcolor index */
		putgif(gs, pixels);		/* encode pixel image */
		ngifbytes = endgif(gs);
	} /* all done */
	else {						/*newgif() failed to init gifsave89 */
		if (gifimage != NULL)
			free(gifimage);		/* free gifimage if malloc'ed */
		gifimage = NULL;
	}							/* reset returned ptr */
  end_of_job:
	if (nbytes != NULL)			/* return #bytes in gifimage */
		*nbytes = (gifimage == NULL ? 0 : ngifbytes);	/* or 0 for error */
	return (gifimage);			/* back to caller with gif */
}								/* --- end-of-function makegif() --- */


/* ==========================================================================
 * Functions:	newgifstruct ( void **gifimage, int width, int height )
 *		debuggif ( int dblevel, char *dbfile )
 * Purpose:	gif helper functions,
 *		  newgifstruct allocates and inits a GS structure
 *		  debuggif sets static msglevel and msgfile
 * --------------------------------------------------------------------------
 * Arguments:	--- newgifstruct ---
 *		gifimage (I)	(void **) to user's buffer for image
 *		width (I)	int containing #col pixels for screen
 *		height (I)	int containing #row pixels for screen
 *		--- debuggif ---
 *		dblevel (I)	int containing new static msglevel
 *		dbfile (I)	(char ) containing null-terminated string
 *				with full path/name of new static msgfile
 * --------------------------------------------------------------------------
 * Returns:	( GS * )	newgifstruct returns a ptr to a malloc()'ed
 *				and initialized GS struct whose .gifimage BK
 *				contains the caller's **gifimage,
 *				or NULL for any error.
 *		( int )		debuggif returns 1
 * --------------------------------------------------------------------------
 * Notes:     o
 * ======================================================================= */
/* ---
 * entry point
 * -------------- */
GS *newgifstruct(void **gifimage, int width, int height)
{
	GS *gs = (gifimage == NULL ? NULL : (GS *) malloc(sizeof(GS)));
	if (gs == NULL)
		goto end_of_job;		/* failed to allocate memory */
	memset((void *) gs, 0, sizeof(GS));	/* zero out entire struct */
/* ---
 * debugging
 * ------------ */
	gs->msglevel = msglevel;	/* desired verbosity */
	gs->msgfp = MSGFP;			/* debug file ptr (default) */
	if (msglevel > 1 && *msgfile != '\000')	/* redirect debug output to file */
		if ((gs->msgfp = fopen(msgfile, "a"))	/* append debug msgs to file */
			== NULL)
			gs->msgfp = MSGFP;	/* back to default if fopen fails */
	if (ismsgprint(gs, 1))		/* messages enabled? */
		fprintf(gs->msgfp, "%s\n", copyright);	/* always display gnu/gpl copyright */
/* ---
 * gifimage block buffer
 * ------------------------ */
	gs->gifimage.block = (BYTE **) gifimage;	/*user's buffer for returned image */
	*(gs->gifimage.block) = (BYTE *) NULL;	/* nothing allocated yet */
	gs->gifimage.maxblkbytes = 0;	/* #bytes currently allocated */
	gs->gifimage.nblkbytes = 0;	/* #bytes currently in gifimage */
/* ---
 * subblock buffer
 * ----------------- */
	gs->gifsubblock.block = &(gs->gifimage);	/* ptr back to parent block */
	gs->gifsubblock.nsubbytes = 0;	/* #bytes already in gif subblock */
	gs->gifsubblock.nsubbits = 0;	/* #bits already in current byte */
	gs->gifsubblock.index = (-1);	/* index# **not** wanted */
/* ---
 * LZW string table
 * ------------------- */
	gs->nstrings = 0;			/* #strings in lzw string table */
/* ---
 * additional control data
 * -------------------------- */
	gs->width = max2(0, min2(9999, width));	/* #cols for screen in pixels */
	gs->height = max2(0, min2(9999, height));	/* #rows for screen in pixels */
	gs->ncolorbits_gct = gs->ncolorbits = 0;	/* no color table(s) yet */
	gs->bgindex = 0;			/* no colortable bgindex yet */
	gs->npixels = (-1);			/* no image descriptor yet */
	gs->ncontrol = 0;			/* no controlgif() calls issued yet */
	gs->isanimated = 0;			/* set true if animategif() called */
	gs->delay = 0;				/* default animation frame delay */
	gs->tcolor = (-1);			/*  " animation transparency index */
	gs->disposal = 2;			/*  " animation disposal method */
/* ---
 * plaintext control data
 * ------------------------- */
	gs->isplaintext = 0;		/* plaintext flag, 1 if present */
	gs->pt_left = gs->pt_top = 0;	/* top-left corner col;row */
	gs->pt_bg = gs->pt_fg = 0;	/* bg,fg colortable indexes */
	gs->pt_width = gs->pt_height = 0;	/* bg,fg colortable indexes */
	memset(gs->pt_data, 0, 1024);	/* local copy of user text data */
	gs->pt_pixels = NULL;		/* no pixels allocated */
  end_of_job:
	return (gs);				/* new struct or NULL for error */
}								/* --- end-of-function newgifstruct() --- */

/* ---
 * entry point
 * -------------- */
int debuggif(int dblevel, char *dbfile)
{
	msglevel = max2(0, min2(999, dblevel));	/* 0<=msglevel<=999 */
	*msgfile = '\000';			/* reset msgfile */
	if (dbfile != NULL)			/* have dbfile arg */
		if (*dbfile != '\000') {	/* that's not an empty string */
			strncpy(msgfile, dbfile, 127);	/* set msgfile (max 127 chars) */
			msgfile[127] = '\000';
		}						/* make sure it's null-terminated */
	return (1);
}								/* --- end-of-function debuggif() --- */

/* ---
 * entry point
 * -------------- */
int fprintpixels(GS * gs, int format, void *pixels)
{
/* -------------------------------------------------------------------------
Allocations and Declarations
-------------------------------------------------------------------------- */
	static int display_width = 72;	/* max columns for display */
	char scanline[999];			/* ascii image for one scan line */
	char fgchar = '*', bgchar = '.';	/* foreground/background chars */
	static char *hexchars = "0123456789ABCDEFxxxxxxxxxx";	/* for hex display */
	int scan_width = 0;			/* #chars in scan (<=display_width) */
	int irow = 0, locol = 0, hicol = (-1);	/* height index, width indexes */
	FILE *fp = gs->msgfp;		/* pointer to output device */
/* --------------------------------------------------------------------------
initialization
-------------------------------------------------------------------------- */
/* --- redirect null fp --- */
	if (fp == NULL)
		fp = MSGFP;				/* default fp if null */
/* --- adjust display for hexdump --- */
	if (format != 1)
		display_width /= 3;		/* "xx xx xx..." for hex display */
/* --- header line --- */
	fprintf(fp, "fprintpixels> width=%d height=%d ...\n", gs->width,
			gs->height);
/* --------------------------------------------------------------------------
display ascii dump of image raster (in segments if display_width < gr->width)
-------------------------------------------------------------------------- */
	while ((locol = hicol + 1) < gs->width) {	/*start where prev segment left off */
		/* --- set hicol for this pass (locol set above) --- */
		hicol += display_width;	/* show as much as display allows */
		if (hicol >= gs->width)
			hicol = gs->width - 1;	/*but not more than pixels */
		scan_width = hicol - locol + 1;	/* #chars in this scan */
		if (locol > 0)
			fprintf(fp, "----------\n");	/*separator between segments */
		/* ------------------------------------------------------------------------
		   display all scan lines for this local...hicol segment range
		   ------------------------------------------------------------------------ */
		for (irow = 0; irow < gs->height; irow++) {	/* all scan lines for col range */
			/* --- allocations and declarations --- */
			int lopix = irow * gs->width + locol;	/* index of 1st pixel in this scan */
			int ipix;			/* pixel index for this scan */
			/* --- set chars in scanline[] based on pixels --- */
			memset(scanline, ' ', 255);	/* blank out scanline */
			for (ipix = 0; ipix < scan_width; ipix++) {	/* set each char or hexbyte */
				int pixel = (int) (((BYTE *) pixels)[lopix + ipix]);	/* current pixel */
				int isbg = (pixel == gs->bgindex ? 1 : 0);	/* background pixel */
				if (format == 1)
					scanline[ipix] = (isbg ? bgchar : fgchar);
				else {			/* dump hex byte */
					int icol = 3 * ipix;	/* first col for display */
					if (isbg)
						scanline[icol] = scanline[icol + 1] = bgchar;
					else {
						scanline[icol] = hexchars[(pixel / 16) % 16];
						scanline[icol + 1] = hexchars[pixel % 16];
					}
				}				/* --- end-of-if/else(format==1) --- */
			}					/* --- end-of-for(ipix) --- */
			/* --- display completed scan line --- */
			fprintf(fp, "%.*s\n", scan_width * (format == 1 ? 1 : 3),
					scanline);
		}						/* --- end-of-for(irow) --- */
	}							/* --- end-of-while(hicol<gs->width) --- */
/* -------------------------------------------------------------------------
Back to caller with 1=okay, 0=failed.
-------------------------------------------------------------------------- */
	return (1);
}								/* --- end-of-function fprintpixels() --- */


/* ==========================================================================
 * Function:	plainmimetext ( char *expression, int *width, int *height )
 * Purpose:	obtain pixel image corresponding to expression
 *		by "calling" mimetex, using wget,
 *		for pbm-style bitmap of expression,
 *		and then pixelizing it.
 * --------------------------------------------------------------------------
 * Arguments:	expression (I)	char * to null-terminated string
 *				containing any TeX expression
 *				recognized by mimeTeX
 *				See http://www.forkosh.com/mimetex.html
 *		width, height (O)  int * pointers returning
 *				width,height of generated pixelized image
 * --------------------------------------------------------------------------
 * Returns:	( BYTE * )	newly allocated pixel array with pixelized
 *				image of input expression, rendered
 *				by mimetex, or NULL for any error.
 * --------------------------------------------------------------------------
 * Notes:     o	Caller must free(returned_pixels) when finished with them.
 * ======================================================================= */
/* ---
 * entry point
 * -------------- */
BYTE *plainmimetext(char *expression, int *width, int *height)
{
/* ---
 * allocations and declarations
 * ------------------------------- */
	BYTE *pixels = NULL;		/* returned pixels */
	char command[2048],			/* complete popen(command,"r")... */
	*execwget = WGET			/*...begins with /path/to/wget */
		" -q -O - "				/*...followed by wget options */
		"\"" MIMETEX "?\\pbmpgm{1} ";	/*..."mimetex.cgi?\pbmpgm{1}... */
	FILE *wget = NULL;			/* wget's stdout */
	char pipeline[512],			/* fgets(pipeline,511,wget) buffer */
	*pipeptr = NULL, *endptr = NULL;	/* strtol ptr to 1st char after num */
	char *magicnumber = "P1";	/* filetype identification */
	int gotmagic = 0;			/* set true when magicnumber found */
	int ncols = 0, nrows = 0,	/* local width,height from wget */
		ipixel = 0, npixels = 0,	/* and index, npixels=ncols*nrows */
		pixval = 0;				/* pixval=strtol() from pipeline */
/* ---
 * initialization
 * ----------------- */
/* --- check arguments --- */
	if (width != NULL)
		*width = 0;				/* init for error */
	if (height != NULL)
		*height = 0;			/* init for error */
	if (expression == NULL)
		goto end_of_job;		/* nothing to pixelize */
	if (*expression == '\000')
		goto end_of_job;		/* still nothing to pixelize */
	if (strlen(expression) > 1536)
		goto end_of_job;		/*too much (sanity check) */
/* ---
 * open pipe to wget
 * -------------------- */
/* --- first construct popen() command --- */
	strcpy(command, execwget);	/* shell command plus mimetex url */
	strcat(command, expression);	/* followed by caller's expression */
	strcat(command, "\"");		/* and closing " */
/* --- issue wget command and capture its stdout --- */
	if ((wget = popen(command, "r"))	/* issue command and capture stdout */
		== NULL)
		goto end_of_job;		/* or quit if failed */
/* ---
 * read the first few lines to get width, height
 * ------------------------------------------------ */
	while (1) {					/* read lines (shouldn't find eof) */
		if (fgets(pipeline, 511, wget) == NULL)
			goto end_of_job;	/*premature eof */
		if (gotmagic) {			/* next line has "width height" */
			ncols = (int) strtol(pipeline, &endptr, 10);	/* convert width */
			nrows = (int) strtol(endptr, NULL, 10);	/* and convert height */
			break;
		}						/* and we're ready to pixelize... */
		if (strstr(pipeline, magicnumber) != NULL)	/* found magicnumber */
			gotmagic = 1;		/* set flag */
	}							/* --- end-of-while(1) --- */
/* ---
 * allocate pixels and xlate remainder of pipe
 * ---------------------------------------------- */
/* --- allocate pixels --- */
	npixels = nrows * ncols;	/* that's what we need */
	if (npixels < 1 || npixels > 999999)
		goto end_of_job;		/* sanity check */
	if ((pixels = (BYTE *) malloc(npixels))	/* ask for it if not insane */
		== NULL)
		goto end_of_job;		/* and quit if request denied */
/* --- xlate remainder of pipe --- */
	while (1) {					/* read lines (shouldn't find eof) */
		/* --- read next line --- */
		if (fgets(pipeline, 511, wget) == NULL) {	/* premature eof */
			free(pixels);
			pixels = NULL;		/* failed to xlate completely */
			goto end_of_job;
		}
		/* free, reset return ptr, quit */
		/* --- xlate 0's and 1's on the line --- */
		pipeptr = pipeline;		/* start at beginning of line */
		while (ipixel < npixels) {
			while (*pipeptr != '\000')	/* skip leading non-digits */
				if (isdigit(*pipeptr))
					break;		/* done at first digit */
				else
					pipeptr++;	/* or skip leading non-digit */
			if (*pipeptr == '\000')
				break;			/* no more numbers on this line */
			pixval = (int) strtol(pipeptr, &endptr, 10);	/*convert next num to 0 or 1 */
			pixels[ipixel++] = (BYTE) pixval;	/* let's hope it's 0 or 1, anyway */
			pipeptr = endptr;	/* move on to next number */
		}						/* --- end-of-while(ipixels<npixels) --- */
		if (ipixel >= npixels)
			break;				/* pixelized image complete */
	}							/* --- end-of-while(1) --- */
/* ---
 * end-of-job
 * ------------- */
/* --- dropped through if completed successfully --- */
	if (width != NULL)
		*width = ncols;			/* supply width to caller */
	if (height != NULL)
		*height = nrows;		/* supply height to caller */
/* --- or jumped here if failed --- */
  end_of_job:
	if (wget != NULL)
		pclose(wget);			/* close pipe (if open) */
	return (pixels);			/* back with pixelized expression */
}								/* --- end-of-function plainmimetext() --- */


/* ==========================================================================
 * Function:	overlay ( pix1,w1,h1, pix2,w2,h2, col1,row1, bg,fg )
 * Purpose:	gif helper function, overlays pix2 onto pix1,
 *		with pix2's upper-left corner at col1,row1 of pix1,
 *		returning a new pix image with pix1's dimensions
 *		(pix2 is "truncated" if it extends beyond pix1's boundaries).
 * --------------------------------------------------------------------------
 * Arguments:	pix1 (I)	BYTE * to original image, presumably
 *				passed to putgif() by user
 *		w1, h1 (I)	ints containing width, height of pix1 image
 *		pix2 (I)	BYTE * to overlay image, presumably
 *				containing "plaintext" or whatever
 *		w1, h1 (I)	ints containing width, height of pix2 image
 *		col1, row1 (I)	ints containing upper-left corner on pix1
 *				where pix2's first byte is placed,
 *				i.e., col1=0,row1=0 for upper-left corners
 *				to coincide. Special alignment directives:
 *				  center horiz,vert if col1<0,row1<0
 *				  place at right,bottom if col1>w1,row1>h1
 *		bg, fg (I)	ints containing
 *				  bg color index for 0's in pix2
 *				  fg color index for 1's in pix2
 * --------------------------------------------------------------------------
 * Returns:	( BYTE * )	newly allocated pix array with pix1's
 *				w1,h1-dimensions, containing a copy of pix1,
 *				overlaid with pix2, as above.
 *				any out-of-bounds pix2 pixels are discarded.
 * --------------------------------------------------------------------------
 * Notes:     o	Caller must free(returned_pix) when finished with them.
 * ======================================================================= */
/* ---
 * entry point
 * -------------- */
BYTE *overlay(BYTE * pix1, int w1, int h1, BYTE * pix2, int w2, int h2,
			  int col1, int row1, int bg, int fg)
{
/* ---
 * allocations and declarations
 * ------------------------------- */
	int npixels = (pix1 == NULL || pix2 == NULL || w1 < 1 || w2 < 1 ? 0 : w1 * h1);	/* #pixels */
	BYTE *pix = (npixels < 1
				 || npixels > 999999 ? NULL : (BYTE *) malloc(npixels));
	int col = 0, row = 0;		/* pix2 col,row indexes */
/* ---
 * initialization
 * ----------------- */
/* --- check args --- */
	if (pix == NULL)
		goto end_of_job;		/* bad args or malloc failed */
/* --- init pix = pix1 --- */
	memcpy((void *) pix, (void *) pix1, npixels);	/* pix = pix1 to begin with */
/* --- centering requested? --- */
	if (col1 < 0)
		col1 = max2(0, (w1 - w2 + 1) / 2);	/* horizontal centering */
	if (row1 < 0)
		row1 = max2(0, (h1 - h2 + 1) / 2);	/*  vertical centering  */
/* --- check right, bottom edges --- */
	if (col1 + w2 > w1)
		col1 = max2(0, w1 - w2);	/* text went past right of image */
	if (row1 + h2 > h1)
		row1 = max2(0, h1 - h2);	/* text went below bottom */
/* ---
 * now overlay pix2
 * ------------------- */
	for (row = 0; row < h2; row++) {	/* loop over each pix2 row */
		int ipix1 = w1 * (row1 + row) + col1,	/* first pix1 col for this row */
			ipix2 = w2 * row;	/* first pix2 col for this row */
		if (row1 + row >= h1)
			break;				/* past bottom edge of pix1 */
		for (col = 0; col < w2; col++) {	/* and over each col in row */
			if (col1 + col >= w1)
				break;			/* past right edge of pix1 */
			pix[ipix1 + col] =	/* overlay pix2 pixel onto pix... */
				(pix2[ipix2 + col] == 0 ? bg : fg);	/* with bg index if pix2=0, else fg */
		}						/* --- end-of-for(col) --- */
	}							/* --- end-of-for(row) --- */
  end_of_job:
	return (pix);
}								/* --- end-of-function overlay() --- */


/* ==========================================================================
 * Functions:	putgifcolortable  ( GS *gs, int *colors )
 *		putgifapplication ( GS *gs, GIFAPPLICATION *ga )
 * Purpose:	write gif blocks
 * --------------------------------------------------------------------------
 * Arguments:	gs (I)		(GS *) to gifsave89 data struct
 *	For putgifcolortable()...
 *		colors (I)	int * containing r,g,b, r,g,b, ..., -1
 *				values 0-255 for color index 0, 1, ...,
 *				up to max 256, terminated by any negative
 *				number. Any trailing ...,r,-1 or ...,r,g,-1
 *				is truncated. And the number of colors
 *				supplied must be a power of 2; if not, it's
 *				reduced to the next lower number that is.
 *	For putgifapplication()...
 *		ga (I)		(GIFAPPLICATION *) to application extension
 *				containing header data to be written.
 *				Caller must write actual data subblocks.
 * --------------------------------------------------------------------------
 * Returns:	( int )		putgifcolortable returns #bits+1 required
 *				to store the largest colors[] index,
 *				others return 1 for successful completion,
 *				or all return -1 for any error.
 * --------------------------------------------------------------------------
 * Notes:     o	structs can't be output as, e.g.,
 *		  memcpy((void *)image,(void *)&struct,sizeof(struct));
 *		unless we're absolutely sure the struct is packed.
 *		Since that's unfortunately not completely portably possible,
 *		these funcs output each member element separately.
 *	      o	putgifcolortable() can be called with GS=NULL, in which
 *		case it outputs nothing, but returns the #bits+1 required
 *		to store the largest colors[] index.
 * ======================================================================= */
/* ---
 * entry point
 * -------------- */
//int putgifcolortable(GS * gs, unsigned char *colors, int ncolors)
int putgifcolortable(GS * gs, int *colors)
{
/* --- allocations and declarations --- */
	int ncolors = 0,			/* #colors in colors[] table */
		ncolorbits = (0);		/* #bits+1 for colors[] index */
	BK *bk = (gs == NULL ? NULL : &(gs->gifimage));	/* ptr to gif image block */
	int status = 1, putblkbyte();	/* one byte for each rgb component */
/* --- force #colors to power of 2, count #bits for color index --- */
	if (colors != NULL) {		/* caller supplied r,g,b colors[] */
		int pow2 = 2;			/* power of 2 strictly > ncolors */
		while (colors[ncolors] >= 0)
			ncolors++;			/* count r,g,b, r,g,b, ..., -1 */
		ncolors = (ncolors / 3);	/* truncate trailing r or r,g */
		ncolors = min2(256, max2(0, ncolors));	/* 0 <= ncolors <= 256 */
		if (ncolors > 1) {		/* at least 2 colors */
			while (ncolors >= pow2) {	/* power of 2 strictly > ncolors */
				pow2 *= 2;
				ncolorbits++;
			}					/* bump power of 2 and #bits */
			ncolors = pow2 / 2;
		}						/* scale back ncolors */
	}							/* --- end-of-if(colors!=NULL) --- */
	if (gs != NULL)				/* caller wants color table written */
		if (ncolors > 1) {		/* and provided colors[] to write */
			int irgb = 0;		/* colors[] index for r,g,b triples */
			if (ismsgprint(gs, 32))	/* (debug) msglevel>=32 */
				fprintf(gs->msgfp, "putgifcolortable> %d colors...\n",
						ncolors);
			for (irgb = 0; irgb < 3 * ncolors; irgb++) {	/* each component of each color */
				if (OKAY)
					status = putblkbyte(bk, colors[irgb]);	/* write byte */
				else
					break;		/* quit if failed */
				if (ismsgprint(gs, 32))	/* (debug) msglevel>=32 */
					if (irgb % 3 == 0) {	/* one color per set of 3 r,g,b */
						char newline =	/* 6 per line */
							((irgb + 3) % 18 == 0
							 || irgb + 3 == 3 * ncolors ? '\n' : ' ');
						fprintf(gs->msgfp, "%3d:%2x,%2x,%2x%c", irgb / 3,
								colors[irgb], colors[irgb + 1],
								colors[irgb + 2], newline);
					}
			}					/* --- end-of-for(irgb) --- */
			if (!OKAY)
				ncolorbits = (-1);	/* signal failure */
		}
	/* --- end-of-if(gs!=NULL&&ncolors>1) --- */
	/*end_of_job: */
	return (ncolorbits);		/* return #bits for color index */
}								/* --- end-of-function putgifcolortable() --- */

/* ---
 * entry point
 * -------------- */
int putgifapplication(GS * gs, GIFAPPLICATION * ga)
{
/* --- allocations and declarations --- */
	int status = (1);			/* init signalling okay */
	int Introducer = 0x21;		/* always 0x21 */
	int Label = 0xFF;			/* always 0xFF */
	int BlockSize = 0x0B;		/* always 0x0B */
	int Terminator = 0x00;		/* always 0x00 */
	BK *bk = (gs == NULL ? NULL : &(gs->gifimage));	/* ptr to gif image block */
	int putblkbytes(), putblkbyte(), putblkword();	/* write to image block */
/* ---
 * Graphic Control Extension Block
 * ---------------------------------- */
	if (ga == NULL)
		goto end_of_job;
	if (OKAY)
		status = putblkbyte(bk, Introducer);
	if (OKAY)
		status = putblkbyte(bk, Label);
	if (OKAY)
		status = putblkbyte(bk, BlockSize);
	if (OKAY)
		status = putblkbytes(bk, ga->Identifier, 8);
	if (OKAY)
		status = putblkbytes(bk, ga->AuthentCode, 3);
	if (OKAY && 0)
		status = putblkbyte(bk, Terminator);
  end_of_job:
	return (status);			/* signal 1=success/-1=failure */
}								/* --- end-of-function putgifapplication() --- */


/* ==========================================================================
 * Functions:	encodelzw ( GS *gs, int codesize, int nbytes, BYTE *data )
 *		clearlzw  ( GS *gs, int codesize )
 *		putlzw    ( GS *gs, int index, int byte )
 *		getlzw    ( GS *gs, int index, int byte )
 * Purpose:	lzw encoder
 * --------------------------------------------------------------------------
 * Arguments:	gs (I)		(GS *) to gifsave89 data struct
 *				containing BLOCK buffer where lzw-encoded
 *				data will be stored.
 *	For encodelzw(), clearlzw()...
 *		codesize (I)	int containing initial string table codesize
 *	For encodelzw()...
 *		nbytes (I)	integer containing #bytes to be lzw-encoded
 *		data (I)	(BYTE *) pointer to bytes to be lzw-encoded
 *	For putlzw(), getlzw()...
 *		index (I)	int containing string table index of
 *				current prefix string
 *		byte (I)	(int)-type integer interpreted as (BYTE)byte
 *				containing last byte after prefix
 * --------------------------------------------------------------------------
 * Returns:	( int )
 * --------------------------------------------------------------------------
 * Notes:     o
 * ======================================================================= */
/* ---
 * entry point
 * -------------- */
int encodelzw(GS * gs, int codesize, int nbytes, BYTE * data)
{
	int clearcode = POW2(codesize),
		endofdata = clearcode + 1,
		codebits = codesize + 1, limit = POW2(codebits) - 1;
	int prefix = NEXTFIRST, hashindex = 0;
	int clearlzw(), putlzw(), getlzw(), putsubblock(), flushsubblock();
	SB *sb = (gs == NULL ? NULL : &(gs->gifsubblock));	/* ptr to gif subblock */
	int byte = 0, ibyte = 0,	/* bytes from input data[] */
		maxbyte = (gs == NULL ? 0 : POW2(gs->ncolorbits) - 1),	/* max index */
		nblkbytes = (gs == NULL ? 0 : gs->gifimage.nblkbytes);	/* size at start */
/* ---
 * check input
 * -------------- */
	if (gs == NULL)
		goto end_of_job;		/* no gs data struct */
	if (data == NULL || nbytes < 1)
		goto end_of_job;		/* no data to encode */
/* ---
 * first init string table,
 * and output code telling the decoder to clear it, too
 * ---------------------------------------------------- */
	clearlzw(gs, codesize);		/* clear lzw string table */
	putsubblock(sb, clearcode, codebits);	/* tell decoder to clear it */
/* --- pack image --- */
	for (ibyte = 0; ibyte < nbytes; ibyte++) {
		byte = ((int) data[ibyte]);	/* current data byte */
		if (byte > maxbyte)
			byte = 0;			/* default if byte too big */
		/* ---
		 * if the string already exists in the table,
		 * just make this string the new prefix
		 * ------------------------------------------ */
		if ((hashindex = getlzw(gs, prefix, byte)) != HASHFREE) {
			prefix = hashindex;	/* oldprefix+byte already in table */
			continue;
		}
		/* try adding next byte to prefix */
		/* ---
		 * otherwise, the string does not exist in the table
		 * first, output code of the old prefix
		 * ------------------------------------------------- */
		putsubblock(sb, prefix, codebits);
		/* ---
		 * second, add the new string (prefix + new byte)
		 * to the string table (note variable length table)
		 * --------------------------------------------------- */
		if (putlzw(gs, prefix, byte) > limit) {	/* too many strings for codesize */
			if (++codebits > CODEBITS) {
				putsubblock(sb, clearcode, codebits - 1);
				clearlzw(gs, codesize);
				codebits = codesize + 1;
			}
			limit = POW2(codebits) - 1;
		}
		/* ---
		 * finally, reset prefix to a string containing only the new byte
		 * (since all one-byte strings are in the table, don't check for it)
		 * ------------------------------------------------------------------- */
		prefix = byte;
	}							/* --- end-of-for(ibyte) --- */
/* -- end of data, write last prefix --- */
	if (prefix != NEXTFIRST)	/*one-byte strings already in table */
		putsubblock(sb, prefix, codebits);
/* --- write end-of-data signal, and flush the buffer --- */
	putsubblock(sb, endofdata, codebits);
	flushsubblock(sb);
  end_of_job:
	return ((gs == NULL ? 0 : gs->gifimage.nblkbytes - nblkbytes));	/*#bytes encoded */
}								/* --- end-of-function encodelzw() --- */

/* ---
 * entry point
 * -------------- */
int clearlzw(GS * gs, int codesize)
{
	int ncodes = RESCODES + ((int) (1 << codesize)),	/* # one-char codes */
		putlzw(),				/* insert all one-char codes */
		index;					/* table index */
/* --- no strings in the table --- */
	gs->nstrings = 0;
/* --- mark entire hashtable as free --- */
	for (index = 0; index < HASHSIZE; index++)
		gs->strhash[index] = HASHFREE;
/* --- insert 2**codesize one-char strings, and reserved codes --- */
	for (index = 0; index < ncodes; index++)
		putlzw(gs, NEXTFIRST, index);
	return (1);					/* back to caller */
}								/* --- end-of-function clearlzw() --- */

/* ---
 * entry point
 * -------------- */
int putlzw(GS * gs, int index, int byte)
{
	int newindex = gs->nstrings,	/* init returned index */
		hashindex = HASH(index, byte);	/* init hash for new string */
/* --- string table out-of-memory? --- */
	if (gs->nstrings >= NSTRINGS) {	/* too many strings in table */
		newindex = HASHFREE;
		goto end_of_job;
	}
	/* signal error to caller */
	/* --- find free postion in string table --- */
	while (gs->strhash[hashindex] != HASHFREE)
		hashindex = (hashindex + HASHSTEP) % HASHSIZE;
/* --- insert new string and bump string table index --- */
	gs->strhash[hashindex] = gs->nstrings;
	gs->strbyte[gs->nstrings] = ((BYTE) byte);
	gs->strnext[gs->nstrings] = (index != NEXTFIRST) ? index : NEXTFIRST;
	gs->nstrings++;				/* bump string table index */
/* --- return index of new string --- */
  end_of_job:
	return (newindex);
}								/* --- end-of-function putlzw() --- */

/* ---
 * entry point
 * -------------- */
int getlzw(GS * gs, int index, int byte)
{
	int hashindex = HASH(index, byte),	/* init the hash index */
		nextindex;
/* ---
 * if requested index is NEXTFREE (=0xFFFF), just return byte since
 * all one-character strings have their byte value as their index
 * ---------------------------------------------------------------- */
	if (index == NEXTFIRST)
		return (((int) byte));
/* ---
 * otherwise, search the string table until the string is found,
 * or we find HASHFREE, in which case the string does not exist
 * ------------------------------------------------------------- */
	while ((nextindex = gs->strhash[hashindex]) != HASHFREE) {
		if (gs->strnext[nextindex] == index
			&& gs->strbyte[nextindex] == ((BYTE) byte))
			return (nextindex);
		hashindex = (hashindex + HASHSTEP) % HASHSIZE;
	}
/* ---
 * string not in table
 * ------------------- */
	return (HASHFREE);			/* signal "error" to caller */
}								/* --- end-of-function getlzw() --- */


/* ==========================================================================
 * Functions:	putblkbytes ( BK *bk, BYTE *bytes, int nbytes )
 *		putblkbyte  ( BK *bk, int byte )
 *		putblkword  ( BK *bk, int word )
 * Purpose:	write bytes and integer words to data block,
 *		and reallocate block as necessary
 * --------------------------------------------------------------------------
 * Arguments:	bk (I/O)	(BK *) to BLOCK data struct, which
 *				contains BYTE **block to buffer that's
 *				first malloc()'ed and then realloc()'ed
 *				as needed.
 *	For putblkbytes()...
 *		bytes (I)	(BYTE *) pointer to bytes to be stored
 *		nbytes (I)	integer containing #bytes to be stored
 *	For putblkbyte()...
 *		byte (I)	(int)-type integer interpreted as (BYTE)byte
 *				to be stored in next buffer byte
 *	For putblkword()...
 *		word (I)	(int)-type integer whose two least
 *				significant byte values are to be stored in
 *				little-endian order (regardless of host order)
 * --------------------------------------------------------------------------
 * Returns:	( int )		putblkbytes returns #bytes stored,
 *				putblkbyte  returns 1 byte stored,
 *				putblkword  returns 1 word stored,
 *				or all return -1 for any error.
 * --------------------------------------------------------------------------
 * Notes:     o
 * ======================================================================= */
/* ---
 * entry point
 * -------------- */
int putblkbytes(BK * bk, BYTE * bytes, int nbytes)
{
	int status = (-1);			/* init to signal error */
	if (bk == NULL || bytes == NULL || nbytes < 1)
		goto end_of_job;		/* check args */
/* --- realloc gifimage as necessary --- */
	while (bk->nblkbytes + nbytes > bk->maxblkbytes) {	/* more room for nbytes */
		int newsize =
			(bk->nblkbytes <
			 1 ? MAXBLKBYTES : (3 * (bk->maxblkbytes)) / 2);
		void *newblock = realloc((void *) (*(bk->block)), newsize);
		if (newblock == NULL)
			goto end_of_job;	/* failed to realloc */
		*(bk->block) = (BYTE *) newblock;	/* store reallocated block ptr */
		bk->maxblkbytes = newsize;
	}							/* and its new larger size */
/* --- copy caller's bytes --- */
	memcpy((void *) (*(bk->block) + bk->nblkbytes), bytes, nbytes);	/* copy bytes */
	bk->nblkbytes += nbytes;	/* bump byte count */
	status = nbytes;			/* signal success */
  end_of_job:
	return (status);			/* back with #bytes or -1=error */
}								/* --- end-of-function putblkbytes() --- */

/* ---
 * entry point
 * -------------- */
int putblkbyte(BK * bk, int byte)
{
	int putblkbytes();			/* write byte */
	BYTE thisbyte[2];			/* place caller's byte in string */
	thisbyte[0] = ((BYTE) byte);	/* caller's byte */
	thisbyte[1] = '\000';		/* (not really necessary) */
	return (putblkbytes(bk, thisbyte, 1));	/* back with #bytes=1 or -1=error */
}								/* --- end-of-function putblkbyte() --- */

/* ---
 * entry point
 * -------------- */
int putblkword(BK * bk, int word)
{
	int putblkbytes();			/* write bytes */
/* --- byte values regardless of little/big-endian ordering on host --- */
	int lobyte = (abs(word)) % 256,	/* least significant byte */
		hibyte = ((abs(word)) / 256) % 256;	/* next least significant byte */
	BYTE thisword[2];			/* buffer for word's bytes */
/* --- store little-endian ordering (least significant byte first) --- */
	thisword[0] = ((BYTE) lobyte);	/* low-order byte written first */
	thisword[1] = ((BYTE) hibyte);	/* high-order byte written second */
	return (putblkbytes(bk, thisword, 2));	/* store word */
}								/* --- end-of-function putblkword() --- */


/* ==========================================================================
 * Functions:	putsubblock   ( SB *sb, int bits,  int nbits )
 *		putsubbytes   ( SB *sb, int bytes, int nbytes )
 *		flushsubblock ( SB *sb )
 * Purpose:	Bitwise operations to construct gif subblocks...
 *		
 * --------------------------------------------------------------------------
 * Arguments:	sb (I/O)	(SB *) to SUBBLOCK data struct
 *		bits (I)	int whose low-order bits
 *				are to be added at the end of subblock,
 *				and then eventually flushed into block,
 *				preceded by a byte count of the subblock
 *		nbits (I)	int containing number of bits to be stored
 *	For putsubbytes...
 *		bytes (I)	BYTE * for sequence of 8-bit bytes
 *				to be added at the end of subblock
 *		nbytes (I)	int containing number of bytes to be stored
 * --------------------------------------------------------------------------
 * Returns:	( int )		putsubblock    returns nbits,
 *				putsubbytes    returns nbytes,
 *				flushsubblock  returns #bytes flushed,
 *				or both return -1 for any error.
 * --------------------------------------------------------------------------
 * Notes:     o	gif format stores image data in 255-byte subblocks,
 *		each preceded by a leading byte containing
 *		the number of data bytes in the subblock that follows.
 *		That's typically several 255-byte subblocks, and
 *		a final short subblock. That last subblock must be
 *		followed by a 0-byte (the byte count of the non-existent
 *		next subblock) signalling end-of-data to the gif decoder.
 *	      o	gif image data is typically lzw-encoded, done by
 *		the encodelzw() function above, and written several
 *		bits at a time, rather than in complete bytes.
 *	      o	putsubblock() accommodates this behavior by
 *		accumulating bits into complete bytes, and then
 *		flushsubblock() writes full 255-byte subblocks
 *		(preceded by byte count) as necessary, whenever
 *		called by putsubblock().
 *	      o	the final short subblock is written when flushsubblock()
 *		is called the by user, after his last call to putsubblock().
 *		It's also the user's responsibility to write the
 *		trailing 0-byte after calling flushsubblock().
 *		That's done by calling putblkbyte(bk,0) with the
 *		parent BLOCK *bk subblocks were written to.
 * ======================================================================= */
/* ---
 * entry point
 * -------------- */
int putsubblock(SB * sb, int bits, int nbits)
{
	int status = 0;				/* returns nbits or signals error */
	int flushsubblock();		/* flush subblock when full */
	int indexlen = (sb == NULL ? 0 : (sb->index >= 0 ? 1 : 0));	/* >=0 to write index# */
	if (sb != NULL)				/* make sure we have subblock */
		while (1) {				/* finished after nbits */
			/* ---
			 * flush subblock whenever full
			 * ---------------------------- */
			if (sb->nsubbytes + indexlen >= SUBBLOCKSIZE)	/* subblock full */
				if (flushsubblock(sb) == (-1)) {	/* flush subblock, check for error */
					status = (-1);
					break;
				}
			/* and signal error on failure */
			/* ---
			 * remaining bits fit in current byte, or fill current byte and continue
			 * --------------------------------------------------------------------- */
			if (nbits < 1)
				break;			/* done after all bits exhausted */
			else {				/* or still have bits for gifblock */
				/* ---fieldbits is lesser of remaining nbits, or #bits left in byte--- */
				int freebits = BITSPERBYTE - sb->nsubbits,	/*#unused bits in this byte */
					fieldbits = (nbits <= freebits ? nbits : freebits);	/*#bits for this byte */
				/* subblock[nsubbytes] |= ((bits&(bitmask(fieldbits))) << nsubbits); */
				putbitfield(sb->subblock[sb->nsubbytes], sb->nsubbits,
							fieldbits, bits);
				bits >>= fieldbits;	/* shift out the bits we just used */
				nbits -= fieldbits;	/* fewer bits remaining */
				sb->nsubbits += fieldbits;	/* more bits used in current byte */
				if (sb->nsubbits >= BITSPERBYTE) {	/*byte exhausted, move on to next */
					sb->nsubbits = 0;
					sb->nsubbytes++;
				}				/* first bit of next byte */
				status += fieldbits;
			}					/* update status */
		}						/* --- end-of-while(1) --- */
	return (status);			/* back with #bits or -1=error */
}								/* --- end-of-function putsubblock() --- */

/* ---
 * entry point
 * -------------- */
int putsubbytes(SB * sb, BYTE * bytes, int nbytes)
{
	int status = 0;				/* returns nbits or signals error */
	int ibyte = 0, putsubblock();	/* just store each byte as 8 bits */
	if (sb != NULL && bytes != NULL && nbytes > 0)	/* data check */
		for (ibyte = 0; ibyte < nbytes; ibyte++) {	/* put one 8-bit byte at a time */
			status = putsubblock(sb, ((int) (bytes[ibyte])), BITSPERBYTE);	/*thisbyte */
			if (!OKAY)
				break;
		}						/* quit if failed */
	return ((OKAY ? nbytes : status));	/* back with #bytes or -1=error */
}								/* --- end-of-function putsubbytes() --- */

/* ---
 * entry point
 * -------------- */
int flushsubblock(SB * sb)
{
	int status = (-1);			/* init to signal error */
	BK *bk = (sb == NULL ? NULL : sb->block);	/* ptr to parent block */
	int index = (sb == NULL ? (-1) : sb->index);	/* >=0 to write index# */
	int nflush = (sb == NULL ? (-1) :	/* #bytes in subblock to be flushed */
				  (sb->nsubbytes + (sb->nsubbits == 0 ? 0 : 1)));
	int putblkbyte(), putblkbytes();	/* write subblock to block */
	if (nflush < 1)				/* nothing to flush or error */
		status = nflush;		/* return 0 or -1=error to caller */
	else {						/* have data in subblock */
		int indexlen = (index >= 0 ? 1 : 0);	/* and maybe extra byte for index# */
		/* --- trap internal error --- */
		if (nflush + indexlen > SUBBLOCKSIZE)
			nflush = SUBBLOCKSIZE - indexlen;
		/* --- first, emit leading byte containing number of bytes to follow --- */
		if (putblkbyte(bk, nflush + indexlen) == (-1))
			goto end_of_job;	/*failed? */
		/* --- next, subblock index#, if desired --- */
		if (index >= 0) {		/* index# desired */
			if (++index > 255)
				index = 1;		/* bump index#, wrap to 1 after 255 */
			if (putblkbyte(bk, index) == (-1))
				goto end_of_job;	/*quit if failed */
			sb->index = index;
		}
		/* store bumped index# */
		/* --- finally, the subblock bytes themselves  --- */
		if (putblkbytes(bk, sb->subblock, nflush) != (-1)) {	/* okay */
			memset(sb->subblock, 0, SUBBLOCKSIZE);	/* clear the subblock */
			sb->nsubbytes = sb->nsubbits = 0;	/* reset subblock indexes */
			status = nflush;
		}						/* signal success */
	}							/* --- end-of-if(nflush>=1) --- */
  end_of_job:
	return (status);			/* back with #bytes or -1=error */
}								/* --- end-of-function flushsubblock() --- */


/* ==========================================================================
 * Functions:	gifwidth  ( gs )
 *		gifheight ( gs )
 * Purpose:	user utility functions
 *		  returns width, height of gif
 *		  (for caller's function that doesn't know gs structure)
 * --------------------------------------------------------------------------
 * Arguments:	gs (I)		void *address of GS struct,
 *				which we'll interpret for caller
 * --------------------------------------------------------------------------
 * Returns:	( int )		gs->width or gs->height, as appropriate,
 *				or 0 for any error.
 * --------------------------------------------------------------------------
 * Notes:     o
 * ======================================================================= */
/* --- entry points --- */
int gifwidth(void *gs)
{
	return ((gs == NULL ? 0 : ((GS *) gs)->width));
}

int gifheight(void *gs)
{
	return ((gs == NULL ? 0 : ((GS *) gs)->height));
}


/* ==========================================================================
 * Function:	pixgraph ( ncols, nrows, f, n )
 * Purpose:	user utility function
 *		  generates pixels for putgif()
 *		  representing a simple graph of f[0]...f[n-1]
 * --------------------------------------------------------------------------
 * Arguments:	ncols (I)	int containing width of gif image
 *		nrows (I)	int containing height of gif image
 *		f (I)		double *  to values to be graphed
 *		n (I)		int  containing number of f-values
 * --------------------------------------------------------------------------
 * Returns:	( BYTE * )	malloc'ed buffer of nrows*ncols pixels,
 *				or NULL for any error.
 * --------------------------------------------------------------------------
 * Notes:     o
 * ======================================================================= */
/* --- entry point --- */
BYTE *pixgraph(int ncols, int nrows, double *f, int n)
{
/* --- allocations and declarations --- */
	int maxpixels = 1000000, npixels = (ncols < 1 || nrows < 1 ? 0 : min2(ncols * nrows, maxpixels));	/*#bytes */
	BYTE *pixels = (BYTE *) (npixels < 1 ? NULL : malloc(npixels));	/*malloc buffer */
	int row = 0, col = 0;		/* row=y,col=x pixel coords */
	double bigf = 0.0;			/* largest f-value */
/* --- initialize pixels and check input --- */
	if (pixels == NULL)
		goto end_of_job;		/* malloc failed (or input error) */
	memset(pixels, 0, npixels);	/* clear pixel bytes (bg=0 assumed) */
	if (f == NULL || n < 1)
		goto end_of_job;		/* nothing to graph */
/* ---
 * set pixels representing f[]
 * ------------------------------ */
	for (col = 0; col < n; col++)
		if (absval(f[col]) > bigf)
			bigf = absval(f[col]);
	for (row = 0; row < nrows; row++) {
		double yval =
			bigf * ((double) ((nrows / 2) - row)) / ((double) (nrows / 2));
		for (col = 0; col < ncols; col++) {
			int i = (col * (n - 1)) / ncols;
			int ipixel = min2(row * ncols + col, maxpixels);
			if (yval * f[i] >= yval * yval)
				pixels[ipixel] = ((BYTE) 1);
		}
	}							/* --- end-of-for(row) --- */
  end_of_job:
	return (pixels);			/* caller should free(pixels) */
}								/* --- end-of-function pixgraph() --- */


#if defined(GSTESTDRIVE)
/* ==========================================================================
 * Function:	main ( int argc, char *argv[] )
 * Purpose:	test driver for gifsave89
 * --------------------------------------------------------------------------
 * Arguments:	argc (I)	as usual.
 *		argv (I)	Also see "command-line args" in main()
 *				argv[1] = test#, 1 or 2
 *					   for test#1   for test#2
 *					-----------------------------
 *				argv[2] =   #rows        #frames
 *				argv[3] =   #cols     disposal method
 *				argv[4] =  filename      filename
 * --------------------------------------------------------------------------
 * Returns:	( int )		always 0
 *		Also writes file gifsave89test.gif, if you don't specify
 *		an alternative filename. Just typing
 *		  ./gifsave89
 *		outputs a single waveform image, 300x300 pixels, to the file.
 *		Typing
 *		  ./gifsave89 2
 *		outputs an animation sequence comprising 50 frames.
 *		See "command-line args" in main() for further details.
 * --------------------------------------------------------------------------
 * Notes:     o	See http://www.forkosh.com/gifsave89.html
 *		for a more detailed discussion of the following information.
 *	      o	Compile gifsave89 as
 *		  cc -DGSTESTDRIVE gifsave89.c -lm -o gifsave89
 *		for an executable image with this main() test driver, or as
 *		  cc yourprogram.c gifsave89.c -o yourprogram
 *		for use with your own program (-lm no longer necessary
 *		unless yourprogram needs it).
 *		-------------- gifsave89 usage instructions -----------------
 *	      o	main() is a short program illustrating simple gifsave89 usage
 *		(the hard part is usually generating the pixels).
 *	      o	The typical sequence of gifsave function calls is...
 *		  First, call
 *		    gs = newgif(&gifimage,ncols,nrows,colortable,bgindex)
 *		  Next, optionally call
 *		    animategif(gs,nrepetitions,delay,tcolor,disposal)
 *		  Now, once or in a loop {
 *		    optionally call  controlgif(gs,etc)
 *		    always     call  putgif(gs,pixels) }
 *		  Finally, call
 *		    nbytes = endgif(gs)
 *		  Now do what you want with gifimage[nbytes],
 *		    and then free(gifimage)
 *	      o	Note that void *newgif() returns a ptr that must be passed
 *		as the first argument to every subsequent gifsave89 function
 *		called.
 *	      o	Minimal usage example/template...
 *		  void *gs=NULL, *newgif();
 *		  int  nbytes=0, putgif(), endgif();
 *		  int  ncols=255, nrows=255,
 *		       *colortable = { 255,255,255, 0,0,0, -1 },
 *		       bgindex=0;
 *		  unsigned char *pixels = { 0,1,0,1, ..., ncols*nrows };
 *		  unsigned char *gifimage = NULL;
 *		  gs = newgif(&gifimage,ncols,nrows,colortable,bgindex);
 *		  if ( gs != NULL ) {
 *		    putgif(gs,pixels);
 *		    nbytes = endgif(gs); }
 *		  if ( nbytes > 0 ) {
 *		    do_what_you_want_with(gifimage,nbytes);
 *		    free(gifimage); }
 *		---------------- gifsave89 data elements --------------------
 *	      o	unsigned char pixels[nrows*ncols]; which you may malloc(),
 *		is always a sequence of bytes in row order, i.e.,
 *		pixels[0] corresponds to the upper-left corner pixel,
 *		pixels[ncols] to the leftmost pixel of the second row, ...
 *		pixels[nrows*ncols-1] to the lower-right corner pixel.
 *	      o	Each pixels[] byte contains the color table index,
 *		described immediately below, for the corresponding pixel.
 *	      o	int colortable[] = { r0,g0,b0, r1,g1,b1, r2,g2,b2, ..., -1 };
 *		is a sequence of ints, each one 0 to 255, interpreted as
 *		r,g,b-values. Terminate the sequence with any negative number.
 *	      o	The first three colortable[] numbers are the
 *		r,g,b values for any pixel=pixels[ipixel]  whose value is 0,
 *		the second three for any pixel whose value is 1, etc.
 *	      o	The number of colors in colortable[] must be a power of 2,
 *		e.g., you must have 6 (for 2 colors), or 12 (for 4), or etc,
 *		numbers in colortable[] preceding the -1.
 *	      o	Simple usage is int colortable[] = {255,255,255, 0,0,0, -1};
 *		for monochrome pixel=0/white, pixel=1/black.
 *		And, obviously, pixel values must be between 0 and #colors-1.
 *		--------------------- animation note ------------------------
 *	      o	For animated gifs, you'd typically expect to call controlgif()
 *		before each putgif() frame, specifying delay=0,disposal=2.
 *		That's not necessary: gifsave89 automatically emits that
 *		graphic control extension for animated gifs if you haven't.
 * ======================================================================= */
/* --- entry point --- */
int main(int argc, char *argv[])
{
/* -------------------------------------------------------------------------
Allocations and Declarations
-------------------------------------------------------------------------- */
/* ---
 * gifsave89 funcs declared below in typical calling sequence order
 * you'd typically need these vars to use gifsave89 in your own program...
 * ----------------------------------------------------------------------- */
/* --- gifsave89 funcs --- */
	void *gs = NULL, *newgif();	/* allocate, init gifsave89 struct */
	void *gifimage = NULL, *makegif();	/* output generated by gifsave89 */
	int animategif(),			/* "NETSCAPE""2.0" application ext */
	 putgif(),					/* add (another) gif frame */
	 nbytes = 0, endgif();		/* #bytes in generated gifimage */
/* --- typical gifsave89 colortable --- */
	int colortable[] = { 255, 255, 255, 0, 0, 255,	/*bg(index0)=white,fg(1)=blue */
		0, 255, 0, 255, 0, 0,	/* also colors 2=green, 3=red */
		-1
	};							/* end-of-colortable[] */
	int bgindex = 0;			/* background colortable index */
/* --- optional gifsave89 plaintext --- */
	int plaintxtgif();			/* overlay text on image */
/* --- optional verbosity (for debug) --- */
	int debuggif();				/* set msglevel,msgfile */
/* ---
 * additional funcs and variables for our tests...
 * ----------------------------------------------- */
/* --- additional funcs --- */
	int nwritten = 0, writefile();	/* write test output file */
	BYTE *pixels = NULL, *pixgraph();	/* generate pixels for gif image */
	double *f(), fparam = 0.0, pi = 3.14159;	/* y=f(x) waveform for gif image */
/* --- additional variables --- */
	int nvals = 500;			/* #f-vals generated (all tests) */
	int iframe = 0;				/* animation frame# (for test#2) */
	int istext = 0;				/* true to issue plaintxtgif() */
/* ---
 * command-line args
 * -------------------- */
	int testnum = (argc > 1 ? atoi(argv[1]) : 1);	/*testnum is first arg, or 1 */
/* --- test#1 args --- */
	int nrows = (testnum == 1 && argc > 2 ? atoi(argv[2]) : 300),
		ncols = (testnum == 1 && argc > 3 ? atoi(argv[3]) : 300);
/* --- test#2 args --- */
	int nframes = (testnum == 2 && argc > 2 ? atoi(argv[2]) : 50),
		nreps = (testnum == 2 && argc > 3 ? atoi(argv[3]) : 0);
/* --- args for all tests --- */
	char *text = (argc > 4 ? argv[4] : "\000");	/* text to be overlaid */
	char *file = (argc > 5 ? argv[5] : "gifsave89test.gif");

/* -------------------------------------------------------------------------
test illustrating general gifsave89 usage
-------------------------------------------------------------------------- */
/* --- data checks --- */
	if (testnum != 2)
		testnum = 1;			/* input check: force test#1 or #2 */
	if (strlen(text) > 1)
		istext = 1;				/* have overlay text */
/* --- set verbosity (to display gnu/gpl copyright) --- */
	debuggif(1, NULL);			/* msglevel=1, msgfp=stdout */

/* ---
 * testnum 1: single-frame gif of sample waveform y=f(x)
 * note how easy it is to use gifsave89
 * ------------------------------------------------------- */
	if (testnum == 1) {
		/* ---generate pixels to be graphed (this is typically the hard part)--- */
		if ((pixels = pixgraph(ncols, nrows, f(fparam, nvals), nvals))	/*get pixels */
			== NULL)
			goto end_of_job;	/* quit if failed */
		/* ---"short form" creates gifimage of pixels with one function call--- */
		if (!istext)			/* one-line func call for gifimage */
			gifimage =
				makegif(&nbytes, ncols, nrows, pixels, colortable,
						bgindex);
		/* --- or gifsave89 "long form" to include plaintext --- */
		else if ((gs = newgif(&gifimage, ncols, nrows, colortable, bgindex))	/*init gifsave89 */
				 !=NULL) {		/* if initialization succeeded */
			plaintxtgif(gs, -1, -1, 0, 0, bgindex, 1, text);	/* overlay text */
			putgif(gs, pixels);	/* add image */
			nbytes = endgif(gs);
		}						/* done */
	}

	/* --- end-of-if(testnum==1) --- */
	/* ---
	 * testnum 2: same as testnum#1, except we animate gif frames
	 * almost as easy
	 * ----------------------------------------------------------- */
	if (testnum == 2) {
		/* --- initialize gif as before (as for test#1) --- */
		if ((gs = newgif(&gifimage, ncols, nrows, colortable, bgindex))	/*init gifsave89 */
			== NULL)
			goto end_of_job;	/* quit if failed */
		/* --- but this time emit application extension for looped animation --- */
		if (nreps >= 0)			/* want looped animation? */
			animategif(gs, nreps, 0, -1, 2);	/* emit application extension */
		/* --- if requested, add same text to every frame --- */
		if (istext)
			plaintxtgif(gs, -1, -1, -1, -1, bgindex, 1, text);	/*overlay text */
		/* --- now render frames comprising the animation --- */
		for (iframe = 0; iframe < nframes; iframe++) {	/* render each frame */
			if (iframe > 0)
				free((void *) pixels);	/* first free previous frame */
			/* --- pixgraph() now generates different pixels for each frame --- */
			fparam = 2. * pi * ((double) iframe) / ((double) (nframes));	/* 0<=fparam<2pi */
			pixels = pixgraph(ncols, nrows, f(fparam, nvals), nvals);
			/* --- putgif adds another frame to the animated gif --- */
			putgif(gs, pixels);	/* add another frame image */
		}						/* --- end-of-for(iframe) --- */
		/* --- all frames written --- */
		nbytes = endgif(gs);	/* done */
	}


	/* --- end-of-if(testnum==2) --- */
	/* -------------------------------------------------------------------------
	   write gif image file and cleanup
	   -------------------------------------------------------------------------- */
	/* ---save image to file for display with ImageMagic or other viewer--- */
	if (file != NULL) {
		nwritten = writefile(gifimage, nbytes, file);
		fprintf(stdout, "GIF image written to file: %s\n",
				(nwritten > 0 ? file : "**write failed**"));
	}
/* --- cleanup --- */
	if (pixels != NULL)
		free((void *) pixels);	/* free allocated pixels */
	if (gifimage != NULL)
		free((void *) gifimage);	/* free allocated image */
/* ----------------------------------------------------------------------- */
  end_of_job:
/* ----------------------------------------------------------------------- */
	return (0);					/* bye-bye */
}								/* --- end-of-function main() --- */


/* ==========================================================================
 * Function:	f ( param, n )
 * Purpose:	sample gif image function for gifgraph()
 * --------------------------------------------------------------------------
 * Arguments:	param (I)	double containing f-parameter
 *		n (I)		int containing #values wanted
 * --------------------------------------------------------------------------
 * Returns:	( double * )	ptr to n static values of sample function
 * --------------------------------------------------------------------------
 * Notes:     o
 * ======================================================================= */
/* --- entry points --- */
double *f(double param, int n)
{
	static double yvals[999];	/* returned vals */
	int ix = 0, iy = 0;			/* yvals[ix] index, iy=harmonic# */
	double pi = 3.14159;
	n = max2(2, min2(999, n));	/* 2<=n<=999 */
	for (ix = 0; ix < n; ix++) {
		double x = 2. * pi * ((double) ix) / ((double) n);	/* 0<=x<2pi */
		double phase = param;	/* interpret param as phase */
		yvals[ix] = 0.0;		/* init */
		for (iy = 0; iy < 10; iy++) {	/* harmonics */
			double y = ((double) (iy + 1));
			yvals[ix] += sin(y * x + phase) * cos(x + y * phase);
		}
	}							/* --- end-of-for(ix) --- */
/*end_of_job:*/
	return (yvals);
}								/* --- end-of-function f() --- */


/* ==========================================================================
 * Function:	writefile ( buffer, nbytes, file )
 * Purpose:	emits buffer to file or to stdout
 * --------------------------------------------------------------------------
 * Arguments:	buffer (I)	(BYTE *)pointer to buffer of bytes to be
 *				emitted
 *		nbytes (I)	int containing total #bytes from buffer
 *				to be emitted
 *		file (I)	(char *)pointer to null-terminated
 *				char string containing full path to
 *				file where buffer will be written,
 *				or NULL or empty string for stdout
 * --------------------------------------------------------------------------
 * Returns:	( int )		#bytes emitted (!=nbytes signals error)
 * --------------------------------------------------------------------------
 * Notes:     o
 * ======================================================================= */
/* --- entry point --- */
int writefile(BYTE * buffer, int nbytes, char *file)
{
/* -------------------------------------------------------------------------
Allocations and Declarations
-------------------------------------------------------------------------- */
	FILE *fileptr = stdout;		/* init to write buffer to stdout */
	int nwritten = 0;			/* #bytes actually written */
/* -------------------------------------------------------------------------
initialization
-------------------------------------------------------------------------- */
/* --- check input --- */
	if (buffer == NULL			/* quit if no input from caller */
		|| nbytes < 1)
		goto end_of_job;		/* or no output requested */
/* --- open file (or write to stdout) --- */
	if (file != NULL)			/* have file arg */
		if (*file != '\000')	/* file arg not an empty string */
			if ((fileptr = fopen(file, "wb"))	/* open file for binary write */
				== (FILE *) NULL)
				fileptr = stdout;	/* revert to stdout if fopen fails */
/* -------------------------------------------------------------------------
write bytes from buffer and return to caller
-------------------------------------------------------------------------- */
/* --- write bytes from buffer to file (or to stdout) --- */
	nwritten = fwrite(buffer, sizeof(BYTE), nbytes, fileptr);	/* write bytes */
	if (fileptr != NULL && fileptr != stdout)
		fclose(fileptr);		/* close file */
/* --- back to caller --- */
  end_of_job:
	return (nwritten);			/* back with #bytes written */
}								/* --- end-of-function writefile() --- */
#endif
/* ====================== END-OF-FILE GIFSAVE89.C ======================== */
