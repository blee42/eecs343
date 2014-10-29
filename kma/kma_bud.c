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
#include <stdio.h>

/************Private include**********************************************/
#include "kma_page.h"
#include "kma.h"

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */

 // header at the top of each allocated
 // or free block
 typedef struct allocheader_t {
   int size;
   struct allocheader_t* next;
 } allocheaderT;

// each element in the array
typedef struct freelist_l {
  allocheaderT* first_block;
  struct freelist_l* bigger_size; // 32 -> 64, 64 -> 128, etc.
} freelistL;

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

typedef struct pageheader_t {
  kma_page_t* page;
  freeheadersL free_headers;
  // int[32] bitmap; // = {0}
 } pageheaderT;

#define REALSIZE(size) (size+sizeof(allocheaderT))

/************Global Variables*********************************************/

kma_page_t* first_page = NULL;

/************Function Prototypes******************************************/

void init_page(kma_page_t* page);
void* get_free_entry();
void* get_free_buffer(int size);
int get_required_buffer_size(int size);
void split_buffer();

void* get_buddy();
void coalesce();
	
/************External Declaration*****************************************/

/**************Implementation***********************************************/

void* kma_malloc(kma_size_t size)
{
  printf("%d\n", REALSIZE(size)-size);
  if (REALSIZE(size) > PAGESIZE)
    return NULL;

  if (first_page == NULL)
  {
    init_page(first_page);
  }

  // int buffer_size = get_required_buffer_size(REALSIZE(size));

  // void* allocation = get_free_buffer(buffer_size);
  // if (allocation != NULL)
  // {
  //   return NULL;
  // }


  // still need to think about page stuff

  return NULL;



}

void kma_free(void* ptr, kma_size_t size)
{
  // change bitmap
  // change free list
  coalesce();

}

void init_page(kma_page_t* page)
{
  page = get_page();
  pageheaderT* page_headers = (pageheaderT*) (page->ptr);
  page_headers->page = page;

  freeheadersL* freeheaders = &page_headers->free_headers;
  

  freeheaders->buffer32.bigger_size = &freeheaders->buffer64;
  freeheaders->buffer64.bigger_size = &freeheaders->buffer128;
  freeheaders->buffer128.bigger_size = &freeheaders->buffer256;
  freeheaders->buffer256.bigger_size = &freeheaders->buffer512;
  freeheaders->buffer512.bigger_size = &freeheaders->buffer1024;
  freeheaders->buffer1024.bigger_size = &freeheaders->buffer2048;
  freeheaders->buffer2048.bigger_size = &freeheaders->buffer4096;
  freeheaders->buffer4096.bigger_size = &freeheaders->buffer8192;
  freeheaders->buffer8192.bigger_size = NULL;

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
  freeheadersL* headers;
  // search in list
  split_buffer();

  return NULL;
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

  return NULL;

}


void coalesce()
{
  // for each, get_buddy()
  // if i + get_buddy(i) are free, combine
    // recombine bitmaps and update alloc lists

}


#endif // KMA_BUD
