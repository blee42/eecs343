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
  int id;
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
kma_page_t* new_page(kma_page_t* page);
void* first_fit(kma_size_t size); // first fit algorithm to find space of specified size
void add_ll(freelist_t* head, void* base, int size);
freelist_t* remove_ll(freelist_t* head, int id);
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
    kma_page_t* page = new_page(getpage());
    entryptr = page;

    add_ll(entryptr->header, entryptr->ptr, page->size);
  }

  main = (pageheader_t*)entryptr->ptr;
  location = first_fit(size);
  main.allocated = 1;
  
  return location;
}

void
kma_free(void* ptr, kma_size_t size)
{
  ;
}

kma_page_t* new_page(kma_page_t page)
{
  pageheader_t* header;
  *((kma_page_t**)page->ptr) = page; 
  header = (pageheader_t*)(page->ptr); // add header to page
  header->header = (freelist_t*)((int)header + sizeof(pageheader_t)); // allocate free list
  return page;
}

void* first_fit (kma_size_t size)
{
  pageheader_t* main = (pageheader_t*)entryptr->ptr;
  freelist_t* cur = (freelist_t*)main->header;

  while (cur != NULL)
  {
    // if current size available is greater than or equal to the size needed
    if (cur->size >= size)
    {
      // if current size available is exactly what is needed
      if (cur->size == size)
      {
        main->header = remove_ll(main->header, cur->id);
        return cur;
      }

      cur->base = cur->base + size;
      cur->size = cur->size - size;
      return cur;
    }
  }

  // if you get here, then there was no space in this page
  kma_page_t* page = new_page(getpage());

  main.pages++;
  return first_fit(size);
}

void add_ll(freelist_t* head, void* base, int size)
{
  freelist_t* cur = head;
  freelist_t* prev = NULL;

  freelist_t* new;
  new->base = base;
  new->size = size;
  new->next = NULL;

  if (base < cur->base)
  {
    new->next = cur;
    return;
  }
  
  while (cur != NULL)
  {
    if (base < cur->base)
    {
      prev->next = new;
      new->next = cur;
      return;
    }
    prev = cur;
    cur = cur->next;
  }
  cur->next = new;
}

freelist_t* remove_ll(freelist_t* head, int id)
{
  freelist_t* cur = head;
  freelist_t* prev = NULL;

  if (cur->id == id)
  {
    head = cur->next;
    return head;
  }

  while (cur != NULL)
  {
    if (cur->id == id)
    {
      prev->next = cur->next;
      return head;
    }
    prev = cur;
    cur = cur->nex;
  }
  cur->next = NULL;
  return head;
}

#endif // KMA_RM
