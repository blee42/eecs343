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
   void* start;
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
  struct pageheader_t* next;
  freeheadersL free_headers;
  int used;
  // int[32] bitmap; // = {0}
 } pageheaderT;

#define REALSIZE(size) (size+sizeof(allocheaderT))
#define PAGE_HEADER(page) ((pageheaderT*) (page->ptr + sizeof(allocheaderT)))
#define ALLOC_START(ptr) ((void*) ((long int) ptr + sizeof(allocheaderT)))

/************Global Variables*********************************************/

kma_page_t* first_page = NULL;
int total = 0;

/************Function Prototypes******************************************/

void print_pages();
kma_page_t* init_page(kma_page_t*);
void* get_free_entry();
void* get_free_buffer(int);
freelistL* get_required_buffer(int, pageheaderT*);
freelistL* split_buffer(freelistL*, pageheaderT*);

void coalesce();
allocheaderT* get_buddy();
int round_up(int);
    
/************External Declaration*****************************************/

/**************Implementation***********************************************/

void print_pages()
{
  pageheaderT* page = PAGE_HEADER(first_page);
  freelistL* buffer = get_required_buffer(1, page);
  allocheaderT* alloc = buffer->first_block;

  int i = 32;
  int j = 0;

  while (page != NULL)
  {

    printf("-----------------\n");
    printf("     PAGE %d\n", page->page->id);
    printf("      AT: %d\n", page);
    printf("    USED: %d\n", page->used);
    printf("    NEXT: %d\n", page->next);
    printf("-----------------\n");
    while (buffer != NULL)
    {
      alloc = buffer->first_block;
      printf("\n** Buffer %d **\n", i);
      while (alloc != NULL)
      {
        printf("* Free %d: %d, next: %d, of size: %d\n", j, alloc, alloc->next, alloc->size);
        alloc = alloc->next;
        j++;
      }
      j = 0;
      i = i * 2;
      buffer = buffer->bigger_size;
    }
    page = page->next;
    buffer = get_required_buffer(1, page);
    i = 32;
  }

  // if (total >= 67678)
  //   scanf("%d", &i);
}

void* kma_malloc(kma_size_t size)
{
  total += size;
  if (REALSIZE(size)+sizeof(pageheaderT) > PAGESIZE)
    return NULL;

  if (first_page == NULL)
  {
    first_page = init_page(first_page);
    kma_malloc(sizeof(pageheaderT));
    PAGE_HEADER(first_page)->used = 0;
  }

  printf("--------------------------------------------------\n");
  printf("             MALLOC'ing %d spaces.\n", size);
  printf("              TOTAL REQUESTED: %d\n", total);
  printf("--------------------------------------------------\n");
  void* allocation = get_free_buffer(size);
  // print_pages();
  return allocation;
}

void kma_free(void* ptr, kma_size_t size)
{


  printf("--------------------------------------------------\n");
  printf("              FREE'ing %d spaces.\n", size);
  printf("                @: %d\n", ptr);
  printf("--------------------------------------------------\n");

  int a;  
  // print_pages();
  pageheaderT* page = (pageheaderT*) (BASEADDR(ptr)+sizeof(allocheaderT));
  freelistL* buffer = get_required_buffer(size, page);
  allocheaderT* alloc = buffer->first_block;

  allocheaderT* new_alloc = (allocheaderT*) (ptr);
  new_alloc->start = ALLOC_START(new_alloc);
  new_alloc->size = round_up(size)  ; // fix this
  new_alloc->next = NULL;

  if (new_alloc->size != PAGESIZE)
    if (alloc == NULL)
    {
      buffer->first_block = new_alloc;
    }
    else if ((long int) alloc > (long int) new_alloc)
    {
      new_alloc->next = alloc;
      buffer->first_block = new_alloc;
    }
    else
    {
      while (alloc->next != NULL)
      {
        if ((long int) alloc->next > (long int) new_alloc)
        {
          new_alloc->next = alloc->next;
          alloc->next = new_alloc;
          break;
        }
        else
        {
          alloc = alloc->next;
        }
      } 
    coalesce();
    }

  page->used -= 1;

  if (page->used == 0)
  {
    if (page->page == first_page && page->next != NULL)
    {
      kma_page_t* temp = page->next->page;
      free_page(first_page);
      first_page = temp;
    }
    else if (page->page == first_page)
    {
      free_page(first_page);
      first_page = NULL;
    }
    else
    {
      pageheaderT* temp = PAGE_HEADER(first_page);
      pageheaderT* prev = NULL;

      while (temp != page)
      {
        prev = temp;
        temp = temp->next;
      }
      prev->next = page->next;
      free_page(page->page);
    }
  }
}

