#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include "ct/ct.h"
#include "dat.h"

void
cttestallocf()
{
    char *got;

    got = fmtalloc("hello, %s %d", "world", 5);
    assertf(strcmp("hello, world 5", got) == 0, "got \"%s\"", got);
}


void
cttestoptnone()
{
    Server *srv = server_new();
    char *args[] = {
        NULL,
    };

    optparse(srv, args);
    assert(strcmp(srv->port, Portdef) == 0);
    assert(srv->addr == NULL);
    assert(job_data_size_limit == JOB_DATA_SIZE_LIMIT_DEFAULT);
    assert(srv->wal.filesize == Filesizedef);
    assert(srv->wal.nocomp == 0);
    assert(srv->wal.wantsync == 0);
    assert(srv->user == NULL);
    assert(srv->wal.dir == NULL);
    assert(srv->wal.use == 0);
    assert(verbose == 0);
}


static void
success(void)
{
    _exit(0);
}


void
cttestoptminus()
{
    Server *srv = server_new();
    char *args[] = {
        "-",
        NULL,
    };

    atexit(success);
    optparse(srv, args);
    assertf(0, "optparse failed to call exit");
}


void
cttestoptp()
{
    Server *srv = server_new();
    char *args[] = {
        "-p1234",
        NULL,
    };

    optparse(srv, args);
    assert(strcmp(srv->port, "1234") == 0);
}


void
cttestoptl()
{
    Server *srv = server_new();
    char *args[] = {
        "-llocalhost",
        NULL,
    };

    optparse(srv, args);
    assert(strcmp(srv->addr, "localhost") == 0);
}


void
cttestoptlseparate()
{
    Server *srv = server_new();
    char *args[] = {
        "-l",
        "localhost",
        NULL,
    };

    optparse(srv, args);
    assert(strcmp(srv->addr, "localhost") == 0);
}


void
cttestoptz()
{
    Server *srv = server_new();
    char *args[] = {
        "-z1234",
        NULL,
    };

    optparse(srv, args);
    assert(job_data_size_limit == 1234);
}


void
cttestopts()
{
    Server *srv = server_new();
    char *args[] = {
        "-s1234",
        NULL,
    };

    optparse(srv, args);
    assert(srv->wal.filesize == 1234);
}


void
cttestoptc()
{
    Server *srv = server_new();
    char *args[] = {
        "-n",
        "-c",
        NULL,
    };

    optparse(srv, args);
    assert(srv->wal.nocomp == 0);
}


void
cttestoptn()
{
    Server *srv = server_new();
    char *args[] = {
        "-n",
        NULL,
    };

    optparse(srv, args);
    assert(srv->wal.nocomp == 1);
}


void
cttestoptf()
{
    Server *srv = server_new();
    char *args[] = {
        "-f1234",
        NULL,
    };

    optparse(srv, args);
    assert(srv->wal.syncrate == 1234000000);
    assert(srv->wal.wantsync == 1);
}


void
cttestoptF()
{
    Server *srv = server_new();
    char *args[] = {
        "-f1234",
        "-F",
        NULL,
    };

    optparse(srv, args);
    assert(srv->wal.wantsync == 0);
}


void
cttestoptu()
{
    Server *srv = server_new();
    char *args[] = {
        "-ukr",
        NULL,
    };

    optparse(srv, args);
    assert(strcmp(srv->user, "kr") == 0);
}


void
cttestoptb()
{
    Server *srv = server_new();
    char *args[] = {
        "-bfoo",
        NULL,
    };

    optparse(srv, args);
    assert(strcmp(srv->wal.dir, "foo") == 0);
    assert(srv->wal.use == 1);
}


void
cttestoptV()
{
    Server *srv = server_new();
    char *args[] = {
        "-V",
        NULL,
    };

    optparse(srv, args);
    assert(verbose == 1);
}


void
cttestoptV_V()
{
    Server *srv = server_new();
    char *args[] = {
        "-V",
        "-V",
        NULL,
    };

    optparse(srv, args);
    assert(verbose == 2);
}


void
cttestoptVVV()
{
    Server *srv = server_new();
    char *args[] = {
        "-VVV",
        NULL,
    };

    optparse(srv, args);
    assert(verbose == 3);
}


void
cttestoptVnVu()
{
    Server *srv = server_new();
    char *args[] = {
        "-VnVukr",
        NULL,
    };

    optparse(srv, args);
    assert(verbose == 2);
    assert(srv->wal.nocomp == 1);
    assert(strcmp(srv->user, "kr") == 0);
}
