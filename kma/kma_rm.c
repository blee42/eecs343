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
  short size;
  void* next_block;
 } blockheaderT;

// typedef struct pageheader_t {

//   void* page;
//   blockheaderT* first_free;
//   struct pageheader_t* next_page;

// } pageheaderT;

// #define REAL_PAGE_SIZE (8192 - sizeof(pageheaderT))
#define REAL_PAGE_SIZE (8192 - sizeof(blockheaderT))

/************Global Variables*********************************************/
kma_page_t* first_page = NULL; // entry to page structure
int total = 0;
int free_total =0;
int ignored = 0;

/************Function Prototypes******************************************/

void print_pages();

void* find_first_fit(short size);
void* new_page(short size, blockheaderT* previous_block);
void update_malloc_headers(short size, blockheaderT* current_block, blockheaderT* previous_block);
void remove_malloc_header(blockheaderT* current_block, blockheaderT* previous_block);
void coalesce();
void clear_empty_pages();

/************External Declaration*****************************************/

/**************Implementation***********************************************/

void print_pages()
{
  blockheaderT* block = *((blockheaderT**) first_page->ptr);
  void* page = BASEADDR(block);

  int i = 0;
  int num_free = 0;
  printf("==================\n");
  if (page == first_page->ptr)
    printf("     PAGE %d      \n", first_page->id);
  else
    printf("     PAGE %d      \n", (*((kma_page_t**) page))->id);
  printf("   @ %d\n", page + sizeof(blockheaderT*));
  printf("==================\n");


  while (block != NULL)
  {
    // if (BASEADDR(block) != page)
    // {
    //   page = BASEADDR(block);
    //   printf("==================\n");
    //   printf("     PAGE %d      \n", ((kma_page_t*) page)->id);
    //   printf("   @ %d\n", page + sizeof(void*));
    //   printf("==================\n");
    // }

    // if (block->size > REAL_PAGE_SIZE)
    // {
    //   int a;
    //   printf("1 to continue: \n");
    //   scanf("%d", &a);
    // }

    printf("Block %d: ", i);
    printf("size %d ", block->size);
    printf("located at %d. Next one at %d.\n", block, block->next_block);

    i++;
    num_free = num_free + block->size;
    block = (blockheaderT*) block->next_block; // ok wtf
  }
  printf("Free Space on Page: %d\n", num_free);
  printf("Availble free space: %d\n", REAL_PAGE_SIZE);
  printf("Total ignored space: %d\n", ignored);
  printf("\n");
}

