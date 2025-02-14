/*
 * =====================================================================================
 *
 *       Filename:  sig_func.c
 *
 *    Description:  signal handler
 *
 *        Version:  1.0
 *        Created:  2023年04月16日 22時50分05秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ian 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include "aesdsocket.h"

void signal_handler(int signum) {

    USER_LOGGING("%s", "Caught signal, exiting");
    signal_sign = 1;

    return;
}

void signal_setup(int amount, ...) {

    int signum;
    va_list ap;
    struct sigaction sig_info = {0};

    sigemptyset(&sig_info.sa_mask);
    sig_info.sa_handler = signal_handler;

    va_start(ap, amount);

    while (amount-- > 0) {

        signum = va_arg(ap, int);
        sigaddset(&sig_info.sa_mask, signum);

        if (sigaction(signum, &sig_info, NULL) == -1) {
            perror("sigaction ");
		}
    }

    va_end(ap);

    return;
}
