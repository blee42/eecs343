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

 typedef struct blockheader_t {

  int size;
  void* base;
  struct blockheader_t* next_block;

 } blockheaderT;

typedef struct pageheader_t {

  void* page;
  int num_free_blocks;
  blockheaderT* first_free;
  struct pageheader_t* next_page;

} pageheaderT;

#define REAL_PAGE_SIZE (8192 - sizeof(pageheaderT))

/************Global Variables*********************************************/
kma_page_t* first_page = NULL; // entry to page structure

/************Function Prototypes******************************************/

pageheaderT* init_page();
void* find_first_fit(int size, pageheaderT* current_page);
void update_malloc_headers(int size, blockheaderT* block, pageheaderT* page);
void remove_malloc_header(blockheaderT* block, pageheaderT* page);
void coalesce();

/************External Declaration*****************************************/

/**************Implementation***********************************************/

void* kma_malloc(kma_size_t size)
{
  kma_page_t* current_page = first_page;
  pageheaderT* current_page_header;

  if (current_page == NULL)
    current_page_header = init_page();

  printf("%lu\n", REAL_PAGE_SIZE);

  if (size > REAL_PAGE_SIZE)
  {
    free_page(current_page);
    return NULL;  
  }

  return find_first_fit(size, current_page_header); // may point to not first page!
}

void kma_free(void* ptr, kma_size_t size)
{
  pageheaderT* current_page_header = (pageheaderT*) (first_page->ptr);

  while (current_page_header->page + PAGESIZE < ptr)
  {
    current_page_header = current_page_header->next_page;
  }

  blockheaderT* new_block_header = (blockheaderT*) ptr; // need &?
  new_block_header->size = size;
  new_block_header->base = ptr;

  blockheaderT* current = current_page_header->first_free;
  if (current == NULL)
  {
    current_page_header->first_free = new_block_header;
    new_block_header->next_block = NULL;
  }

  blockheaderT* prev;
  while (current->base < ptr)
  {
    prev = current;
    current = current->next_block;
  }

  prev->next_block = new_block_header;
  new_block_header->next_block = current->next_block;

  coalesce();
}

pageheaderT* init_page()
{
  kma_page_t* current_page = get_page();

  pageheaderT* page_header = (pageheaderT*) (current_page->ptr);
  blockheaderT* first_block_header = (blockheaderT*) (current_page->ptr + sizeof(pageheaderT));

  page_header->page = current_page->ptr;
  page_header->num_free_blocks = 1;
  page_header->first_free = first_block_header;

  first_block_header->size = REAL_PAGE_SIZE;
  first_block_header->base = (void*) (current_page->ptr + sizeof(pageheaderT));

  return page_header;
}

void* find_first_fit(int size, pageheaderT* current_page)
{
  blockheaderT* current_free_block = current_page->first_free;
  pageheaderT* really_current_page = current_page;

  while (really_current_page != NULL)
  {
    while (current_free_block != NULL)
    {
      if (current_free_block->size - size <= sizeof(blockheaderT))
      {
        remove_malloc_header(current_free_block, really_current_page);
        return current_free_block->base;
      }
      else if (size < current_free_block->size)
      {
        update_malloc_headers(size, current_free_block, really_current_page);
        return current_free_block->base;
      }
      else
        current_free_block = current_free_block->next_block;
    }

    really_current_page = really_current_page->next_page;
  }
  
  // could not find a block on any page
  really_current_page = init_page();
  return really_current_page->first_free->base;
}

void update_malloc_headers(int size, blockheaderT* block, pageheaderT* page)
{
  blockheaderT* new_block_header = (blockheaderT*) (block->base + size); // need &?
  new_block_header->size = block->size - size;
  new_block_header->base = block->base + size;

  blockheaderT* current = page->first_free;
  if (current == block)
  {
    page->first_free = new_block_header;
    new_block_header->next_block = current->next_block;
  }

  blockheaderT* prev;
  while (current != block)
  {
    prev = current;
    current = current->next_block;
  }

  prev->next_block = new_block_header;
  new_block_header->next_block = current->next_block;
}

void remove_malloc_header(blockheaderT* block, pageheaderT* page)
{
  blockheaderT* current = page->first_free;
  if (current == block)
  {
    page->first_free = current->next_block;
  }

  blockheaderT* prev;
  while (current != block)
  {
    prev = current;
    current = current->next_block;
  }

  prev->next_block = current->next_block;
}


#endif // KMA_RM
