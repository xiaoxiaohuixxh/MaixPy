
//buf for screen
#define ROWS 24
#define COLUMNS 80
//buf for read buffer, notice buffer, etc.. should >= 64
#define _BUFSIZ  2048	
//buf for hold the whole file*2, orginal is 10KB
#define MIN_FILE (0)
//buf for status line.. should >= 64
#define STATUS_LEN (200)


//platform header
#include "sleep.h"

#include <string.h>

#include "py/objtuple.h"
#include "py/objstr.h"
#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "extmod/misc.h"
#include "uarths.h"
#include "uart.h"
#if MICROPY_VFS
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"
#endif
#include "genhdr/mpversion.h"
#if !MICROPY_VFS
#include "spiffs-port.h"
#include "py/lexer.h"
#endif

uint32_t file_sz = 0;
char* g_fn = NULL;
extern ring_buff_t *ring_recv_hs;
//functions
/****************** stdin_select *******************/
#if 1 
//static int stdin_available(void)
// {
// 	return cbuf_len(u1_cbuf);
// }

//wait for stdin input for t ms
static int stdin_select(int t)
{
	int t0, flag;
	flag = 0;
	t0 = 0;
	while(t0 < t)
	{
		if(ring_recv_hs->flag == 1) 
		{	
			flag = 1;
			break;
		}
        msleep(1);
        t0++;
	}
    
    return flag;
}

#endif

/******************file & console operation*******************/
#if 1
static spiffs_file g_fd;
static int _open(char* fn, int flag)
{
    //const char* mode;
	  int res;
    /*switch(flag)
    {
    case O_RDWR: mode = "a+";break;
    case O_RDONLY: mode = "r";break;
    case O_WRONLY: mode = "w";break;
    case O_WRONLY|O_CREAT|	: mode = "w+";break;
    default: mode = "r";break;
    }*/
    //return (int)fopen(fn, (char*)mode);
   
    spiffs_DIR dir;
	uint8_t found = 0;
	g_fn = malloc(strlen(fn));
	strcpy(g_fn, fn);
    if (!SPIFFS_opendir (&fs, "/", &dir))
        mp_raise_OSError("[MAIXPY]VI:Open dir err");
    struct spiffs_dirent de;
    while (SPIFFS_readdir (&dir, &de))
    {
        if(strcmp(fn, de.name)==0)
        {
            found = 1;
            g_fd =SPIFFS_open(&fs,fn, SPIFFS_RDWR, 0);//SPIFFS_CREAT | SPIFFS_O_TRUNC|
            break;
        }
    }
    SPIFFS_closedir (&dir);

    if(found == 0)
        g_fd =SPIFFS_open(&fs,fn, SPIFFS_CREAT | SPIFFS_O_TRUNC| SPIFFS_RDWR, 0);
    
    if(g_fd == -1){
        mp_raise_OSError(MP_EIO);
        return 0;
    }else {
        g_fd = g_fd + 10;
        return (int)g_fd;
      }
}

int c_read(Byte* p, int size)	//console read
{
    uint64_t rev_size;
    rev_size = 0;
    while(rev_size <= 0)	//block to read at least one char
    {
        rev_size = read_ringbuff(p, size);
    }
    return rev_size;
    //return read(0, p, size);
}
int _read(int fd, Byte* p, int size)
{   int tmp=size;
	char tmpch[10];
    if(fd > 10) {
		SPIFFS_read(&fs,(spiffs_file)(fd-10), p, size);
    }
    else tmp = c_read(p, size);
    return tmp;
    //return read(fd, p, size);
}

int c_write(Byte* p, int size)	//console write
{//for(; size > 0; size--,p++) putchar(*p);return size;
        uarths_send_data((char*)p, size);
		return size;
    //return write(1, p ,size);
}
int _write(int fd, Byte* p, int size)
{   int tmp=size;
    if(fd > 10){

		SPIFFS_close(&fs, fd);
		SPIFFS_remove(&fs, g_fn);
		g_fd =SPIFFS_open(&fs,g_fn, SPIFFS_CREAT | SPIFFS_O_TRUNC| SPIFFS_RDWR, 0);
		if((spiffs_file)(fd-10) == g_fd)
		{
			SPIFFS_lseek(&fs, (spiffs_file)(fd-10), 0, SPIFFS_SEEK_SET);
	        SPIFFS_write(&fs,(spiffs_file)(fd-10), p, size);
			SPIFFS_fflush(&fs, (spiffs_file)(fd-10));
		}
		else
		{
			//...
		}
    }
    else tmp = c_write(p, size);
    return tmp;
    //return write(fd, p, size);
}

void _close(int fd)
{
    SPIFFS_close (&fs, (spiffs_file)(fd-10));
    //close(fd);
}

static int file_size(Byte * fn) // what is the byte size of "fn"
{
	int cnt;

	if (fn == 0 || strlen((const char*)fn) <= 0)
		return (-1);
	cnt = -1;

    spiffs_DIR dir;
    if (!SPIFFS_opendir (&fs, (const char*)fn, &dir))
        mp_raise_OSError(MP_EIO);
    struct spiffs_dirent de;
    while (SPIFFS_readdir (&dir, &de))
    {
        if(strcmp(de.name,fn)==0)
        {
            return de.size;
        }
    }
    SPIFFS_closedir (&dir);
	return (cnt);
}
#endif


/****************** Set terminal attributes *******************/
#if 1
static void rawmode(void)   //set to rawmode
{
	erase_char = 8;	//back space
}

static void cookmode(void)  //return to orig mode
{
	
}

static void vi_init(void)
{
	//uarths_init();
}
#endif