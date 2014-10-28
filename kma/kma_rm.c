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

void print_page(pageheaderT* page)
{
  blockheaderT* block = page->first_free;
  int i = 0;
  int j = 0;

  while (page != NULL)
  {
    printf("==================\n");
    printf("     PAGE %d      \n", i);
    printf("==================\n");

    while (block != NULL)
    {
      printf("Block %d: ", j);
      printf("FREE, size %d", block->size);
      block = block->next_block;
      j++;

    }
    j = 0;
    i++;
    page = page->next_page;
  }
  printf("\n");

}

void* kma_malloc(kma_size_t size)
{

  // kma_page_t* current_page = first_page;
  pageheaderT* current_page_header;

  if (first_page == NULL)
  {
    current_page_header = init_page();
    first_page = (kma_page_t*) (current_page_header->page);
  }


  if (size > REAL_PAGE_SIZE)
  {
    // free_page(current_page);
    return NULL;  
  }

  printf("--------------------------------------------------\n");
  printf("             MALLOC'ing %d spaces.\n", size);
  printf("--------------------------------------------------\n");
  void* first_fit = find_first_fit(size, current_page_header); // may point to not first page!
  print_page((pageheaderT*)first_page->ptr);
  return first_fit;
}

void kma_free(void* ptr, kma_size_t size)
{
  printf("FREE'ing %d spaces.\n", size);
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
  *((kma_page_t**)current_page->ptr) = current_page;

  pageheaderT* page_header = (pageheaderT*) (current_page->ptr);
  blockheaderT* first_block_header = (blockheaderT*) (page_header + sizeof(pageheaderT));

  page_header->page = current_page->ptr;
  page_header->first_free = first_block_header;

  first_block_header->size = REAL_PAGE_SIZE;
  first_block_header->base = (void*) (current_page->ptr + sizeof(pageheaderT));

  return page_header;
}

void* find_first_fit(int size, pageheaderT* current_page)
{
  printf("Finding first fit of size: %d\n", size);
  blockheaderT* current_free_block = current_page->first_free;
  printf("lakshdklfasdfk");
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

void coalesce()
{
  pageheaderT* current_page_header = (pageheaderT*) (first_page->ptr);
  pageheaderT* previous_page_header;
  blockheaderT* current_block = current_page_header->first_free;
  blockheaderT* next_block;
  
  // special conditions stuff for pages
  
  while (current_page_header != NULL && current_page_header->next_page != NULL)
  {
    while (current_block != NULL && current_block->next_block != NULL)
    {
      next_block = current_block->next_block;
      if (next_block->base - (current_block->base + current_block->size) <= sizeof(blockheaderT))
      {
        current_block->size = next_block->base + next_block->size - current_block->base;
        remove_malloc_header(next_block, current_page_header);
      }
      else
      {
        current_block = current_block->next_block;
      }
    }
    
    if (current_page_header->first_free->size == REAL_PAGE_SIZE)
    {
      if (previous_page_header == NULL)
      {
        // previous page is actually second page
        previous_page_header = current_page_header->next_page;
        free_page((kma_page_t*) current_page_header->page);
        first_page->ptr = previous_page_header->page;
        
        current_page_header = previous_page_header;
        previous_page_header = NULL; // in case multiple empty pages
      }
      else
      {
        previous_page_header->next_page = current_page_header->next_page;
        free_page((kma_page_t*) current_page_header->page);
        current_page_header = previous_page_header->next_page;
      }
    }
    else
    {
      previous_page_header = current_page_header;
      current_page_header = current_page_header->next_page;
    }
  }
}


#endif // KMA_RM
