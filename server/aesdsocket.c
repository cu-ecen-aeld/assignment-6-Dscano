/*
 * =====================================================================================
 *
 *       Filename:  aesdsocket.c *
 *    Description:  Assignment 6 part 1
 *
 *        Version:  1.0
 *        Created:  2024 
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ian Chen 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include "aesdsocket.h"
#include "file_manipulate.h" 
#include <stdbool.h>
#include <stdint.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>

#define BSIZE (BUFSIZ * 3)
#define FILE_NAME "/var/tmp/aesdsocketdata"
#define LISTEN_MAX 5
#define TIME_PERIOD 10

/* Global variable */
int signal_sign = 0;

/* function prototype */
int signal_setup(int, ...);

/* A structure contains and file fd and a mutex object. */
static struct {
    pthread_mutex_t *mutex;
    int fd;
    timer_t timer_id;
} timer_data = {.mutex = NULL, .fd = 0, .timer_id = 0};

typedef struct thread {
    struct sockaddr_in conn_addr;
    ssize_t (*recv)(int sock, void *buf, size_t length, int flags);
    ssize_t (*send)(int sock, const void *buf, size_t length, int flags);
    ssize_t (*write)(int fd, const void *buf, size_t count);
    ssize_t (*read)(int fd, void *buf, size_t count);
    pthread_mutex_t *mutex;
    pthread_t tid;
    struct thread *next;
    int conn_sock;
    int fd;
} tcp_thread;

void tcp_shutdown(int sockfd, int how) {

    if (shutdown(sockfd, how)) {
        ERROR_HANDLER(shutdown);
    }
    return;
}

void tcp_close (int sockfd) {

    if (sockfd > 0) {
        tcp_shutdown(sockfd, SHUT_RDWR);
    }
    if (close(sockfd)) {
        ERROR_HANDLER(close);
    }
    return;
}

void tcp_set_nonblock(int sockfd, int invert) {

    int rc, flag;

    flag = fcntl(sockfd, F_GETFL);
    if (!invert) {
        rc = fcntl(sockfd, F_SETFL, flag | O_NONBLOCK);
    } else {
        rc = fcntl(sockfd, F_SETFL, flag ^ O_NONBLOCK);
    }

    return;
}

int tcp_select(int sockfd) {

    int rc;
    fd_set readfds;
    struct timeval time = {0, 0};

    time.tv_sec = 60;

    FD_ZERO(&readfds);
    FD_CLR(sockfd, &readfds);
    FD_SET(sockfd, &readfds);

    rc = select(sockfd + 1, &readfds, NULL, NULL, &time);

    if (rc > 0 && FD_ISSET(sockfd, &readfds)) {
        FD_CLR(sockfd, &readfds);
        return sockfd;
    } else if (rc == 0) {
        return 0;
    } else {
        return -1;
    }
}

int tcp_getopt(int argc, char *argv[]) {

    int ch, rc;

    while ((ch = getopt(argc, argv, "d")) != -1) {
        switch(ch) {
            case 'd':
                printf("Convert %s into daemon.\n", __FILE__);
                rc = daemon(1, 0);
                USER_LOGGING("Running as daemon: %d\n", rc);
                return true;
        }
    }
    return false;
} 

static void fill_sockaddr(struct sockaddr_in *addr) {

    memset(addr, 0, sizeof(struct sockaddr_in));

    addr->sin_addr.s_addr = htonl(INADDR_ANY);
    addr->sin_port = htons(9000);
    addr->sin_family = AF_INET;

    return;
}

static int tcp_socket(void) {

    int sockfd, rc;
    struct sockaddr_in tcp_addr = {0};
    int on = 1;

    fill_sockaddr(&tcp_addr);

    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd == -1) {
        ERROR_HANDLER(socket);
    }

    // Set socket option to enable reuse the local address
    rc = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *) &on, (socklen_t) sizeof(on));

    if (rc == -1) {
        ERROR_HANDLER(setsockopt);
    } else {
        fprintf(stdout, "[%s] set SO_REUSEADDR success.\n", __func__); 
    }

    rc = bind(sockfd, (struct sockaddr *) &tcp_addr, sizeof(struct sockaddr));
    if (rc == -1) {
        ERROR_HANDLER(bind);
        fprintf(stderr, "[%s] exit caused by call to bind.\n", __func__);
        exit(EXIT_FAILURE);
    }

    rc = listen(sockfd, LISTEN_MAX);
    if (rc == -1) {
        ERROR_HANDLER(listen);
    }

    return sockfd;
}

static int tcp_getfd(void) {

    int fd = 0;

    file_delete(FILE_NAME);
    if ((fd = open(FILE_NAME, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO)) == -1) {
        perror("open ");
    }
    return fd;
}    

static void timer_writer(int sig) {

    struct tm *time_info = NULL;
    char time_stamp[BUFSIZ] = {0};
    //tcp_thread *thread = (tcp_thread *) args;
    ssize_t rc = 0;
    struct timespec now = {0};
    time_t diff = 0;

    fprintf(stdout, "[%s] entered.\n", __func__);

    clock_gettime(CLOCK_REALTIME, &now);

    time_info = gmtime(&now.tv_sec);
    strftime(time_stamp, BUFSIZ * sizeof(char), "timestamp:%a, %d %b %Y %T %z\n", time_info);

    if (pthread_mutex_lock(timer_data.mutex)) {
        perror("pthread_mutex_lock ");
    }

    if (lseek(timer_data.fd, 0, SEEK_END) == -1) {
        perror("lseek "); 
    }

    rc = write(timer_data.fd, time_stamp, strlen(time_stamp));
    syslog(LOG_INFO, "[%s] write %ld byted to aesdsocketdata.\n", __func__, rc);

    if (pthread_mutex_unlock(timer_data.mutex)) {
        perror("pthread_mutex_unlokc ");
    }

    return;
}

