/**
 * webserver.c -- A webserver written in C
 *
 * Test with curl (if you don't have it, install it):
 *
 *    curl -D - http://localhost:3490/
 *    curl -D - http://localhost:3490/d20
 *    curl -D - http://localhost:3490/date
 *
 * You can also test the above URLs in your browser! They should work!
 *
 * Posting Data:
 *
 *    curl -D - -X POST -H 'Content-Type: text/plain' -d 'Hello, sample data!' http://localhost:3490/save
 *
 * (Posting data is harder to test from a browser.)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/file.h>
#include <fcntl.h>
#include "net.h"
#include "file.h"
#include "mime.h"
#include "cache.h"

#define PORT "3490"  // the port users will be connecting to

#define SERVER_FILES "./serverfiles"
#define SERVER_ROOT "./serverroot"

/**
 * Send an HTTP response
 *
 * header:       "HTTP/1.1 404 NOT FOUND" or "HTTP/1.1 200 OK", etc.
 * content_type: "text/plain", etc.
 * body:         the data to send.
 *
 * Return the value from the send() function.
 */
int send_response(int fd, char *header, char *content_type, void *body, int content_length)
{
    const int max_response_size = 655360;
    char response[max_response_size];
    int response_length;
    // if (content_length > max_response_size) {
    //   printf("Response is too big\n");
    //   return -1;
    // }
    response_length = sprintf(response, "%s\nConnection: close\nContent-Length: %d\nContent-Type: %s\n\n", header, content_length,content_type);
    printf("%s\n", response);
    memcpy(response + response_length, body, content_length);
    ///////////////////
    // IMPLEMENT ME! //
    ///////////////////

    // Send it all!
    int rv = send(fd, response, response_length + content_length, 0);

    if (rv < 0) {
        perror("send");
    }

    return rv;
}


/**
 * Send a /d20 endpoint response
 */
void get_d20(int fd)
{
    // Generate a random number between 1 and 20 inclusive
    ///////////////////
    // IMPLEMENT ME! //
    ///////////////////
    time_t t;
    srand ((unsigned) time(&t));
    int res = (rand() % 20) + 1;
    char response[4];
    if (res >= 10) {
      sprintf(response, "%d\n", res);
    } else {
      sprintf(response, "0%d\n", res);
    }

    // Use send_response() to send it back as text/plain data

    ///////////////////
    // IMPLEMENT ME! //
    ///////////////////
    send_response(fd, "HTTP/1.1 200 OK", "text/plain", response, strlen(response));
}

/**
 * Send a 404 response
 */
void resp_404(int fd)
{
    char filepath[4096];
    struct file_data *filedata;
    char *mime_type;

    // Fetch the 404.html file
    snprintf(filepath, sizeof filepath, "%s/404.html", SERVER_FILES);
    filedata = file_load(filepath);

    if (filedata == NULL) {
        // TODO: make this non-fatal
        fprintf(stderr, "cannot find system 404 file\n");
        exit(3);
    }

    mime_type = mime_type_get(filepath);

    send_response(fd, "HTTP/1.1 404 NOT FOUND", mime_type, filedata->data, filedata->size);

    file_free(filedata);
}

/**
 * Read and return a file from disk or cache
 */
void get_file(int fd, struct cache *cache, char *request_path)
{
    ///////////////////
    // IMPLEMENT ME! //
    ///////////////////
    struct cache_entry *entry = cache_get(cache, request_path);
    if(entry == NULL){
      char *mime_type = mime_type_get(request_path);
      struct file_data * f = file_load(request_path);
      if(f == NULL) resp_404(fd);
      printf("Serving from file\n");
      cache_put(cache, request_path, mime_type, f->data, f->size);
      send_response(fd, "HTTP/1.1 200 ok", mime_type, f->data, f->size);
    } else {
      printf("Serving from cache\n");
      send_response(fd, "HTTP/1.1 200 ok", entry->content_type, entry->content, entry->content_length);
    }
}

/**
 * Search for the end of the HTTP header
 *
 * "Newlines" in HTTP can be \r\n (carriage return followed by newline) or \n
 * (newline) or \r (carriage return).
 */
char *find_start_of_body(char *header)
{
    ///////////////////
    // IMPLEMENT ME! // (Stretch)
    ///////////////////
}

/**
 * Handle HTTP request and send response
 */
void handle_http_request(int fd, struct cache *cache)
{
    const int request_buffer_size = 65536; // 64K
    char request[request_buffer_size];

    // Read request
    int bytes_recvd = recv(fd, request, request_buffer_size - 1, 0);
    if (bytes_recvd < 0) {
        perror("recv");
        return;
    }
    printf("%s\n", request);


    ///////////////////
    // IMPLEMENT ME! //
    ///////////////////
    int i = 0;
    int c = 0;
    char request_type[10];
    char request_path[50];
    char connection_type[50];
    // Read the three components of the first request line
    for(c = 0;request[i] != ' ';c++){
      request_type[c] = request[i];
      i++;
    }
    request_type[c] = '\0';
    i++;

    for(c = 0;request[i] != ' ';c++){
      request_path[c] = request[i];
      i++;
    }
    request_path[c] = '\0';
    i++;

    for(c = 0;request[i] != '\n';c++){
      connection_type[c] = request[i];
      i++;
    }
    connection_type[c] = '\0';

    // If GET, handle the get endpoints
    if(strcmp("GET", request_type) == 0){
      if(strcmp("/", request_path) == 0){
        char filepath[4096];
        snprintf(filepath, sizeof filepath, "%s/index.html", SERVER_ROOT);
        get_file(fd, cache, filepath);
      }
      else if(strcmp("/d20", request_path) == 0) get_d20(fd);
      else if(strcmp("/cat", request_path) == 0){
        char filepath[4096];
        snprintf(filepath, sizeof filepath, "%s/cat.jpg", SERVER_ROOT);
        get_file(fd, cache, filepath);
      }
      else resp_404(fd);
    }
    //    Check if it's /d20 and handle that special case
    //    Otherwise serve the requested file by calling get_file()


    // (Stretch) If POST, handle the post request
}

/**
 * Main
 */
int main(void)
{
    int newfd;  // listen on sock_fd, new connection on newfd
    struct sockaddr_storage their_addr; // connector's address information
    char s[INET6_ADDRSTRLEN];

    struct cache *cache = cache_create(10, 0);

    // Get a listening socket
    int listenfd = get_listener_socket(PORT);

    if (listenfd < 0) {
        fprintf(stderr, "webserver: fatal error getting listening socket\n");
        exit(1);
    }

    printf("webserver: waiting for connections on port %s...\n", PORT);

    // This is the main loop that accepts incoming connections and
    // forks a handler process to take care of it. The main parent
    // process then goes back to waiting for new connections.

    while(1) {
        socklen_t sin_size = sizeof their_addr;

        // Parent process will block on the accept() call until someone
        // makes a new connection:
        newfd = accept(listenfd, (struct sockaddr *)&their_addr, &sin_size);
        if (newfd == -1) {
            perror("accept");
            continue;
        }

        // Print out a message that we got the connection
        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);

        // newfd is a new socket descriptor for the new connection.
        // listenfd is still listening for new connections.

        handle_http_request(newfd, cache);

        close(newfd);
    }

    // Unreachable code

    return 0;
}
