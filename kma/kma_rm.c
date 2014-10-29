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
  struct blockheader_t* next_block;

 } blockheaderT;

// typedef struct pageheader_t {

//   void* page;
//   blockheaderT* first_free;
//   struct pageheader_t* next_page;

// } pageheaderT;

// #define REAL_PAGE_SIZE (8192 - sizeof(pageheaderT))
#define REAL_PAGE_SIZE (8192 - sizeof(blockheaderT*))

/************Global Variables*********************************************/
kma_page_t* first_page = NULL; // entry to page structure
int total = 0;

/************Function Prototypes******************************************/

void print_pages();

void* find_first_fit(int size);
void* new_page(int size, blockheaderT* previous_block);
void update_malloc_headers(int size, blockheaderT* current_block, blockheaderT* previous_block);
void remove_malloc_header(blockheaderT* current_block, blockheaderT* previous_block);
void coalesce();

/************External Declaration*****************************************/

/**************Implementation***********************************************/

void print_pages()
{
  blockheaderT* block = *((blockheaderT**) first_page->ptr);
  void* page = BASEADDR(block);
  int i = 0;
  int pagenum = 0;
  printf("==================\n");
  printf("     PAGE %d      \n", pagenum);
  printf(" @ %d\n", page + sizeof(blockheaderT*));
  printf("==================\n");


  while (block != NULL)
  {

    if (BASEADDR(block) != page)
    {
      page = BASEADDR(block);
      printf("==================\n");
      printf("     PAGE %d      \n", pagenum);
      printf(" @ %d\n", page + sizeof(blockheaderT*));
      printf("==================\n");
    }

    printf("Block %d: ", i);
    printf("FREE, size %d ", block->size);
    printf("located at %d.\n\n", block);

    i++;
    block = block->next_block; // ok wtf
  }

}

void* kma_malloc(kma_size_t size)
{
  total += size;

  if (first_page == NULL)
  {
    first_page = get_page();
    blockheaderT* first_block_header = (blockheaderT*) (first_page->ptr + sizeof(blockheaderT*));
    first_block_header->size = REAL_PAGE_SIZE;
    first_block_header->next_block = NULL;
    *((blockheaderT**) first_page->ptr) = first_block_header;
  }


  if (size > REAL_PAGE_SIZE)
  {
    // free_page(current_page);
    return NULL;  
  }

  printf("--------------------------------------------------\n");
  printf("             MALLOC'ing %d spaces.\n", size);
  printf("              TOTAL REQUESTED: %d\n", total);
  printf("--------------------------------------------------\n");
  void* first_fit = find_first_fit(size); // may point to not first page!
  // print_pages();
  return first_fit;
}

void kma_free(void* ptr, kma_size_t size)
{
  printf("FREE'ing %d spaces.\n", size);
  // blockheaderT* first_block_header = *(blockheaderT**) BASEADDR(ptr);
  // blockheaderT* previous_block = NULL;

  // while (first_block_header != NULL)
  // {
  //   if ((int)first_block_header > (int)ptr)
  //   {
  //     // update_malloc_headers(size, cu)
  //   }
  // }
//   pageheaderT* current_page_header = (pageheaderT*) (first_page->ptr);

//   while (current_page_header->page + PAGESIZE < ptr)
//   {
//     current_page_header = current_page_header->next_page;
//   }

//   blockheaderT* new_block_header = (blockheaderT*) ptr; // need &?
//   new_block_header->size = size;
//   new_block_header->base = ptr;

//   blockheaderT* current = current_page_header->first_free;
//   if (current == NULL)
//   {
//     current_page_header->first_free = new_block_header;
//     new_block_header->next_block = NULL;
//   }

//   blockheaderT* prev;
//   while (current->base < ptr)
//   {
//     prev = current;
//     current = current->next_block;
//   }

//   prev->next_block = new_block_header;
//   new_block_header->next_block = current->next_block;

//   coalesce();
}

