#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "proxyserver.h"
#include "safequeue.h"


/*
 * Constants
 */
#define RESPONSE_BUFSIZE 10000
#define REQUEST_BUFSIZE 10000
/*
 * Global configuration variables.
 * Their values are set up in main() using the
 * command line arguments (already implemented for you).
 */
int num_listener;
int *listener_ports;
int num_workers;
char *fileserver_ipaddr;
int fileserver_port;
int max_queue_size;
PriorityQueue *pq;

/**
 * sync vars
*/
pthread_mutex_t mutex;
pthread_cond_t empty;
pthread_cond_t add;


void send_error_response(int client_fd, status_code_t err_code, char *err_msg) {
    http_start_response(client_fd, err_code);
    http_send_header(client_fd, "Content-Type", "text/html");
    http_end_headers(client_fd);
    char *buf = malloc(strlen(err_msg) + 2);
    sprintf(buf, "%s\n", err_msg);
    http_send_string(client_fd, buf);
    return;
}

void send_GetJob_response(int client_fd, char *path) {
    char strLength[3];
    sprintf(strLength, "%d", (int)strlen(path));
    http_start_response(client_fd, 200);
    http_send_header(client_fd, "Content-type", "text/plain");
    // http_send_header(client_fd, "Content-length", strlen(path));
    http_send_header(client_fd, "Content-length", strLength);
    // http_send_header(client_fd, "Server", "httpserver/1.0");
    http_end_headers(client_fd);
    http_send_string(client_fd, path);
    return;
}

/*
 * forward the client request to the fileserver and
 * forward the fileserver response to the client
 * called by server when server accepted a new request
 */
void serve_request(int client_fd, char* buffer) {

    // create a fileserver socket
    int fileserver_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fileserver_fd == -1) {
        fprintf(stderr, "Failed to create a new socket: error %d: %s\n", errno, strerror(errno));
        exit(errno);
    }

    // create the full fileserver address
    struct sockaddr_in fileserver_address;
    fileserver_address.sin_addr.s_addr = inet_addr(fileserver_ipaddr);
    fileserver_address.sin_family = AF_INET;
    fileserver_address.sin_port = htons(fileserver_port);

    // connect to the fileserver
    int connection_status = connect(fileserver_fd, (struct sockaddr *)&fileserver_address,
                                    sizeof(fileserver_address));
    if (connection_status < 0) {
        // failed to connect to the fileserver
        printf("Failed to connect to the file server\n");
        send_error_response(client_fd, BAD_GATEWAY, "Bad Gateway");
        return;
    }

    // successfully connected to the file server
    // char *buffer = (char *)malloc(RESPONSE_BUFSIZE * sizeof(char));

    // forward the client request to the fileserver
    // int bytes_read = read(client_fd, buffer, RESPONSE_BUFSIZE);
    
    int ret = http_send_data(fileserver_fd, buffer, strlen(buffer));
    // int ret = http_send_data(fileserver_fd, buffer, bytes_read);
    if (ret < 0) {
        printf("Failed to send request to the file server\n");
        send_error_response(client_fd, BAD_GATEWAY, "Bad Gateway");

    } else {
        // forward the fileserver response to the client
        while (1) {
            int bytes_read = recv(fileserver_fd, buffer, RESPONSE_BUFSIZE - 1, 0);
            if (bytes_read <= 0) // fileserver_fd has been closed, break
                break;
            ret = http_send_data(client_fd, buffer, bytes_read);
            if (ret < 0) { // write failed, client_fd has been closed
                break;
            }
        }
    }

    // close the connection to the fileserver
    shutdown(fileserver_fd, SHUT_WR);
    close(fileserver_fd);
    // close the connection to the client
    shutdown(client_fd, SHUT_WR);
    close(client_fd);

    // Free resources and exit
    // free(buffer);
}

request_info parse_request(const char *request) {
    request_info req_info;
    req_info.delay = 0; // Default delay value
    req_info.client_fd = 0; // Placeholder, should be set from the actual client file descriptor
    req_info.path = NULL;

    // Duplicate the request to avoid modifying the original string
    char *request_copy = strdup(request);
    char *line, *saveptr;

    // Parse the request line for the path
    line = strtok_r(request_copy, "\n", &saveptr);
    if (line != NULL) {
        strtok(line, " ");
        char *path = strtok(NULL, " ");
        if (path != NULL) {
            req_info.path = strdup(path); // Duplicate the path
        }
        // Skipping parsing of the HTTP version
    }

    // Parse headers for the Delay
    while ((line = strtok_r(NULL, "\n", &saveptr)) != NULL) {
        char *key = strtok(line, ": ");
        char *value = strtok(NULL, "");
        if (key != NULL && value != NULL && strcmp(key, "Delay") == 0) {
            req_info.delay = atoi(value); // Convert the delay value from string to integer
        }
    }

    free(request_copy); // Clean up the duplicated string
    return req_info;
}