kma_page_t* init_page(kma_page_t* page)
{
  page = get_page();
  pageheaderT* page_headers = PAGE_HEADER(page);
  page_headers->page = page;
  page_headers->next = NULL;
  page_headers->used = 0;

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

  freeheaders->buffer32.first_block = NULL;
  freeheaders->buffer64.first_block = NULL;
  freeheaders->buffer128.first_block = NULL;
  freeheaders->buffer256.first_block = NULL;
  freeheaders->buffer512.first_block = NULL;
  freeheaders->buffer1024.first_block = NULL;
  freeheaders->buffer2048.first_block = NULL;
  freeheaders->buffer4096.first_block = NULL;
  freeheaders->buffer8192.first_block = NULL;

  allocheaderT* buffer = (allocheaderT*) (page->ptr);
  buffer->start = ALLOC_START(buffer);
  buffer->size = 8192;
  buffer->next = NULL;
  freeheaders->buffer8192.first_block = buffer;

  return page;
}

void* get_free_buffer(int size)
{
  pageheaderT* page_header = PAGE_HEADER(first_page);
  pageheaderT* prev_page = NULL;

  int i = 0;
  int search_size = round_up(REALSIZE(size));
  allocheaderT* to_be_allocated;
  freelistL* buffer_size = get_required_buffer(search_size, page_header);

  while (search_size <= PAGESIZE)
  {
    while (page_header != NULL && buffer_size->first_block == NULL)
    {
      prev_page = page_header;
      page_header = page_header->next;
      buffer_size = get_required_buffer(search_size, page_header);
    }

    // print_pages();

    if (buffer_size == NULL)
    {
      i++;
      page_header = PAGE_HEADER(first_page);
      prev_page = NULL;
      search_size = search_size * 2;
      buffer_size = get_required_buffer(search_size, page_header);
    }
    else
    {
      to_be_allocated = buffer_size->first_block;
      if (i == 0)
      {
        buffer_size->first_block = to_be_allocated->next;
      }
      else
      {
        for(; i > 0; i--)
        {
          buffer_size = split_buffer(buffer_size, page_header);
        }
        to_be_allocated = buffer_size->first_block;
        buffer_size->first_block = to_be_allocated->next;
      }

      page_header->used += 1;

      return to_be_allocated;
    }
  }


  while (page_header != NULL)
  {
    prev_page = page_header;
    page_header = page_header->next;
  }

  kma_page_t* new_page = init_page(NULL);
  page_header = PAGE_HEADER(new_page);
  prev_page->next = page_header;

  if (round_up(REALSIZE(size)) == PAGESIZE)
  {
    page_header->free_headers.buffer32.first_block = NULL;
    page_header->free_headers.buffer64.first_block = NULL;
    page_header->free_headers.buffer128.first_block = NULL;
    page_header->free_headers.buffer256.first_block = NULL;
    page_header->free_headers.buffer512.first_block = NULL;
    page_header->free_headers.buffer1024.first_block = NULL;
    page_header->free_headers.buffer2048.first_block = NULL;
    page_header->free_headers.buffer4096.first_block = NULL;
    page_header->free_headers.buffer8192.first_block = NULL;
    page_header->used += 1;
    return (page_header+sizeof(pageheaderT));
  }


  buffer_size = get_required_buffer(sizeof(pageheaderT), page_header);
  to_be_allocated = buffer_size->first_block;

  i = 0;

  while (buffer_size != NULL && buffer_size->first_block == NULL)
  {
    buffer_size = buffer_size->bigger_size;
    i++;
  }

  printf("I value: %d\n", i);

  for(; i > 0; i--)
  {
    buffer_size = split_buffer(buffer_size, page_header);
  }

  to_be_allocated = buffer_size->first_block;
  buffer_size->first_block = to_be_allocated->next;

  return get_free_buffer(size);
}