void* kma_malloc(kma_size_t size)
{
  total += size;

  if (first_page == NULL)
  {
    first_page = get_page();
    blockheaderT* first_block_header = (blockheaderT*) ((int) first_page->ptr + sizeof(blockheaderT*));
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
  
  blockheaderT* current_free_block = *((blockheaderT**) first_page->ptr);
  void* first_fit = find_first_fit(size); // may point to not first page!
  // print_pages();
  return first_fit;
}

void kma_free(void* ptr, kma_size_t size)
{
  free_total += size;
  printf("--------------------------------------------------\n");
  printf("              FREE'ing %d spaces.\n", size);
  printf("              TOTAL FREED: %d\n", free_total);
  printf("--------------------------------------------------\n");
  
  blockheaderT* current_free_block = *((blockheaderT**) first_page->ptr);
  blockheaderT* previous_block = NULL;

  if ((int) size < sizeof(blockheaderT))
  {
    ignored += size;
    return;
  }

  while (current_free_block != NULL)
  {
    // printf("current_free_block: %d\n", current_free_block);
    // printf("ptr: %d\n", ptr);
    if (((int) current_free_block - (int) ptr) < sizeof(blockheaderT))
    {
      short temp_size = current_free_block->size;
      blockheaderT* temp_next = current_free_block->next_block;
      blockheaderT* new_block = (blockheaderT*) ptr;
      new_block->size = (short) size + temp_size;
      new_block->next_block = temp_next;
      
      if (previous_block == NULL)
      {
        *((blockheaderT**) (first_page->ptr)) = new_block;
      }
      else
      {
        previous_block->next_block = (void*) new_block;
      }

      coalesce();
      clear_empty_pages();
      print_pages();
      return;
    } 
    else if ((int) current_free_block > (int) ptr)
    {
      blockheaderT* new_block = (blockheaderT*) ptr;
      new_block->size = (short) size; 
      new_block->next_block = (void*) current_free_block;

      if (previous_block == NULL)
      {
        *((blockheaderT**) (first_page->ptr)) = new_block;
      }
      else
      {
        previous_block->next_block = (void*) new_block;
      }

      coalesce();
      clear_empty_pages();
      print_pages();
      return;
    }
    else
    {
      previous_block = current_free_block;
      current_free_block = (blockheaderT*) current_free_block->next_block;
    }
  }
}

void* find_first_fit(short size)
{
  blockheaderT* current_free_block = *((blockheaderT**) first_page->ptr);
  blockheaderT* previous_block = NULL;

  while (current_free_block != NULL)
  {
    if (current_free_block->size - size < sizeof(blockheaderT))
    {
      // remove_malloc_header(current_free_block, previous_block);
      // return (void*) current_free_block;
      previous_block = current_free_block;
      current_free_block = (blockheaderT*) current_free_block->next_block;
    }
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
  return new_page(size, previous_block);
}

void* new_page(short size, blockheaderT* previous_block)
{
  kma_page_t* page = get_page();
  *((kma_page_t**)page->ptr) = page;

  blockheaderT* first_block_header = (blockheaderT*) ((int) page->ptr + sizeof(blockheaderT) + (int) size);
  first_block_header->size = REAL_PAGE_SIZE-size;
  first_block_header->next_block = NULL;

  if (previous_block == NULL)
  {
    *((blockheaderT**) first_page->ptr) = first_block_header;
  }
  else
  {
    previous_block->next_block = first_block_header;
  }

  return (void*)((int) page->ptr + sizeof(blockheaderT));
}

void update_malloc_headers(short size, blockheaderT* current_block, blockheaderT* previous_block)
{
  blockheaderT* new_next = (blockheaderT*) current_block->next_block;
  short new_size = current_block->size - size;
  
  blockheaderT* new_block;
  new_block = (blockheaderT*) ((int) current_block + size);

  new_block->size = new_size;
  new_block->next_block = (void*) new_next;

  if (previous_block == NULL)
  {
    *((blockheaderT**) (first_page->ptr)) = new_block;
  }
  else
  {
    previous_block->next_block = (void*) new_block;
  }
}

void remove_malloc_header(blockheaderT* current_block, blockheaderT* previous_block)
{
  if (previous_block == NULL)
  {
    *((blockheaderT**) first_page->ptr) = current_block->next_block;
  }
  else
  {
    previous_block->next_block = current_block->next_block;
  }
}

void coalesce()
{
  // printf("do we ever get here?\n");
  // int a;
  // printf("1 to continue: \n");
  // scanf("%d", &a);
  blockheaderT* current_block = *((blockheaderT**) first_page->ptr);
  blockheaderT* previous = NULL;
  while (current_block != NULL)
  {
    if (current_block->next_block != NULL &&
      BASEADDR(current_block) == BASEADDR(current_block->next_block) &&
      (((int) current_block->next_block) - ((int) current_block + current_block->size)) == 0)
    {
      current_block->size += ((blockheaderT*) (current_block->next_block))->size;
      current_block->next_block = (void*) ((blockheaderT*) (current_block->next_block))->next_block;
    }
    else
    {
      previous = current_block;
      current_block = (blockheaderT*) current_block->next_block;
    }    
  }
}

void clear_empty_pages()
{
  blockheaderT* current_block = *((blockheaderT**) first_page->ptr);
  blockheaderT* previous = NULL;
  while (current_block != NULL)
  {
    int available_space = REAL_PAGE_SIZE - current_block->size;
    if (current_block->size == REAL_PAGE_SIZE)
    {
      void* current_page = BASEADDR(current_block);
      print_pages();

      // Not the first page, and in the middle of the list
      if (current_page != (first_page->ptr) && previous != NULL)
      {
        previous->next_block = current_block->next_block;
        free_page(*((kma_page_t**) current_page));
      }
      // Not the first page, but the first free block
      else if (current_page != (first_page->ptr) && previous == NULL)
      {
        *((blockheaderT**) first_page->ptr) = (blockheaderT*) current_block->next_block;
        free_page(*((kma_page_t**) current_page));
      }
      // first page
      else if (current_page == (first_page->ptr))
      {
        kma_page_t* temp = first_page;
        if (current_block->next_block != NULL)
        {
          first_page = *((kma_page_t**) BASEADDR(current_block->next_block));
          first_page->ptr = current_block->next_block;
        }
        else
        {
          first_page = NULL;
        }

        free_page(temp);
      }
    }
    previous = current_block;
    current_block = (blockheaderT*) current_block->next_block;
  }
}

#endif // KMA_RM