int get_priority_from_path(const char* path) {
    if (path == NULL) {
        return 0; // Default priority if path is NULL
    }

    char* path_copy = strdup(path);
    char* token = strtok(path_copy, "/");
    int priority = 0;

    if (token != NULL) {
        priority = atoi(token);
    }

    free(path_copy);
    return priority;
}

int server_fd;
/*
 * opens a TCP stream socket on all interfaces with port number PORTNO. Saves
 * the fd number of the server socket in *socket_number. For each accepted
 * connection, calls request_handler with the accepted fd number.
 */
void serve_forever(int *server_fd, int proxy_port) {


    // create a socket to listen
    *server_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (*server_fd == -1) {
        perror("Failed to create a new socket");
        exit(errno);
    }

    // manipulate options for the socket
    int socket_option = 1;
    if (setsockopt(*server_fd, SOL_SOCKET, SO_REUSEADDR, &socket_option,
                   sizeof(socket_option)) == -1) {
        perror("Failed to set socket options");
        exit(errno);
    }

    // now use the port passed
    // int proxy_port = listener_ports[0];
    // create the full address of this proxyserver
    struct sockaddr_in proxy_address;
    memset(&proxy_address, 0, sizeof(proxy_address));
    proxy_address.sin_family = AF_INET;
    proxy_address.sin_addr.s_addr = INADDR_ANY;
    proxy_address.sin_port = htons(proxy_port); // listening port

    // bind the socket to the address and port number specified in
    if (bind(*server_fd, (struct sockaddr *)&proxy_address,
             sizeof(proxy_address)) == -1) {
        perror("Failed to bind on socket");
        exit(errno);
    }

    // starts waiting for the client to request a connection
    if (listen(*server_fd, 1024) == -1) {
        perror("Failed to listen on socket");
        exit(errno);
    }

    printf("Listening on port %d...\n", proxy_port);

    struct sockaddr_in client_address;
    size_t client_address_length = sizeof(client_address);
    int client_fd;



    // FILE *log_file = fopen("server_listening_thread_log.txt", "w");
    // listening loop
    while (1) {
        client_fd = accept(*server_fd,
                           (struct sockaddr *)&client_address,
                           (socklen_t *)&client_address_length);
        if (client_fd < 0) {
            // fprintf(log_file, "Error accepting socket\n");
            perror("Error accepting socket");
            continue;
        }
        printf("Accepted connection from %s on port %d\n",
               inet_ntoa(client_address.sin_addr),
               client_address.sin_port);

        // should now read and parse
        char buffer[REQUEST_BUFSIZE];
        ssize_t bytes_read = recv(client_fd, buffer, REQUEST_BUFSIZE - 1, 0);
        if (bytes_read < 0) {
            // Handle read error
            perror("Error reading from socket");
            close(client_fd);
            continue;
        }
        buffer[bytes_read] = '\0';
        // fprintf(log_file, "Received request: \n%s", buffer);
        request_info req_info = parse_request(buffer);
        request_info *getjob = (request_info *)malloc(sizeof(request_info));
        pthread_mutex_lock(&mutex);
        if (strcmp(req_info.path, "/GetJob") == 0) {
            if (get_work_nonblocking(pq, getjob) == -1) {
                send_error_response(client_fd, QUEUE_EMPTY, http_get_response_message(QUEUE_EMPTY));
            }
            else {
                send_GetJob_response(client_fd, getjob->path);
            }
            pthread_mutex_unlock(&mutex);
            shutdown(client_fd, SHUT_WR);
            close(client_fd);
            free(getjob);
            continue;            
        } 
        free(getjob);
        pthread_mutex_unlock(&mutex);
        req_info.client_fd = client_fd;
        char *buffer_copy = strdup(buffer);
        req_info.buffer = buffer_copy;

        int priority = get_priority_from_path(req_info.path);

        /**
         * should work as produser
         * call add_work and cond_wait()
        */
        pthread_mutex_lock(&mutex);
        if(pq->size == max_queue_size){
            // queue is full
            send_error_response(client_fd, QUEUE_FULL, http_get_response_message(QUEUE_FULL));
            shutdown(client_fd, SHUT_WR);
            close(client_fd);
            pthread_mutex_unlock(&mutex);
            continue;
            // pthread_cond_wait(&empty, &mutex);
        }
        /**
         * todo:
         * should add_work()
        */
        add_work(pq, req_info, priority);
        pthread_cond_signal(&add);
        pthread_mutex_unlock(&mutex);


    }

    shutdown(*server_fd, SHUT_RDWR);
    close(*server_fd);
}

// listener thread
void *listener_thread(void *arg) {
    int port = *(int*)arg;
    int server_fd;
    serve_forever(&server_fd, port);
    pthread_exit(NULL);
}

