/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the buddy algorithm
 *    Author: Stefan Birrer
 *    Copyright: 2004 Northwestern University
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    Revision 1.2  2009/10/31 21:28:52  jot836
 *    This is the current version of KMA project 3.
 *    It includes:
 *    - the most up-to-date handout (F'09)
 *    - updated skeleton including
 *        file-driven test harness,
 *        trace generator script,
 *        support for evaluating efficiency of algorithm (wasted memory),
 *        gnuplot support for plotting allocation and waste,
 *        set of traces for all students to use (including a makefile and README of the settings),
 *    - different version of the testsuite for use on the submission site, including:
 *        scoreboard Python scripts, which posts the top 5 scores on the course webpage
 *
 *    Revision 1.1  2005/10/24 16:07:09  sbirrer
 *    - skeleton
 *
 *    Revision 1.2  2004/11/05 15:45:56  sbirrer
 *    - added size as a parameter to kma_free
 *
 *    Revision 1.1  2004/11/03 23:04:03  sbirrer
 *    - initial version for the kernel memory allocator project
 *
 ***************************************************************************/
#ifdef KMA_BUD
#define __KMA_IMPL__

/************System include***********************************************/
#include <assert.h>
#include <stdlib.h>

/************Private include**********************************************/
#include "kma_page.h"
#include "kma.h"

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */

#define PAGESIZE 8192

 typedef struct header_t {
  int[32] bitmap; // = {0}
  freeheadersL free_headers;
  kma_page_t* next_page;
 } headerT;


typedef struct freeheaders_l {
  freelistL buffer32;
  freelistL buffer64;
  freelistL buffer128;
  freelistL buffer256;
  freelistL buffer512;
  freelistL buffer1024;
  freelistL buffer2048;
  freelistL buffer4096;
  freelistL buffer8192;
} freeheadersL;

typedef struct freelist_l {
  int id;
  int size;
  int numfree;

  void* base;
  void* next;
} freelistL;

/************Global Variables*********************************************/

kma_page_t* start = NULL;

/************Function Prototypes******************************************/

void init_page();
void* get_free_entry();
int get_required_buffer_size(int size);
void split_buffer();

void* get_buddy();
void coalesce();
	
/************External Declaration*****************************************/

/**************Implementation***********************************************/

void* kma_malloc(kma_size_t size)
{
  int real_size = size + sizeof(bitmapL);
  if (real_size > PAGESIZE)
    return NULL;

  if (start == NULL)
  {
    init_page();
  }

  int buffer_size = get_required_buffer_size(real_size);

  void* allocation = get_free_buffer(buffer_size);
  if (allocation != NULL)
  {
    return 
  }


  // still need to think about page stuff



}

void kma_free(void* ptr, kma_size_t size)
{
  // change bitmap
  // change free list
  // coalesce()

}

void init_page()
{
  start = get_page();
  freeheadersL* freeheaders = (freeheadersL*) (start->ptr + sizeof(bitmapL));

  freeheaders.buffer32.size = 32;
  freeheaders.buffer64.size = 64;
  freeheaders.buffer128.size = 128;
  freeheaders.buffer256.size = 256;
  freeheaders.buffer512.size = 512;
  freeheaders.buffer1024.size = 1024;
  freeheaders.buffer2048.size = 2048;
  freeheaders.buffer4096.size = 4096;
  freeheaders.buffer8192.size = 8192;

}

int get_required_buffer_size(int size)
{
  // think of a way to optimize
  if (size <= 32)
    return 32;
  else if (size <= 64)
    return 64;
  else if (size <= 128)
    return 128;
  else if (size <= 256)
    return 256;
  else if (size <= 512)
    return 512;
  else if (size <= 1024)
    return 1024;
  else if (size <= 2048)
    return 2048;
  else if (size <= 4096)
    return 4096;
  else
    return 8192;
}

void* get_free_buffer(int size)
{
  freeheadersL* headers = (freeheadersL*) (start->ptr + sizeof(bitmapL));
  // search in list
  split_buffer()

}

void split_buffer()
{
  // find out size of split
  // iter through n/2-1 of bitmap, set next to NULL
  // assign 0 through n/2-1 to LEFT

  // assign n/2-n to RIGHT

  // change alloc lists

}

void* get_buddy()
{

}


void coalesce()
{
  // for each, get_buddy()
  // if i + get_buddy(i) are free, combine
    // recombine bitmaps and update alloc lists

}


#endif // KMA_BUD