void* find_first_fit(int size)
{
  blockheaderT* current_free_block = *((blockheaderT**) first_page->ptr);
  blockheaderT* previous_block = NULL;
  printf("loc of current: %d\n", current_free_block->next_block);
  printf("size: %d\n", current_free_block->size);

  while (current_free_block != NULL)
  {
    if (current_free_block->size - size <= sizeof(blockheaderT))
    {
      remove_malloc_header(current_free_block, previous_block);
      return (void*) current_free_block;
    }
    else if (size < current_free_block->size)
    {
      update_malloc_headers(size, current_free_block, previous_block);
      return (void*) current_free_block;
    }
    else
    {
      previous_block = current_free_block;
      current_free_block = current_free_block->next_block;
    }
  }

  // no openings, new page
  return new_page(size, previous_block);
}

void* new_page(int size, blockheaderT* previous_block)
{
  kma_page_t* page = get_page();
  blockheaderT* first_block_header = (blockheaderT*) (page->ptr + sizeof(blockheaderT*) + size);
  first_block_header->size = REAL_PAGE_SIZE-size;
  first_block_header->next_block = NULL;
  *((blockheaderT**) page->ptr) = first_block_header;

  previous_block->next_block = first_block_header;

  return (void*)(page->ptr + sizeof(blockheaderT*));
}

void update_malloc_headers(int size, blockheaderT* current_block, blockheaderT* previous_block)
{
  int temp_size = current_block->size - size;
  blockheaderT* temp_next = current_block->next_block;

  blockheaderT* new_block;
  new_block = (blockheaderT*) ((long int) current_block + size);
  new_block->size = temp_size;
  new_block->next_block = temp_next;    

  if (previous_block == NULL)
  {
    *((blockheaderT**) (BASEADDR(current_block))) = new_block;
  }
  else
  {
    // case where all space is taken up does not apply here
    previous_block->next_block = new_block;
  }
}

void remove_malloc_header(blockheaderT* current_block, blockheaderT* previous_block)
{
  if (previous_block == NULL)
  {

  }
  previous_block->next_block = current_block->next_block;
  // blockheaderT* current = page->first_free;
  // if (current == block)
  // {
  //   page->first_free = current->next_block;
  // }

  // blockheaderT* prev;
  // while (current != block)
  // {
  //   prev = current;
  //   current = current->next_block;
  // }

  // prev->next_block = current->next_block;
}

void coalesce()
{
//   pageheaderT* current_page_header = (pageheaderT*) (first_page->ptr);
//   pageheaderT* previous_page_header;
//   blockheaderT* current_block = current_page_header->first_free;
//   blockheaderT* next_block;
  
//   // special conditions stuff for pages
  
//   while (current_page_header != NULL && current_page_header->next_page != NULL)
//   {
//     while (current_block != NULL && current_block->next_block != NULL)
//     {
//       next_block = current_block->next_block;
//       if (next_block->base - (current_block->base + current_block->size) <= sizeof(blockheaderT))
//       {
//         current_block->size = next_block->base + next_block->size - current_block->base;
//         remove_malloc_header(next_block, current_page_header);
//       }
//       else
//       {
//         current_block = current_block->next_block;
//       }
//     }
    
//     if (current_page_header->first_free->size == REAL_PAGE_SIZE)
//     {
//       if (previous_page_header == NULL)
//       {
//         // previous page is actually second page
//         previous_page_header = current_page_header->next_page;
//         free_page((kma_page_t*) current_page_header->page);
//         first_page->ptr = previous_page_header->page;
        
//         current_page_header = previous_page_header;
//         previous_page_header = NULL; // in case multiple empty pages
//       }
//       else
//       {
//         previous_page_header->next_page = current_page_header->next_page;
//         free_page((kma_page_t*) current_page_header->page);
//         current_page_header = previous_page_header->next_page;
//       }
//     }
//     else
//     {
//       previous_page_header = current_page_header;
//       current_page_header = current_page_header->next_page;
//     }
//   }
}


#endif // KMA_RM
