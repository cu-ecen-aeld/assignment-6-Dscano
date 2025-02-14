/*
 * =====================================================================================
 *
 *       Filename:  aesdsocket.h
 *
 *    Description:  Assignment 5 part 1 header file
 *
 *        Version:  1.0
 *        Created:  2023年04月12日 23時42分40秒
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

#ifndef AESD_SCOKET
#define AESD_SOCKET 
//#define ERROR_HANDLER(func) { perror(#func); exit(EXIT_FAILURE); }
#define ERROR_HANDLER(func) { perror(#func); }
//#define USER_LOGGING(format, addr, port) syslog(LOG_DEBUG, format, addr, port)
#define USER_LOGGING(format, ...) syslog(LOG_DEBUG, format, __VA_ARGS__)
extern int signal_sign;
#endif
