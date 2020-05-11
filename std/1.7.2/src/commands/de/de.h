/****************************************************************/
/*								*/
/*	de.h							*/
/*								*/
/*		Definitions for the "Disk editor".		*/
/*								*/
/****************************************************************/
/*  origination         1989-Jan-15        Terrence W. Holm	*/
/****************************************************************/


/****************************************************************/
/*								*/
/*	de(1)							*/
/*								*/
/*  This is the MINIX disk editor. It allows the user to	*/
/*  observe and modify a file system. It can also be used	*/
/*  to recover unlink(2)'ed files				*/
/*								*/
/*  See the de(1) man page.					*/
/*								*/
/****************************************************************/


/****************************************************************/
/*								*/
/*	de		   Copyright  Terrence W. Holm  1989	*/
/*								*/
/* This program was written for users of the Minix operating	*/
/* system, and in the spirit of other public domain software	*/
/* written for said system, this source code is made available	*/
/* at no cost to everyone. I assume no responsibility for	*/
/* damage to file systems caused by this program.		*/
/*								*/
/* This program (one .h, five .c's and a "man" page) may be	*/
/* copied and/or modified subject to (1) no charge must be	*/
/* made for distribution, other than for the medium, (2) all	*/
/* modified sources must be clearly marked as such, (3) all	*/
/* sources must carry this copyright.				*/
/*								*/
/****************************************************************/


/****************************************************************/
/*								*/
/*	files							*/
/*								*/
/*	    de.h		Definitions			*/
/*	    de.c		The main loop			*/
/*	    de_stdin.c		Character input routines	*/
/*	    de_stdout.c		Output routines			*/
/*	    de_diskio.c		File system read/write		*/
/*	    de_recover.c	File restoration routines	*/
/*								*/
/*	    de.1		"Man" page			*/
/*	    Makefile		For "make"			*/
/*	    README		Installation help		*/
/*								*/
/*								*/
/*	fs/path.c was modified to support the 'x' command.	*/
/*	fs/link.c and fs/open.c were changed for 'X'.		*/
/*								*/
/****************************************************************/
#undef printf
#include <stdio.h>

/*  General constants  */

#define   MAX_STRING	60		/*  For all input lines	*/
#define   MAX_PREV	8		/*  For 'p' command	*/
#define   SEARCH_BUFFER (4*K)		/*  For '/' and 'n'	*/


/*  Files  */

#define   TMP      "/tmp"		/*  For "-r" output	*/
#define   DEV	   "/dev"		/*  Where devices are	*/


/*  a.out header constants  (see a.out.h, if you have it)  */

#if (CHIP == INTEL)
#define   A_OUT    0x0301
#define   SPLIT    0x0420
#endif

#if (CHIP == M68000)
#define   A_OUT    0x0301
#define   SPLIT	   0x0B20
#endif

#if (CHIP == SPARC)
#define   A_OUT    0x0301
#define   SPLIT    0x0B20
#endif

/*  Each buffer is 1k.  In WORD mode 16 words (32 bytes) can be	*/
/*  displayed at once. In BLOCK mode 1K bytes can be displayed.	*/
/*  In MAP mode 2048 bits (256 bytes) are displayed.		*/

#define   K		1024		/*  STD_BLK		*/
#define   K_MASK	(~(K-1))	/*  Round to K boundary	*/
#define   K_SHIFT	10		/*  Ie. 1<<10 = K	*/
#define   PAGE_MASK	0x1f		/*  Word mode: 32 bytes	*/
#define   PAGE_SHIFT    5		/*  Ie. 1<<5 = 32	*/
#define   MAP_BITS_PER_BLOCK (8 * K)    /*  1k block, 8192 bits */
#define   MAP_MASK	0xff		/*  256 bytes/screen	*/



/*  Terminal i/o codes  */

#define   CTRL_D	'\004'		/*  ASCII ^D		*/
#define   BELL		'\007'		/*  ASCII bell code     */
#define   BS		'\010'		/*  ASCII back space	*/
#define   CTRL_U	'\025'		/*  ASCII ^U		*/
#define	  ESCAPE  	'\033'		/*  ASCII escape code	*/
#define   DEL           '\177'		/*  ASCII delete code   */


/*  Input escape codes generated by the	Minix console.	*/
/*  Format: ESC [ X. 					*/

