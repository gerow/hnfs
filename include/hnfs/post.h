#ifndef HNFS_POST_H_
#define HNFS_POST_H_

#include <stdlib.h>

/* forward declare post */
struct hnfs_post;

struct hnfs_post {
  char *title;
  char *url;
  char *user;
  char *id;
  struct hnfs_post *next;
};

size_t
hnfs_posts_libcurl_writer(void *buffer, size_t size, size_t nmemb, void *userp);

#endif /* HNFS_POST_H_ */
