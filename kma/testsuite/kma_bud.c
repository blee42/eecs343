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

/* Structure at the top of each power of two free block. Contains a pointer
   to the end of this structure, where actual free memory space begins, a
   size, and a pointer to the next free block. */
 typedef struct allocheader_t {
   void* start;
   int size;
   struct allocheader_t* next;
 } allocheaderT;

/* Linked list of all free allocheaders of a given size. Contains a pointer
   to the type for free allocheaders of a larger size for easy iteration. */
typedef struct freelist_l {
  allocheaderT* first_block;
  struct freelist_l* bigger_size; // 32 -> 64, 64 -> 128, etc.
} freelistL;

/* Struct containing all the possible sizes of free buffers. Exists in the
   page header. */
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

/* Header of each page, contains link to the page object, the next page, 
   indicates how many blocks have been allocated, and a list of all the
   free buffers. */
typedef struct pageheader_t {
  kma_page_t* page;
  struct pageheader_t* next;
  freeheadersL free_headers;
  int used;
 } pageheaderT;

// Gets the real size need when malloc'ing, including header.
#define REALSIZE(size) (size + sizeof(allocheaderT))

// Gets the page header given the page.
#define PAGE_HEADER(page) ((pageheaderT*) ((long int) page->ptr + sizeof(allocheaderT)))

// Gets the malloc'ed space, given the location of the allocation.
#define ALLOC_START(ptr) ((void*) ((long int) ptr + sizeof(allocheaderT)))

/************Global Variables*********************************************/

kma_page_t* first_page = NULL;

/************Function Prototypes******************************************/

// void print_pages(); debug function

kma_page_t* init_page();
void* get_free_entry();
void* get_free_buffer(int);
freelistL* get_required_buffer(int, pageheaderT*);
freelistL* split_buffer(freelistL*, pageheaderT*);

void coalesce(pageheaderT*);
allocheaderT* get_buddy();
int round_up(int);
    
/************External Declaration*****************************************/

/**************Implementation***********************************************/

// debugging function. triggers lots of warnings!
// void print_pages()
// {
//   pageheaderT* page = PAGE_HEADER(first_page);
//   freelistL* buffer = get_required_buffer(1, page);
//   allocheaderT* alloc = buffer->first_block;

//   int i = 32;
//   int j = 0;

//   while (page != NULL)
//   {

//     printf("-----------------\n");
//     printf("     PAGE %d\n", page->page->id);
//     printf("      AT: %d\n", page);
//     printf("    USED: %d\n", page->used);
//     printf("    NEXT: %d\n", page->next);
//     printf("-----------------\n");
//     while (buffer != NULL)
//     {
//       alloc = buffer->first_block;
//       printf("\n** Buffer %d **\n", i);
//       while (alloc != NULL)
//       {
//         printf("* Free %d: %d, next: %d, of size: %d\n", j, alloc, alloc->next, alloc->size);
//         alloc = alloc->next;
//         j++;
//       }
//       j = 0;
//       i = i * 2;
//       buffer = buffer->bigger_size;
//     }
//     page = page->next;
//     buffer = get_required_buffer(1, page);
//     i = 32;
//   }
// }

/* malloc function. Initializes the first page if needed, then
   returns the appropriate allocated space. */
void* kma_malloc(kma_size_t size)
{
  if (REALSIZE(size)+sizeof(pageheaderT) > PAGESIZE)
    return NULL;

  if (first_page == NULL)
  {
    first_page = init_page();
    kma_malloc(sizeof(pageheaderT));    // mallocs the page header so it isn't overwritten.
    PAGE_HEADER(first_page)->used = 0;  // resets the allocation counter because the page
                                        // header doesn't count.
  }

  return get_free_buffer(size);
}

/* free function. Finds the block allocatd at the pointer and frees it, adding
   to the appropriately sized free buffer list, then coalesces the page and
   frees the page if needed. Free buffers of the same size are ordered by their
   addresses. */
   
