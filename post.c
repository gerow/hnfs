#include "hnfs/post.h"

#include <string.h>
#include <assert.h>
#include <curl/curl.h>
#include <jsmn.h>

#include "aldn/aldn.h"

static char *
front_page_api_path = "http://localhost:4567/";

typedef struct {
  char *data;
  size_t size;
} curl_saver_t;

static size_t
curl_saver(void *buffer, size_t size, size_t nmemb, void *userp)
{
  size_t bytes = size * nmemb;
  curl_saver_t *saver = userp;

  fprintf(stderr, "curl_saver called\n");
  fprintf(stderr, "input size: %zu\n", size);
  fprintf(stderr, "input nmemb: %zu\n", nmemb);
  fprintf(stderr, "saver size: %zu\n", saver->size);

  size_t new_size = saver->size + bytes;


  /* alloc one more for the null character */
  saver->data = realloc(saver->data, new_size + 1);
  if (saver->data == NULL) {
    perror("failed to realloc in curl_saver");
    exit(EXIT_FAILURE);
  }

  memcpy(saver->data + saver->size, buffer, bytes);
  saver->size += bytes;

  /* make sure we're null terminated */
  saver->data[saver->size] = '\0';

  return bytes;
}

static int
fetch_url(char *url, curl_saver_t *saver)
{
  /* get a curl instance going to download the front page */
  CURL *curl;
  CURLcode res;

  memset(saver, 0, sizeof *saver);
  saver->data = NULL;
  saver->size = 0;

  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_saver);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, saver);
  res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  return res;
}

static int
load_file(char *filename, char **buffer)
{
  int ret = 0;

  FILE *f = fopen(filename, "r");
  if (f == NULL) {
    perror("failed to load file");
    exit(1);
  }
  ret = fseek(f, 0, SEEK_END);
  if (ret != 0) {
    perror("failed to seek to end of file");
    exit(1);
  }
  off_t file_size = ftell(f);
  assert(file_size == 12113);
  ret = fseek(f, 0, SEEK_SET);
  if (ret != 0) {
    perror("failed to seek to end of file");
    exit(1);
  }
  *buffer = malloc(file_size + 1);

  ret = fread(*buffer, 1, file_size, f);
  assert(ret == 12113);
  (*buffer)[file_size] = '\0';
  assert(strlen(*buffer) == 12113);

  if (ret != file_size) {
    perror("failed to read from file");
    exit(1);
  }

  ret = fclose(f);
  if (ret != 0) {
    perror("failed to close file");
    exit(1);
  }

  return 0;
}

int
hnfs_post_update(hnfs_post_collection_t *collection)
{
  int ret = 0;
  const static int num_tokens = 512;
  time_t cur_time = time(NULL);
  char *json = NULL;
  jsmn_parser p;
  /* now get jsmn to parse it into a list of tokens */
  /* 512 tokens should be enough for anybody... right? */
  jsmntok_t tokens[num_tokens];
  int posts_index;

  /* lock our collection mutex */
  pthread_mutex_lock(&collection->mutex);
  /* check to see if our data is old */
  if (cur_time - collection->update_time < HNFS_TIME_BETWEEN_UPDATES) {
    fprintf(stderr, "Data fresh. Skipping update.");
    goto cleanup_lock;
  }

  (void) fetch_url;
  curl_saver_t saver;
  fetch_url(front_page_api_path, &saver);
  json = saver.data;
  //load_file("/Users/gerow/proj/hnfs/test.json", &json);
  //assert(strlen(json) == 12113);

  jsmn_init(&p);
  //assert(strlen(json) == 12113);
  int jsmn_res = jsmn_parse(&p, json, tokens, 512);
  //assert(strlen(json) == 12113);
  if (jsmn_res != JSMN_SUCCESS) {
    fprintf(stderr, "jsmn had trouble parsing the data, dumping:\n");
    fprintf(stderr, "%s\n", json);
    exit(1);
  }

  aldn_context_t context;
  context.json = json;
  context.tokens = tokens;
  context.num_tokens = num_tokens;

  posts_index = aldn_key_value(&context, 0, "items");
  if (posts_index < 0) {
    fprintf(stderr, "Failed to find index of posts inside object\n");
    exit(1);
  }

  assert(tokens[posts_index].type == JSMN_ARRAY);
  /* only the first 30 entires are what we're looking for */
  for (int i = 0; i < 30; i++) {
    /* free this entry's content pointer if it is set */
    if (collection->posts[i].content) {
      free(collection->posts[i].content);
      collection->posts[i].content = NULL;
      collection->posts[i].content_update_time = 0;
    }
    int cur_post = aldn_ith_value(&context, posts_index, i);
    printf("object type is %d\n", tokens[cur_post].type);
    assert(tokens[cur_post].type == JSMN_OBJECT);
    int title_index = aldn_key_value(&context, cur_post, "title");
    assert(title_index >= 0);
    aldn_extract_string(&context,
                        title_index,
                        collection->posts[i].title,
                        HNFS_POST_STRING_SIZE);
    int url_index = aldn_key_value(&context, cur_post, "url");
    assert(url_index >= 0);
    aldn_extract_string(&context,
                        url_index,
                        collection->posts[i].url,
                        HNFS_POST_STRING_SIZE);
  }

  collection->update_time = cur_time;
/*cleanup_saver:*/
  free(json);
cleanup_lock:
  pthread_mutex_unlock(&collection->mutex);

  return ret;
}

/*
 * the calling thread should have a lock on the collection before
 * calling this function
 */
int hnfs_post_fetch_content(hnfs_post_t *post)
{
  if ((time(NULL) - post->content_update_time) < HNFS_TIME_BETWEEN_UPDATES) {
    fprintf(stderr, "Content fresh. Skipping update.\n");
    return 0;
  }

  /* if content is allocated free it */
  if (post->content) {
    free(post->content);
  }

  curl_saver_t saver = {
    .data = NULL,
    .size = 0
  };
  CURLcode res = fetch_url(post->url, &saver);
  if (res != CURLE_OK) {
    fprintf(stderr,
            "Failed to fetch url %s: %s\n",
            post->url,
            curl_easy_strerror(res));
    return res;
  }
  post->content = saver.data;
  post->content_update_time = time(NULL);

  return 0;
}
