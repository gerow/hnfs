#include "hnfs/post.h"

#include <string.h>
#include <assert.h>
#include <curl/curl.h>
#include <jsmn.h>

#include "aldn/aldn.h"

static const char
front_page_api_path[] = "http://api.ihackernews.com/page";

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
  fprintf(stderr, "input size: %u\n", size);
  fprintf(stderr, "input nmemb: %u\n", nmemb);
  fprintf(stderr, "saver size: %u\n", saver->size);

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
  curl_easy_perform(curl);
  curl_easy_cleanup(curl);
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
  const static int num_tokens = 512;
  /* lock our collection mutex */
  pthread_mutex_lock(&collection->mutex);
  char *json = NULL;

  //curl_saver_t saver;
  //fetch_url(front_page_api_path, &saver);
  //json = saver.data;
  load_file("test.json", &json);
  assert(strlen(json) == 12113);

  /* now get jsmn to parse it into a list of tokens */
  jsmn_parser p;
  /* 512 tokens should be enough for anybody... right? */
  jsmntok_t tokens[num_tokens];

  jsmn_init(&p);
  assert(strlen(json) == 12113);
  int jsmn_res = jsmn_parse(&p, json, tokens, 512);
  assert(strlen(json) == 12113);
  if (jsmn_res != JSMN_SUCCESS) {
    fprintf(stderr, "jsmn had trouble parsing the data, dumping:\n");
    fprintf(stderr, "%s\n", json);
    exit(1);
  }

  aldn_context_t context;
  context.json = json;
  context.tokens = tokens;
  context.num_tokens = num_tokens;

  int posts_index = aldn_key_value(&context, 0, "items");
  if (posts_index < 0) {
    fprintf(stderr, "Failed to find index of posts inside object\n");
    exit(1);
  }

  assert(tokens[posts_index].type == JSMN_ARRAY);
  assert(tokens[posts_index].size == 31);
  /* only the first 30 entires are what we're looking for */
  for (int i = 0; i < 30; i++) {
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
cleanup_saver:
  free(json);
cleanup_lock:
  pthread_mutex_unlock(&collection->mutex);
}
