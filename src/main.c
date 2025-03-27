/*
 * main.c
 *
 *  Created on: Sep 24, 2013
 *      Author: shved
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <getopt.h>
#include <arpa/nameser.h>
#include <resolv.h>

#include "log.h"
#include "net.h"
#include "app.h"
#include "aor.h"

static const char *log_priority(const int priority) {
    static struct {
        int priority;
        char *name;
    } *m, priorities[] = {

                __type_name(LOG_EMERG),
                __type_name(LOG_ALERT),
                __type_name(LOG_CRIT),
                __type_name(LOG_ERR),
                __type_name(LOG_WARNING),
                __type_name(LOG_NOTICE),
                __type_name(LOG_INFO),
                __type_name(LOG_DEBUG),

                __type_name_END
            };

    for (m = priorities; m->name; ++m)
        if (m->priority == priority)
            return m->name;

    static char buff[16];

    sprintf(buff, "LOG_%d", priority);

    return buff;
}

static void usage(char *filename, char *hostname, char *resolver) {
    printf("%s (SIP Front-end) %i.%i.%i\n", basename(filename), PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR,
           PROJECT_VERSION_PATCH);
    printf("Copyright (C) 2013 inCOMa, LDT. http://www.incoma.ru\n");
    printf("Written by Pavel Shvedko <shved@mail.ru>\n");
    printf("\n");
    printf("Usage: %s [OPTION]\n", filename);
    printf("  -h, --help\t\tdisplay this help and exit\n");
    printf("  -d, --daemon\t\trun in background\n");
    printf("  -H, --hostname=fqdn\tfully qualified domain name, default \"%s\"\n", hostname);
    printf("  -R, --resolver=ip\taddress of nameserver, default \"%s\"\n", resolver);
    printf("  -p, --pidfile=path\tto pid file, default \"%s.pid\"\n", basename(filename));
    printf("  -q, --quiet\t\tturn off any debug, except [LOG_EMERG]\n");
    printf("  -l, --debug=level\tset debug level, default 4 [LOG_WARNING]\n");
    printf("  -x\t\t\tincrease debug level by 1\n");
    printf("\n");

    exit(0);
}

/**
 *
 */
int main(const int argc, char **argv) {
    int option_daemon = 0;
    int option_index = 0;
    int option_last = 0;
    int option_log = LOG_PERROR | LOG_NDELAY;
    int option_priority = LOG_WARNING;
    int option_level = 0;
    char option_pid_file[PATH_MAX] = "";

    res_init();

    char resolver[64];
    inet_ntop(_res.nsaddr_list[0].sin_family, &_res.nsaddr_list[0].sin_addr, resolver, sizeof(resolver) - 1);

    char hostname[256];
    gethostname(hostname, sizeof(hostname) - 1);

    const struct option option_array[] = {
        {"help", 0, NULL, 'h'},
        {"daemon", 0, NULL, 'd'},
        {"quiet", 0, NULL, 'q'},
        {"debug", 1, NULL, 'l'},
        {"pidfile", 1, NULL, 'p'},
        {"hostname", 1, NULL, 'H'},
        {"resolver", 1, NULL, 'R'},
        {0, 0, 0, 0}
    };

    while (!option_last) {
        switch (getopt_long(argc, argv, "dhqxl:p:R:H:", option_array, &option_index)) {
            case -1:
                option_last = 1;
                option_level += option_priority;
                if (option_level > LOG_DEBUG)
                    option_level = LOG_DEBUG;
                if (!*option_pid_file)
                    snprintf(option_pid_file, PATH_MAX, "%s.pid", basename(*argv));
                break;
            case 'R':
                strncpy(resolver, optarg, sizeof(resolver) - 1);
                break;
            case 'H':
                strncpy(hostname, optarg, sizeof(hostname) - 1);
                break;
            case 'd':
                option_daemon = 1;
                option_log = LOG_NDELAY | LOG_PID;
                break;
            case 'l':
                option_priority = atoi(optarg);
                break;
            case 'q':
                option_priority = LOG_EMERG;
                break;
            case 'x':
                option_level += 1;
                break;
            case 'p':
                strncpy(option_pid_file, optarg, PATH_MAX - 1);
                break;
            case 'h':
            default:
                usage(*argv, hostname, resolver);
                break;
        }
    }

    if (option_daemon != 0) {
        if (daemon(1, 0)) {
            perror("daemon");
            return -1;
        }
    }

    aor_init();
    dns_init();
    net_init();
    sip_init();
    cmd_init();

    app_set_hostname(&__g_app_DNS, resolver);
    app_set_hostname(&__g_app_CMD, hostname);
    app_set_hostname(&__g_app_SIP, hostname);

    log_open("sip", option_log, LOG_USER);

    log_level(option_level);

    log_alert("SIP begins, nameserver: [%s], hostname: [%s], level: [%s]",
              resolver,
              hostname,
              log_priority(option_level));

    int b = net_bind(&__g_net_TCP, &__g_app_SIP);
    if (b == 0)
        b = net_bind(&__g_net_UDP, &__g_app_SIP);
    // if (b == 0)
    // 	b = net_bind(&__g_net_TLS, &__g_app_SIP);
    if (b == 0)
        b = net_bind(&__g_net_UDP, &__g_app_DNS);
    if (b == 0)
        b = net_bind(&__g_net_TCP, &__g_app_CMD);
    if (b == 0)
        b = net_open(&__g_net_TCP, &__g_app_CMD, "192.168.0.244", 1883);
    if (b)
        return -1;

    const int n = open(option_pid_file, O_CREAT | O_TRUNC | O_WRONLY, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
    if (n != -1) {
        dprintf(n, "%i\n", getpid());
        close(n);
    } else
        log_alert("pidfile '%s' failed: %m", option_pid_file);

    net_loop();

    net_free();
    sip_free();
    cmd_free();
    dns_free();
    aor_free();

    log_alert("SIP done, exit.");
    log_close();

    unlink(option_pid_file);

    return 0;
}
