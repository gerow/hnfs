#include "aldn/aldn.h"

#include <assert.h>

int
aldn_key_value(const aldn_context_t *context,
               int object_index,
               char *name)
{
  jsmntok_t *tokens = context->tokens;
  char *json = context->json;
  size_t num_tokens = context->num_tokens;

  assert(tokens[object_index].type == JSMN_OBJECT);
  /* so the next indexed object should be the first key
   * in the object, unless the objectis empty, but don't
   * worry we handle that! */
   int i = object_index + 1;
   size_t object_end = tokens[object_index].end;
   size_t name_size = strlen(name);

   /* so while we aren't past the end of all tokens and  we
    * haven't gone past the end of this object */
   while (i < num_tokens && tokens[i].start < object_end) {
    /* this should always be a key. If it isn't then there's a logic error
     * in this library */
    assert(tokens[i].type == JSMN_STRING);
    int key_length = tokens[i].end - tokens[i].start;
    if (name_size == key_length &&
        strncmp(json + tokens[i].start, name, name_size) == 0) {
      return i + 1;
    }

    /* if that didn't work we need to skip over the value. */
    i++;
    size_t value_end = tokens[i].end;
    do {
      i++;
      if (i >= num_tokens) {
        return -1;
      }
      /* we've walked off the end of the object! */
      if (tokens[i].start > object_end) {
        return -1;
      }
    } while (tokens[i].start < value_end);
  }
}

int
aldn_ith_value(const aldn_context_t *context,
               int object_index,
               int array_index)
{
  jsmntok_t *tokens = context->tokens;
  char *json = context->json;
  size_t num_tokens = context->num_tokens;

  assert(tokens[object_index].type = JSMN_ARRAY);
  /* if it's off the end of the array, return -1 */
  if (array_index >= tokens[object_index].size) {
    return -1;
  }
  int i = object_index + 1;
  int cur_array_index = 0;
  while (cur_array_index != array_index) {
    int end = tokens[i].end;
    do {
      i++;
      if (i >= num_tokens) {
        return -1;
      }
    } while (tokens[i].start < end);
    cur_array_index++;
  }

  return i;
}

int
aldn_extract_string(const aldn_context_t *context,
                    int string_index,
                    char *buffer,
                    size_t buffer_size)
{
  jsmntok_t *tokens = context->tokens;
  int string_len = tokens[string_index].end - tokens[string_index].start;
  /* the +1 is for the null character */
  if (string_len + 1 > buffer_size) {
    /* it just won't fit */
    return -1;
  }
  strncpy(buffer, context->json + tokens[string_index].start, string_len);
  /* throw a null character on the end */
  buffer[string_len] = '\0';
}
