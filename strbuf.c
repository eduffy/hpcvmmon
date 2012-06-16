
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "strbuf.h"

struct StringBuffer *new_string_buffer()
{
   struct StringBuffer *sb = (struct StringBuffer *)malloc(sizeof(struct StringBuffer));
   size_t capacity = getpagesize();
   sb->buffer = sb->position = (char *)malloc(capacity);
   sb->end = sb->buffer + capacity;
   return sb;
}

void sbprintf(struct StringBuffer *sb, char *format, ...)
{
   va_list args;
   int len;
   ptrdiff_t remaining = sb->end - sb->position;
  
   /* make sure there's enough room in the string buffer
    * before the write */
   va_start(args, format);
   len = vsnprintf(NULL, 0, format, args);
   va_end(args);
   if(remaining <= len) {
      ptrdiff_t size = sb->position - sb->buffer;
      ptrdiff_t capacity = 2 * (sb->end - sb->buffer);
      sb->buffer = realloc(sb->buffer, capacity);
      sb->position = sb->buffer + size;
      sb->end = sb->buffer + capacity;
      fprintf(stderr, "new buf size = %ld\n", capacity);
   }

   va_start(args, format);
   len = vsprintf(sb->position, format, args);
   va_end(args);
   sb->position += len;
   assert(sb->position <= sb->end);
}

char *free_string_buffer(struct StringBuffer *sb, int free_contents)
{
   char *b = sb->buffer;
   free(sb);
   if(free_contents) {
      free(b);
      return NULL;
   }
   return b;
}

int string_buffer_is_empty(struct StringBuffer *sb)
{
   return sb->buffer == sb->position;
}