void kma_free(void* ptr, kma_size_t size)
{
  pageheaderT* page = (pageheaderT*) (BASEADDR(ptr)+sizeof(allocheaderT));
  freelistL* buffer = get_required_buffer(size, page);
  allocheaderT* alloc = buffer->first_block;

  // can't do any of this entire page is allocated to one chunk.
  if (round_up(size) != PAGESIZE)
  {
    // creating new free buffer
    allocheaderT* new_alloc = (allocheaderT*) (ptr);
    new_alloc->start = ALLOC_START(new_alloc);
    new_alloc->size = round_up(size);
    new_alloc->next = NULL;

    // insert buffer as first item if empty
    if (alloc == NULL)
      buffer->first_block = new_alloc;
    // insert buffer as first item when not empty
    else if ((long int) alloc > (long int) new_alloc)
    {
      new_alloc->next = alloc;
      buffer->first_block = new_alloc;
    }
    // find the appropriate location and insert the bufer
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
          alloc = alloc->next;
      } 
    coalesce(page);
    }
  }

  // if no more elements have been allocated to this page, free it.
  page->used -= 1;

  if (page->used == 0)
  {
    // case 1: freed first page, but there are still some pages existing.
    if (page->page == first_page && page->next != NULL)
    {
      kma_page_t* temp = page->next->page;
      free_page(first_page);
      first_page = temp;
    }
    // case 2: all pages are being freed.
    else if (page->page == first_page)
    {
      free_page(first_page);
      first_page = NULL;
    }
    // case 3: freeing any other page
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

/* Creates the initial pageheader struct at the begnning of the page.
   Links each of the buffers to the larger size and sets the list of free
   buffers to be NULL, then creates a single buffer of size PAGESIZE.*/
kma_page_t* init_page()
{
  kma_page_t* page = get_page(); 
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
  buffer->size = PAGESIZE;
  buffer->next = NULL;
  freeheaders->buffer8192.first_block = buffer;

  return page;
}

/* Searches all of the free buffers across all pages for one that
   properly fits the requested size (adjusted for alloc header).
   This searches all pages for given buffer size before moving on
   to the bigger size, splitting the larger buffer down if necessary.

   If none of the pages can fit the requested size, a new page is
   allocated and a page header created and allocated. In the special
   case of the buffer request being > PAGESIZE/2 a new page is always
   allocated.
*/
void* get_free_buffer(int size)
{
  pageheaderT* page_header = PAGE_HEADER(first_page);
  pageheaderT* prev_page = NULL;

  int i = 0;
  int search_size = round_up(REALSIZE(size));
  allocheaderT* to_be_allocated;
  freelistL* buffer_size = get_required_buffer(search_size, page_header);

  // Loops through each buffer size
  while (search_size <= PAGESIZE)
  {
    // Loops through each page until it goes through all of them or
    // finds a free buffer.
    while (page_header != NULL && buffer_size->first_block == NULL)
    {
      prev_page = page_header;
      page_header = page_header->next;
      buffer_size = get_required_buffer(search_size, page_header);
    }

    // None found, increase size and repeat
    if (buffer_size == NULL)
    {
      i++;
      page_header = PAGE_HEADER(first_page);
      prev_page = NULL;
      search_size = search_size * 2;
      buffer_size = get_required_buffer(search_size, page_header);
    }
    // Found. Splits depending on the number of times through the loop,
    // removes the buffer from the free list, and returns it.
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

  // No buffers found. Find the last page again.
  while (page_header != NULL)
  {
    prev_page = page_header;
    page_header = page_header->next;
  }

  // Create a new page, split it up to insert the header.
  kma_page_t* new_page = init_page();
  page_header = PAGE_HEADER(new_page);
  prev_page->next = page_header;

  buffer_size = get_required_buffer(sizeof(pageheaderT), page_header);
  to_be_allocated = buffer_size->first_block;

  i = 0;
  while (buffer_size != NULL && buffer_size->first_block == NULL)
  {
    buffer_size = buffer_size->bigger_size;
    i++;
  }
  for(; i > 0; i--)
  {
    buffer_size = split_buffer(buffer_size, page_header);
  }

  to_be_allocated = buffer_size->first_block;
  buffer_size->first_block = to_be_allocated->next;

  // If size > PAGESIZE/2, clears free headers and allocates the
  // whole page.
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
    return (void*)((long int) page_header + sizeof(pageheaderT));
  }

  // Now that the page has been created, try allocating again.
  return get_free_buffer(size);
}

/* Given a size and page, returns the appropriate free buffer list */
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

/* Takes a free buffer on a page, splits it in two, adds/removes appropriate
   entries on the page header, then returns the left buffer for further
   processing. */
freelistL* split_buffer(freelistL* buffer_size, pageheaderT* page_header)
{
  // Remove buffer from original list
  allocheaderT* original = buffer_size->first_block;
  buffer_size->first_block = original->next;

  // Create two new buffers
  int new_size = (original->size) / 2;
  allocheaderT* new_buffer = (allocheaderT*) ((long int) original + new_size); // -1?
  new_buffer->size = new_size;
  new_buffer->start = ALLOC_START(new_buffer);
  new_buffer->next = NULL;
  original->size = new_size;
  original->next = new_buffer;

  // Add two new buffers to appropriate list
  freelistL* new_buffer_size = get_required_buffer(new_size, page_header);
  allocheaderT* current_buffer = new_buffer_size->first_block;
  if (current_buffer == NULL)
    new_buffer_size->first_block = original;
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

/* Computes the location of a blocks buddy. This function relies on if the block
   is an even or odd multiple of its size away from the beginning, which indicates
   if the buddy will be on the left or the right. */
allocheaderT* get_buddy(allocheaderT* alloc)
{
  int diff = (long int) alloc - (long int) BASEADDR(alloc);

  if ((diff / (alloc->size)) & 1) // odd
    return (allocheaderT*) ((long int) alloc - (long int) alloc->size);
  else
    return (allocheaderT*) ((long int) alloc + (long int) alloc->size);
}

/* Iterates through a page and coalesces free buffers. Called only after
   freeing allocated space on the page. Checks a buffer with its buddy and coalesces
   the two if both are free. */
void coalesce(pageheaderT* page)
{
  freelistL* buffer = get_required_buffer(1, page);
  allocheaderT* alloc;
  allocheaderT* prev = NULL;

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
        alloc = alloc->next; 
    }
    buffer = buffer->bigger_size;
  }
}

/* Helper function that uses bitwise operators to round a number
   to the higher power of 2. */
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
