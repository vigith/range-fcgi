/* fastcgi */
#include "fcgi_stdio.h"
#include "fcgiapp.h"
#include "fcgi_config.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/* range */
#include <libcrange.h>
#include <apr_pools.h>

// gcc -I /usr/local/include/ -I/usr/include/apr-1/ -I /home/vigith/range/libcrange/source/src/ -L /usr/local/lib/ -lcrange -lapr-1 -lpthread range-fcgi.c -lfcgi

/* how many threads do we need */
#define THREAD_COUNT 1
#define QUERY_STR_SIZE 1024*1024

static int get_range_query(FCGX_Request *, char *); /* get the range request */
static void invalid_request(FCGX_Request *, char *, char *); /* send invalid request!  */
static void * doit(void *);	/* thread worker */

/* TODO: Comment */
static void invalid_request(FCGX_Request *rq, char *code, char *msg) {
      FCGX_FPrintF(rq->out,
		   "Status: %s %s\r\n",
		   code, msg);

      return;
}

/* 
 * get_range_query
 *   Parses the GET|POST request and extracts the 'range query'
 *   string.
 * Args: 
 *   - FCGX_Request * (of the thread)
 *   - char * (location where the 'range query' is written)
 * Returns:
 *   - O (expand)
 *   - 1 (list)
 */
static int get_range_query(FCGX_Request *rq, char *query) {
  char *method = FCGX_GetParam("REQUEST_METHOD", rq->envp);
  char *script_name = FCGX_GetParam("SCRIPT_NAME", rq->envp);
  int list = -1;			/* list or expand? */

  /* accept only GET && POST */
  if(strcmp(method, "GET") != 0 && strcmp(method, "POST") != 0) {
    invalid_request(rq, "401", "Only GET || POST Request_Method Allowed");
  } else if (strcmp(method, "GET") == 0) {
    strcpy(query,FCGX_GetParam("QUERY_STRING", rq->envp));
  } else if (strcmp(method, "POST") == 0) {
    /* TODO: i might have to loop this in while and do a strcat + realloc if i need to increase
       string length at runtime */
    FCGX_GetStr(query, QUERY_STR_SIZE, rq->in);
  }

  /* we have two cases here
   *  - /range/list?(.*)
   *  - /range/expand?(.*)
   * SCRIPT_NAME == /range/list || /range/expand
   * QUERY_STRING == (.*) (in our case query has it)
   * strtok() for SCRIPT_NAME and decide which kind it is, 
   * for QUERY_STRING is passed as is.
   */
  if (strlen(query) == 0) {
    invalid_request(rq, "402", "No Query String Found");
    return 0;
  }

  /* list ? */
  list = strcmp(script_name, "/range/list") == 0 ? 1 : -1;
  
  /* if not list, but is expand */
  if (list != 1 && strcmp(script_name, "/range/expand") == 0) 
    list = 0;
  
  /* neither list nor expand */
  if (list == -1) {
    invalid_request(rq, "403", "Expects /range/list?... or /range/expand?...");
    return 0;
  }
  
  /*
  FCGX_FPrintF(rq->out,
	       "Content-type: text/plain\r\n"
	       "foo: bar\r\n"
	       "\r\n");
  FCGX_FPrintF(rq->out, "List (%d) Query: (%s) Script: (%s)\n", list, query, script_name);
  */

  return list;
}


/* thread function */
/* TODO: Comment */
static void *doit(void *a){
  int rc, i, thread_id = (pthread_t) a;
  pid_t pid = getpid();
  FCGX_Request request;
  apr_pool_t* pool;
  struct range_request* rr;
  const char **nodes;
  int debug = 1;
  struct libcrange *lr;
  char *config_file = NULL;
  char * r_query;		/* range query */
  int r_status = 0;

  apr_initialize();
  atexit(apr_terminate);
  apr_pool_create(&pool, NULL);

  config_file = LIBCRANGE_CONF;

  lr = libcrange_new(pool, config_file);

  /* malloc for query */
  r_query = (char *)malloc(QUERY_STR_SIZE);

  FCGX_InitRequest(&request, 0, 0);

  for (;;)
    {
      static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;

      apr_pool_t* sub_pool;

      apr_pool_create(&sub_pool, pool);

      /* zero it out */
      bzero(r_query, QUERY_STR_SIZE);
      r_status = 0;
	

      /* Some platforms require accept() serialization, some don't.. */
      pthread_mutex_lock(&accept_mutex);
      rc = FCGX_Accept_r(&request);
      pthread_mutex_unlock(&accept_mutex);

      if (rc < 0)
	break;

      
      r_status = get_range_query(&request, r_query);
      rr = range_expand(lr, sub_pool, r_query);

      FCGX_FPrintF(request.out, "Content-type: text/plain\r\n");
      
      /* set headers */
      if (range_request_has_warnings(rr)) {
      	const char *warnings = range_request_warnings(rr);
      	FCGX_FPrintF(request.out,
      		     "RangeException: %s\r\n",
      		     warnings);
      }

      /* End Delimiter */
      FCGX_FPrintF(request.out, "\r\n");

      /* r_status == 1, then wants list */
      if (r_status == 1) {
      	const char **nodes = range_request_nodes(rr);
      	while(*nodes) {
      	  FCGX_FPrintF(request.out, "%s\n", *nodes++);
      	}
      } else if (r_status == 0) {
      	FCGX_FPrintF(request.out, "%s\n", range_request_compressed(rr));
      }
      
      apr_pool_destroy(sub_pool);

      FCGX_Finish_r(&request);

    } /* for (;;) */

  /* free, what I hogged :-) */
  free(r_query);

  apr_pool_destroy(pool);

  return NULL;
}

/* and thus all started */
int main(void) {
  pthread_t i;
  pthread_t id[THREAD_COUNT];

  FCGX_Init();

  for (i = 1; i < THREAD_COUNT; i++)
    pthread_create(&id[i], NULL, doit, (void*)i);

  doit(0);

  return 0;
}
