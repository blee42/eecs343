/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the resource map algorithm
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
#ifdef KMA_RM
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

// free list holds <base,size> pairs
typedef struct 
{
  void* base;
  int size;
  void* next;
} freelist_t;

// header for each page
typedef struct
{
  void* self;
  int pages; // number of pages
  int allocated; // empty page or allocated page
  freelist_t* header;
} pageheader_t;

/************Global Variables*********************************************/
kma_page_t* entryptr = 0; // entry to page structure

/************Function Prototypes******************************************/
void* firstfit(kma_size_t size); // first fit algorithm to find space of specified size

/************External Declaration*****************************************/

/**************Implementation***********************************************/

void*
kma_malloc(kma_size_t size)
{
  // requested size too large
  if ((size + sizeof(kma_page_t*)) > page->size)
  {
    free_page(page);
    return NULL;
  }

  pageheader_t* main;
  void* location;

  // set entry pointer
  if (entryptr == 0)
  {
    kma_page_t* page = getpage();
    pageheader_t* header;

    *((kma_page_t**)page->ptr) = page; 
    header = (pageheader_t*)(page->ptr); // add header to page
    header->header = (freelist_t*)((int)header + sizeof(pageheader_t)); // allocate free list

    entryptr = page;

    // add entire page to free list
  }

  main = (pageheader_t*)entryptr->ptr;
  location = firstfit(size);
  main.allocated = 1;
  
  return location;
}

void
kma_free(void* ptr, kma_size_t size)
{
  ;
}

#endif // KMA_RM
