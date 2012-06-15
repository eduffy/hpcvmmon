
#ifndef STRBUF_H
#define STRBUF_H

struct StringBuffer
{
   char *buffer;
   char *position;
   char *end;
};

struct StringBuffer *new_string_buffer();
void sbprintf(struct StringBuffer *sb, char *format, ...);
char *free_string_buffer(struct StringBuffer *sb, int free_contents);
int string_buffer_is_empty(struct StringBuffer *sb);

#endif /* STRBUF_H */
