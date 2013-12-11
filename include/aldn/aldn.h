#ifndef ALDN_ALDN_H_
#define ALDN_ALDN_H_

#include <jsmn.h>
#include <stdlib.h>

typedef struct {
  char *json;
  jsmntok_t *tokens;
  size_t num_tokens;
} aldn_context_t;

typedef struct {
  aldn_context_t context;
  /* the index of the array in the tokens list */
  int array_index;
  /* the index we're going to read next */
  int current_index[];
} aldn_array_iterator_t;

/*
 * Take in an array of tokens and the size of the token array and return
 * the index of the value for the key that matches name. object_index is
 * the index of the object we're searching. If object_index is not an object
 * type our assertion fails...
 */
int
aldn_key_value(const aldn_context_t *context,
               int object_index,
               char *name);

/*
 * Take the json string, an array of tokens, and the number of tokens,
 * an index of the object we want to find the ith value of, and the index
 * we want to find in it. Basically, we're looking for object_index to point
 * to an array object inside of the tokens array
 */
int
aldn_ith_value(const aldn_context_t *context,
               int object_index,
               int array_index);

int
aldn_extract_string(const aldn_context_t *context,
                    int string_index,
                    char *buffer,
                    size_t buffer_size);

/*
 * create a new array iterator object
 */
void
aldn_array_iterator_create(aldn_array_iterator_t *iterator,
                           aldn_context_t *context,
                           int array_index);

/* 
 * get the index into the tokens array of the current value
 * the iterator is pionting to or -1 if we're at the end of
 * the array.
 */
int
aldn_array_iterator_get(aldn_array_iterator_t *iterator);

/*
 * moves us forward to the next value in the array and returns
 * its index in the tokens array. Returns -1 if we've hit the
 * end of the array
 */
int
aldn_array_iterator_next(aldn_array_iterator_t *iterator);

#endif /* ALDN_ALDN_H_ */