#define   ESC_HOME	('H' + 0x80)
#define   ESC_UP	('A' + 0x80)
#define   ESC_PGUP	('V' + 0x80)
#define   ESC_LEFT	('D' + 0x80)
#define   ESC_5		('G' + 0x80)
#define   ESC_RIGHT	('C' + 0x80)
#define   ESC_END	('Y' + 0x80)
#define   ESC_DOWN	('B' + 0x80)
#define   ESC_PGDN	('U' + 0x80)
#define   ESC_PLUS	('T' + 0x80)
#define   ESC_MINUS	('S' + 0x80)


/*  Graphic box codes - only applicable for console display  */
/*  in visual mode "map".				     */

#if (CHIP == INTEL)
#define   BOX_CLR	' '		/*  Empty box		*/
#define   BOX_ALL	'\333'		/*  Filled box		*/
#define   BOX_TOP	'\337'		/*  Filled upper half	*/
#define   BOX_BOT	'\334'		/*  Filled lower half   */
#endif

#if (CHIP == M68000)
/*  Please change these.  */
#define   BOX_CLR	' '		/*  Empty box		*/
#define   BOX_ALL	'='		/*  Filled box		*/
#define   BOX_TOP	'-'		/*  Filled upper half	*/
#define   BOX_BOT	'_'		/*  Filled lower half   */
#endif

#if (CHIP == SPARC)
/*  Please change these.  */
#define   BOX_CLR	' '		/*  Empty box		*/
#define   BOX_ALL	'='		/*  Filled box		*/
#define   BOX_TOP	'-'		/*  Filled upper half	*/
#define   BOX_BOT	'_'		/*  Filled lower half   */
#endif

/*  Move positions for the output display.  */

#define   STATUS_COLUMN	 2
#define   STATUS_LINE    0
#define   BLOCK_COLUMN	 4
#define   BLOCK_LINE	 4
#define   INFO_COLUMN	 30
#define   INFO_LINE	 BLOCK_LINE
#define   PROMPT_COLUMN	 0
#define   PROMPT_LINE	 23
#define   WARNING_COLUMN 5
#define   WARNING_LINE   10



/*  Values returned by Process() and Get_Filename()  */

#define   OK		  0		/*  No update required	*/
#define   REDRAW	  1		/*  Redraw whole screen	*/
#define   REDRAW_POINTERS 2		/*  Redraw just ptrs	*/
#define   ERROR		  3		/*  Beep		*/


/*  Visual modes  */

#define   WORD	   1
#define   BLOCK    2
#define   MAP	   3

typedef  unsigned short word_t;		/*  For most user i/o	*/
#if _WORD_SIZE == 2
typedef  unsigned int Word_t;		/*  What it should always be */
#else
typedef  int Word_t;			/*  Unsigned promotion under ANSI C */
#endif

typedef  struct  de_state		/*  State of disk ed.	*/
  {
  /*  Information from super block  */
  /*  The types here are mostly promoted types for simplicity	*/
  /*  and efficiency.						*/

  unsigned inodes;			/*  Number of i-nodes	*/
  zone_t zones;				/*  Total # of blocks	*/
  unsigned inode_maps;			/*  I-node map blocks	*/
  unsigned zone_maps;			/*  Zone map blocks	*/
  unsigned inode_blocks;		/*  I-node blocks	*/
  unsigned first_data;			/*  Total non-data blks	*/
  int magic;				/*  Magic number	*/

  /* Numbers derived from the magic number */  
  
  unsigned char is_fs;			/*  Nonzero for good fs	*/
  unsigned char v1;			/*  Nonzero for V1 fs	*/
  unsigned inode_size;			/*  Size of disk inode	*/
  unsigned nr_indirects;		/*  # indirect blocks	*/
  unsigned zone_num_size;		/*  Size of disk z num	*/

  /* Other derived numbers */  
  
  bit_t inodes_in_map;			/*  Bits in i-node map	*/
  bit_t zones_in_map;			/*  Bits in zone map	*/
  int ndzones;				/*  # direct zones in an inode */

  /*  Information from map blocks  */

  bitchunk_t inode_map[ I_MAP_SLOTS * K / sizeof (bitchunk_t) ];
  bitchunk_t zone_map[ Z_MAP_SLOTS * K / sizeof (bitchunk_t) ];

  /*  Information for current block  */

  off_t address;			/*  Current address	*/
  off_t last_addr;			/*  For erasing ptrs	*/
  zone_t block;				/*  Current block (1K)	*/
  unsigned offset;			/*  Offset within block	*/

  char buffer[ K ];

  /*  Display state  */

  int  mode;				/*  WORD, BLOCK or MAP	*/
  int  output_base;			/*  2, 8, 10, or 16	*/

  /*  Search information  */

  char search_string[ MAX_STRING + 1 ];	/*  For '/' and 'n'	*/
  off_t prev_addr[ MAX_PREV ];		/*  For 'p' command	*/
  int   prev_mode[ MAX_PREV ];

  /*  File information  */

  char *device_name;			/*  From command line	*/
  int   device_d;
  int   device_mode;			/*  O_RDONLY or O_RDWR	*/
  zone_t device_size;			/*  Number of blocks	*/

  char  file_name[ MAX_STRING + 1 ];	/*  For 'w' and 'W'	*/
  FILE *file_f;
  int   file_written;			/*  Flag if written to	*/

  }  de_state;



