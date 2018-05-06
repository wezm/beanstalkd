#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>
#include "dat.h"

Server *server_new(void) {
    struct Server *srv = calloc(1, sizeof(struct Server));
    if (!srv) {
        twarnx("OOM");
        return NULL;
    }

    srv->store = job_store_new();
    if (!srv->store) {
        free(srv);
        return NULL;
    }
    srv->port = Portdef;
    srv->wal.filesize = Filesizedef;

    return srv;
}

// TODO: server_free()

void
srvserve(Server *s)
{
    int r;
    Socket *sock;
    int64 period;

    if (sockinit() == -1) {
        twarnx("sockinit");
        exit(1);
    }

    s->sock.x = s;
    s->sock.f = (Handle)srvaccept;
    s->conns.less = (Less)connless;
    s->conns.rec = (Record)connrec;

    r = listen(s->sock.fd, 1024);
    if (r == -1) {
        twarn("listen");
        return;
    }

    r = sockwant(&s->sock, 'r');
    if (r == -1) {
        twarn("sockwant");
        exit(2);
    }


    for (;;) {
        period = prottick(s);

        int rw = socknext(&sock, period);
        if (rw == -1) {
            twarnx("socknext");
            exit(1);
        }

        if (rw) {
            sock->f(sock->x, rw);
        }
    }
}


void
srvaccept(Server *s, int ev)
{
    h_accept(s->sock.fd, ev, s);
}
