#ifndef HNFS_POST_H_
#define HNFS_POST_H_

#include <stdlib.h>
#include <pthread.h>

#include "hnfs/decls.h"

typedef struct {
  char title[255];
  char url[255];
  char *user;
  char *id;
} hnfs_post_t;

typedef struct {
  hnfs_post_t posts[HNFS_NUM_POSTS];
  pthread_mutex_t mutex;
} hnfs_post_collection_t;

int
hnfs_post_update(hnfs_post_collection_t *collection);

#endif /* HNFS_POST_H_ */