static void timer_setup(int sig, void (*func)(int)) {

    struct sigaction sig_info = {0};
    struct sigevent event = {0};
    struct itimerspec time_val = {0};

    sigemptyset(&sig_info.sa_mask);
    sigaddset(&sig_info.sa_mask, sig);

    sig_info.sa_handler = func;

    if (sigaction(sig, &sig_info, NULL)) {
        perror("sigaction ");
    }

    event.sigev_signo = sig;
    event.sigev_notify = SIGEV_SIGNAL;

    if (timer_create(CLOCK_REALTIME, &event, &timer_data.timer_id)) {
        perror("timer_create ");
    }

    time_val.it_value.tv_sec = TIME_PERIOD;
    time_val.it_interval.tv_sec = TIME_PERIOD;

    if (timer_settime(timer_data.timer_id, 0, &time_val, NULL)) {
        perror("timer_settime ");
    }

    syslog(LOG_INFO, "[%s] timer set up success.", __func__);

    return;
}

static void *tcp_transceiver(void *args) {

    struct thread *thread_info = (struct thread *) args;
    char buffer[BUFSIZ] = {0}; 
    ssize_t buflen = 0; 

    fprintf(stdout, "[%s] entered.\n", __func__);

    buflen = thread_info->recv(thread_info->conn_sock, buffer, sizeof(buffer), 0);

    if (buflen <= 0) {
        perror("recv ");
        return NULL;
    }

    if (pthread_mutex_lock(thread_info->mutex) != 0) {
        perror("pthread_mutex_lock ");
        return NULL;
    }

    lseek(thread_info->fd, 0, SEEK_END);
    if (thread_info->write(thread_info->fd, buffer, buflen) != buflen) {
        perror("write");
    }

    if (pthread_mutex_unlock(thread_info->mutex) != 0) {
        perror("pthread_mutex_unlock ");
        return NULL;
    }

    memset(buffer, 0, sizeof(buffer));
    lseek(thread_info->fd, 0, SEEK_SET);

    if (thread_info->read(thread_info->fd, buffer, sizeof(buffer)) == -1) {
        perror("read ");
    }

    //printf("read buffer: %s", buffer);

    if (thread_info->send(thread_info->conn_sock, buffer, strlen(buffer), 0) == -1) {
        perror("send ");
        return NULL;
    }

    return NULL;
}

tcp_thread *thread_alloc(tcp_thread *head, int conn_sock, int fd, void *mutex) {

    if (head == NULL) {
        head = (tcp_thread *) malloc(sizeof(tcp_thread));    
        head->write = write;
        head->read = read;
        head->send = send;
        head->recv = recv;
        head->conn_sock = conn_sock;
        head->fd = fd;
        head->mutex = (pthread_mutex_t *) mutex;
        head->next = NULL;
    } else {
        head->next = thread_alloc(head->next, conn_sock, fd, mutex);
    }
    return head;
}

tcp_thread *thread_lastone(tcp_thread *node) {

    while (node->next != NULL) {
        node = node->next;
    }
    return node;
}

static void thread_release(tcp_thread *head) {

    tcp_thread *temp = NULL;

    while (head != NULL) {

        if (pthread_join(head->tid, NULL)) {
            perror("pthread_join ");
        }

        temp = head;
        head = head->next;

        tcp_close(temp->conn_sock);
        syslog(LOG_INFO, "Closed connection from %s", inet_ntoa(temp->conn_addr.sin_addr));
        free(temp);
    }
    return;
}

int main(int argc, char *argv[]) {

    int tcp_sock = 0, conn_sock = 0, fd = 0, buflen = 0;
    int rc = 0;
    struct sockaddr_in conn_addr = {0};
    socklen_t addr_len = sizeof(struct sockaddr);
    pthread_mutex_t mutex;
    tcp_thread *head = NULL;

    tcp_getopt(argc, argv);

    openlog(NULL, LOG_PID, LOG_USER);

    pthread_mutex_init(&mutex, NULL);

    fd = tcp_getfd();

    timer_data.mutex = &mutex;
    timer_data.fd = fd;

    timer_setup(SIGUSR1, timer_writer);

    signal_setup(2, SIGTERM, SIGINT);

    tcp_sock = tcp_socket();

    while (!signal_sign) {

        //fprintf(stdout, "Waiting for incoming connection\n");              
        conn_sock = accept(tcp_sock, (struct sockaddr *) &conn_addr, &addr_len);

        if (conn_sock == -1) {
            perror("accept ");
        } else {
            syslog(LOG_INFO, "Accepted connection from %s", inet_ntoa(conn_addr.sin_addr));
            //fprintf(stdout, "Accepted connection from %s\n", inet_ntoa(conn_addr.sin_addr));
        }

        head = thread_alloc(head, conn_sock, fd, &mutex);
        memcpy(&head->conn_addr, &conn_addr, sizeof(struct sockaddr_in));

        if (pthread_create(&(thread_lastone(head)->tid), NULL, tcp_transceiver, (void *) thread_lastone(head))) {
            perror("pthread_create ");
        }
    }

    //printf("Receive complete, %d.\n", __LINE__);

    thread_release(head);

    close(fd);
    tcp_close(tcp_sock);
    timer_delete(timer_data.timer_id);
    pthread_mutex_destroy(&mutex);
    fprintf(stdout, "Process exit.\n");

    return EXIT_SUCCESS;
}