freelistL* get_required_buffer(int size, pageheaderT* page_header)
{
  if (page_header == NULL)
  {
    return NULL;
  }

  freeheadersL* freeheaders = &page_header->free_headers;
  if (size <= 32)
    return &freeheaders->buffer32;
  else if (size <= 64)
    return &freeheaders->buffer64;
  else if (size <= 128)
    return &freeheaders->buffer128;
  else if (size <= 256)
    return &freeheaders->buffer256;
  else if (size <= 512)
    return &freeheaders->buffer512;
  else if (size <= 1024)
    return &freeheaders->buffer1024;
  else if (size <= 2048)
    return &freeheaders->buffer2048;
  else if (size <= 4096)
    return &freeheaders->buffer4096;
  else
    return &freeheaders->buffer8192;
}

// return a pointer to the left buffer
freelistL* split_buffer(freelistL* buffer_size, pageheaderT* page_header)
{
  // remove from original list
  allocheaderT* original = buffer_size->first_block;
  buffer_size->first_block = original->next;

  // make two new ones
  int new_size = (original->size) / 2;
  allocheaderT* new_buffer = (allocheaderT*) ((long int) original + new_size); // -1?
  new_buffer->size = new_size;
  new_buffer->start = ALLOC_START(new_buffer);
  new_buffer->next = NULL;
  original->size = new_size;
  original->next = new_buffer;

  // add both new ones to the new list
  freelistL* new_buffer_size = get_required_buffer(new_size, page_header);
  allocheaderT* current_buffer = new_buffer_size->first_block;
  if (current_buffer == NULL)
  {
    new_buffer_size->first_block = original;
  }
  else
  {
    while (current_buffer->next != NULL)
    {
      current_buffer = current_buffer->next;
    }
    current_buffer->next = original;    
  }

  return new_buffer_size;
}

allocheaderT* get_buddy(allocheaderT* alloc)
{
  int diff = (long int) alloc - (long int) BASEADDR(alloc);

  if ((diff / (alloc->size)) & 1) // odd
    return (allocheaderT*) ((long int) alloc - (long int) alloc->size);
  else
    return (allocheaderT*) ((long int) alloc + (long int) alloc->size);
}


void coalesce()
{
  pageheaderT* page = PAGE_HEADER(first_page);
  freelistL* buffer = get_required_buffer(1, page);
  allocheaderT* alloc;
  allocheaderT* prev = NULL;

  while (page != NULL)
  {
    while (buffer != NULL)
    {
      alloc = buffer->first_block;
      while (alloc != NULL)
      {
        if (alloc->next != NULL && (get_buddy(alloc) == alloc->next))
        {
          if (prev == NULL)
            buffer->first_block = alloc->next->next;
          else
            prev->next = alloc->next->next;
          page->used += 1;
          kma_free(alloc, alloc->size * 2);
          return;
        }
        else
        {
          alloc = alloc->next;
        }
        
      }
      buffer = buffer->bigger_size;
    }
    page = page->next;
    buffer = get_required_buffer(1, page);
  }
}

int round_up(int x)
{
  x--;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  x++;
  if (x < 32)
    x = 32;
  return x;
}


#endif // KMA_BUD