void *worker_thread(void *arg) {
    /**
     * should be similar to consumer
     * call getwork()
     * if -1 , empty, cond_wait
     * else , delay and then call serve_request()
    */

    request_info *work = (request_info*)malloc(sizeof(request_info));
    // FILE *log_file = fopen("server_worker_thread_log.txt", "w");
    // work forever
    while(1) {
        pthread_mutex_lock(&mutex);
        while ( pq->size == 0) {
            pthread_cond_wait(&add, &mutex);
        }
        // get work from pq
        get_work(pq, work);
        // pthread_cond_signal(&empty);
        pthread_mutex_unlock(&mutex);
        // delay, canbe zero
        if(work->delay != 0)
            sleep(work->delay);
        
        serve_request(work->client_fd, work->buffer);
        free(work->path);
        free(work->buffer);
        
       
    }
    free(work);
    pthread_exit(NULL);
}

/*
 * Default settings for in the global configuration variables
 */
void default_settings() {
    num_listener = 1;
    listener_ports = (int *)malloc(num_listener * sizeof(int));
    listener_ports[0] = 8000;

    num_workers = 1;

    fileserver_ipaddr = "127.0.0.1";
    fileserver_port = 3333;

    max_queue_size = 100;
}

void print_settings() {
    printf("\t---- Setting ----\n");
    printf("\t%d listeners [", num_listener);
    for (int i = 0; i < num_listener; i++)
        printf(" %d", listener_ports[i]);
    printf(" ]\n");
    printf("\t%d workers\n", num_workers);
    printf("\tfileserver ipaddr %s port %d\n", fileserver_ipaddr, fileserver_port);
    printf("\tmax queue size  %d\n", max_queue_size);
    printf("\t  ----\t----\t\n");
}

// siganl to close server
void signal_callback_handler(int signum) {
    printf("Caught signal %d: %s\n", signum, strsignal(signum));
    for (int i = 0; i < num_listener; i++) {
        if (close(server_fd) < 0) perror("Failed to close server_fd (ignoring)\n");
    }
    free(listener_ports);
    exit(0);
}

char *USAGE =
    "Usage: ./proxyserver [-l 1 8000] [-n 1] [-i 127.0.0.1 -p 3333] [-q 100]\n";

void exit_with_usage() {
    fprintf(stderr, "%s", USAGE);
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
    signal(SIGINT, signal_callback_handler);

    /* Default settings */
    default_settings();

    int i;
    for (i = 1; i < argc; i++) {
        if (strcmp("-l", argv[i]) == 0) {
            num_listener = atoi(argv[++i]);
            free(listener_ports);
            listener_ports = (int *)malloc(num_listener * sizeof(int));
            for (int j = 0; j < num_listener; j++) {
                listener_ports[j] = atoi(argv[++i]);
            }
        } else if (strcmp("-w", argv[i]) == 0) {
            num_workers = atoi(argv[++i]);
        } else if (strcmp("-q", argv[i]) == 0) {
            max_queue_size = atoi(argv[++i]);
        } else if (strcmp("-i", argv[i]) == 0) {
            fileserver_ipaddr = argv[++i];
        } else if (strcmp("-p", argv[i]) == 0) {
            fileserver_port = atoi(argv[++i]);
        } else {
            fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
            exit_with_usage();
        }
    }

    // initialize all locks
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&empty, NULL);
    pthread_cond_init(&add, NULL);

    print_settings();
    // create pq
    pq = create_queue(max_queue_size);

    // FILE *log_file = fopen("server_log.txt", "w");

    // serve_forever(&server_fd);
    pthread_t *listener_threads = malloc(num_listener * sizeof(pthread_t));
    for (int i = 0; i < num_listener; i++) {
        pthread_create(&listener_threads[i], NULL, listener_thread, (void*)&listener_ports[i]);
        // fprintf(log_file, "Hello from listening thread\n");
    }

    pthread_t *worker_threads = malloc(num_workers * sizeof(pthread_t));
    for (int i = 0; i < num_workers; i++) {
        pthread_create(&worker_threads[i], NULL, worker_thread, NULL);
        // fprintf(log_file, "Hello from worker thread\n");
    }

    // wait all listener_thread exit
    for (int i = 0; i < num_listener; i++) {
        pthread_join(listener_threads[i], NULL);
        free(listener_threads);
    }

    // wait all worker_thread exit
    for (int i = 0; i < num_workers; i++) {
        pthread_join(worker_threads[i], NULL);
        free(worker_threads);
    }

    // free pq
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < max_queue_size; i++){
        free(pq->items);
    }
    free(pq);
    pthread_mutex_unlock(&mutex);

    // release all the locks
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&empty);
    pthread_cond_destroy(&add);
    return EXIT_SUCCESS;
}