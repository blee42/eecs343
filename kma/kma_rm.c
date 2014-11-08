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

/* Structure indicating the beginning of a free block. On request, the 
   address of the front of this struct is given to be allocated, so this
   will be overwritten. */
 typedef struct blockheader_t {
  short size;
  void* next_block;
 } blockheaderT;

#define REAL_PAGE_SIZE (8192 - sizeof(blockheaderT))
#define FIRST_PAGE_BLOCK (*((blockheaderT**) first_page->ptr))


/************Global Variables*********************************************/

kma_page_t* first_page = NULL;

/************Function Prototypes******************************************/

void* find_first_fit(short size);
void* new_page(short size);
void update_malloc_headers(short size, blockheaderT* current_block, blockheaderT* previous_block);
void coalesce();
void clear_empty_pages();

/************External Declaration*****************************************/

/**************Implementation***********************************************/

/* Main malloc function. On call, searches through all free block headers to
   find the first one that fits. */
void* kma_malloc(kma_size_t size)
{
  if (first_page == NULL)
  {
    first_page = get_page();
    blockheaderT* first_block_header = (blockheaderT*) ((long int) first_page->ptr + sizeof(blockheaderT*));
    first_block_header->size = REAL_PAGE_SIZE;
    first_block_header->next_block = NULL;
    FIRST_PAGE_BLOCK = first_block_header;
  }

  if (size > REAL_PAGE_SIZE)
  {
    return NULL;  
  }
  
  return find_first_fit(size); // may point to not first page!
}

/* Main freeing function. On call, inserts a new free block header 
   of the given size into the appropriate space in memory. Since the
   list of free blocks is ordered by address, free must search
   to find the right gap. Afterward, coalesces any free blocks 
   together and clears any empty pages. */
void kma_free(void* ptr, kma_size_t size)
{ 
  blockheaderT* current_free_block = FIRST_PAGE_BLOCK;
  blockheaderT* previous_block = NULL;

  while (current_free_block != NULL)
  {
    if ((long int) current_free_block > (long int) ptr)
    {
      blockheaderT* new_block = (blockheaderT*) ptr;
      new_block->size = (short) size; 
      new_block->next_block = (void*) current_free_block;

      if (previous_block == NULL)
        *((blockheaderT**) (first_page->ptr)) = new_block;
      else
        previous_block->next_block = (void*) new_block;
      break;
    }
    else
    {
      previous_block = current_free_block;
      current_free_block = (blockheaderT*) current_free_block->next_block;
    }
  }

  coalesce();
  clear_empty_pages();
}

/* Iterates through and returns the first fit, except in the case that this fit
   leaves a gap smaller than the free header. Reduces the appropriate header by
   the given amount of space. Allocates a new page if no openings are found. */
void* find_first_fit(short size)
{
  blockheaderT* current_free_block = FIRST_PAGE_BLOCK;
  blockheaderT* previous_block = NULL;

  while (current_free_block != NULL)
  {
    // skip this block if it results in a really small extra space
    if (current_free_block->size - size < sizeof(blockheaderT))
    {
      previous_block = current_free_block;
      current_free_block = (blockheaderT*) current_free_block->next_block;
    }
    // good, normal fit
    else if ((size < current_free_block->size))
    { 
      update_malloc_headers(size, current_free_block, previous_block);
      return (void*) current_free_block;
    }
    else
    {
      previous_block = current_free_block;
      current_free_block = (blockheaderT*) current_free_block->next_block;
    }
  }

  // no openings, new page
  return new_page(size);
}

/* Creates a new page and inserts that page into the appropriate location. Since get_page
   may return a previously freed page, this iteration is necessary to find the right gap
   to inset the free block headers into. This newly created block header is reduce by the
   amount of space requested when creating this new page. */
void* new_page(short size)
{
  kma_page_t* page = get_page();
  *((kma_page_t**)page->ptr) = page;

  blockheaderT* first_block_header = (blockheaderT*) ((long int) page->ptr + sizeof(void*) + size);
  first_block_header->size = REAL_PAGE_SIZE-size;
  first_block_header->next_block = NULL;

  // find and insert block header in right spot
  blockheaderT* current_free_block = FIRST_PAGE_BLOCK;
  blockheaderT* previous_block = NULL;

  while (current_free_block != NULL)
  {
    if ((long int) current_free_block > (long int) first_block_header)
    {
      previous_block->next_block = (void*) first_block_header;
      first_block_header->next_block = (void*) current_free_block;
      break;
    }
    else if (current_free_block->next_block == NULL)
    {
      current_free_block->next_block = (void*) first_block_header;
      break;
    }
    else
    {
      previous_block = current_free_block;
      current_free_block = (blockheaderT*) current_free_block->next_block;
    }
  }

  return (void*)((long int) page->ptr + sizeof(void*));
}

/* Reduces a block header's size by giving up the front of the block and creating a new
   block header after the allocated space. */
void update_malloc_headers(short size, blockheaderT* current_block, blockheaderT* previous_block)
{
  blockheaderT* new_next = (blockheaderT*) current_block->next_block;
  short new_size = current_block->size - size;
  
  blockheaderT* new_block;
  new_block = (blockheaderT*) ((long int) current_block + size);

  new_block->size = new_size;
  new_block->next_block = (void*) new_next;

  if (previous_block == NULL)
    *((blockheaderT**) (first_page->ptr)) = new_block;
  else
    previous_block->next_block = (void*) new_block;
}

/* Merges adjacent blocks on the same page if they are both free. Iterates through the entire list of
   block headers looking for these. */
void coalesce()
{
  blockheaderT* current_block = FIRST_PAGE_BLOCK;
  while (current_block != NULL)
  {
    if (current_block->next_block != NULL &&
      BASEADDR(current_block) == BASEADDR(current_block->next_block) &&
      (((long int) current_block->next_block) - ((long int) current_block + (long int) current_block->size)) == 0)
    {
      current_block->size += ((blockheaderT*) (current_block->next_block))->size;
      current_block->next_block = (void*) ((blockheaderT*) (current_block->next_block))->next_block;
    }
    else
    {
      current_block = (blockheaderT*) current_block->next_block;
    }    
  }
}

/* Iterates through the set of all pages checking for any empty pages. Relinks the block headers as needed
   to maintain the flow of the pages. */
void clear_empty_pages()
{
  blockheaderT* current_block = FIRST_PAGE_BLOCK;
  blockheaderT* previous = NULL;
  while (current_block != NULL)
  {
    if (current_block->size == REAL_PAGE_SIZE)
    {
      void* current_page = BASEADDR(current_block);

      // Not the first page, and in the middle of the list
      if (current_page != (first_page->ptr) && previous != NULL)
      {
        previous->next_block = current_block->next_block;
        free_page(*((kma_page_t**) current_page));
      }
      // Not the first page, but the first free block
      else if (current_page != (first_page->ptr) && previous == NULL)
      {
        FIRST_PAGE_BLOCK = (blockheaderT*) current_block->next_block;
        free_page(*((kma_page_t**) current_page));
      }
      // Is the first page
      else
      {
        // More unfreed pages still exist
        if (current_block->next_block != NULL)
        {
          kma_page_t* temp = first_page;
          first_page = *((kma_page_t**) BASEADDR(current_block->next_block));
          FIRST_PAGE_BLOCK = (blockheaderT*) current_block->next_block;
          free_page(temp);
        }
        // First page is the only one left
        else
        {
          free_page(first_page);
          first_page = NULL;
          return;
        }
      }
    }
    previous = current_block;
    current_block = (blockheaderT*) current_block->next_block;
  }
}

#endif // KMA_RM