/*  Forward references for external routines  */

/*  de.c  */

_PROTOTYPE(void main , (int argc , char *argv []));
_PROTOTYPE(int Process , (de_state *s , int c ));

#if __STDC__
void  Error( const char *text, ... );
#else
void  Error();
#endif

_PROTOTYPE(int In_Use , (bit_t bit , bitchunk_t *map ));
_PROTOTYPE(ino_t Find_Inode , (de_state *s , char *filename ));


/*  de_stdin.c  */

_PROTOTYPE(void Save_Term , (void));
_PROTOTYPE(void Set_Term , (void));
_PROTOTYPE(void Reset_Term , (void));
_PROTOTYPE(int Get_Char , (void));
_PROTOTYPE(char *Get_Line , (void));
_PROTOTYPE(int Arrow_Esc , (int c ));

/*  de_stdout.c  */

_PROTOTYPE(int Init_Termcap , (void));
_PROTOTYPE(void Draw_Help_Screen , (de_state *s ));
_PROTOTYPE(void Wait_For_Key , (void));
_PROTOTYPE(void Draw_Prompt , (char *string ));
_PROTOTYPE(void Erase_Prompt , (void));

_PROTOTYPE(void Draw_Screen , (de_state *s ));
_PROTOTYPE(void Draw_Strings , (de_state *s ));
_PROTOTYPE(void Draw_Pointers , (de_state *s ));
_PROTOTYPE(void Print_Ascii , (int c ));

_PROTOTYPE(void Goto , (int column , int line ));
_PROTOTYPE(void Block_Type , (de_state *s ));
_PROTOTYPE(void Draw_Words , (de_state *s ));
_PROTOTYPE(void Draw_Info , (de_state *s ));
_PROTOTYPE(void Draw_Block , (char *block ));
_PROTOTYPE(void Draw_Map , (char *block , int max_bits ));
_PROTOTYPE(void Draw_Offset , (de_state *s ));
_PROTOTYPE(void Word_Pointers , (off_t old_addr , off_t new_addr ));
_PROTOTYPE(void Block_Pointers , (off_t old_addr , off_t new_addr ));
_PROTOTYPE(void Map_Pointers , (off_t old_addr , off_t new_addr ));
_PROTOTYPE(void Print_Number , (Word_t number , int output_base ));
_PROTOTYPE(void Draw_Zone_Numbers , (de_state *s , struct inode *inode ,
						int zindex , int zrow ));

#if __STDC__
void  Warning( const char *text, ... );
#else
void  Warning();
#endif


/*  de_diskio.c  */

_PROTOTYPE(void Read_Disk , (de_state *s , off_t block_addr , char *buffer ));
_PROTOTYPE(void Read_Block , (de_state *s , char *buffer ));
_PROTOTYPE(void Read_Super_Block , (de_state *s ));
_PROTOTYPE(void Read_Bit_Maps , (de_state *s ));
_PROTOTYPE(off_t Search , (de_state *s , char *string ));
_PROTOTYPE(void Write_Word , (de_state *s , Word_t word ));


/*  de_recover.c  */

_PROTOTYPE(int Path_Dir_File , (char *path_name , char **dir_name ,
							char **file_name ));
_PROTOTYPE(char *File_Device , (char *file_name ));
_PROTOTYPE(ino_t Find_Deleted_Entry , (de_state *s , char *path_name ));
_PROTOTYPE(off_t Recover_Blocks , (de_state *s ));


#undef    printf			/*  Because fs/const.h	*/
					/*  defines it.		*/


/*  Static functions are all pre-declared FORWARD but none are	*/
/*  declared static yet - this can wait until all functions are	*/
/*  declared with prototypes.					*/

#undef FORWARD
#define FORWARD /* static */