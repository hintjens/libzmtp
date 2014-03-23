/*  =========================================================================
    zmtp_dealer - DEALER socket class

    Copyright contributors as noted in the AUTHORS file.
    This file is part of libzmtp, the C ZMTP stack.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

#include "../include/zmtp.h"

struct zmtp_dealer {
    zmtp_connection_t *conn;
};

zmtp_dealer_t *
zmtp_dealer_new (int fd)
{
    zmtp_dealer_t *self = (zmtp_dealer_t *) malloc (sizeof *self);
    if (self) {
        *self = (zmtp_dealer_t) { .conn = zmtp_connection_new (fd, ZMTP_DEALER) };
        if (!self->conn) {
            free (self);
            return NULL;
        }
        if (zmtp_connection_negotiate (self->conn) == -1) {
            zmtp_connection_destroy (&self->conn);
            free (self);
            return NULL;
        }
    }
    return self;
}

zmtp_dealer_t *
zmtp_dealer_ipc_connect (const char *path)
{
    struct sockaddr_un remote = { .sun_family = AF_UNIX };
    if (strlen (path) >= sizeof remote.sun_path)
        return NULL;
    strcpy (remote.sun_path, path);
    //  Create socket
    const int s = socket (AF_UNIX, SOCK_STREAM, 0);
    if (s == -1)
        return NULL;
    //  Connect the socket
    const int rc =
        connect (s, (const struct sockaddr *) &remote, sizeof remote);
    if (rc == -1) {
        close (s);
        return NULL;
    }
    else {
        zmtp_dealer_t *self = zmtp_dealer_new (s);
        if (!self)
            close (s);
        return self;
    }
}

zmtp_dealer_t *
zmtp_dealer_tcp_connect (const char *addr, unsigned short port)
{
    char service [8 + 1];
    snprintf (service, sizeof service, "%u", port);

    //  Create socket
    const int s = socket (AF_INET, SOCK_STREAM, 0);
    if (s == -1)
        return NULL;
    //  Resolve address
    const struct addrinfo hints = {
        .ai_family   = AF_INET,
        .ai_socktype = SOCK_STREAM,
        .ai_flags    = AI_NUMERICHOST | AI_NUMERICSERV
    };
    struct addrinfo *result = NULL;
    if (getaddrinfo (addr, service, &hints, &result)) {
        close (s);
        return NULL;
    }
    assert (result);
    //  Create socket
    const int rc = connect (s, result->ai_addr, result->ai_addrlen);
    freeaddrinfo (result);
    if (rc == -1) {
        close (s);
        return NULL;
    }
    else {
        zmtp_dealer_t *self = zmtp_dealer_new (s);
        if (!self)
            close (s);
        return self;
    }
}

void
zmtp_dealer_destroy (zmtp_dealer_t **self_p)
{
    assert (self_p);

    if (*self_p) {
        zmtp_dealer_t *self = *self_p;
        zmtp_connection_destroy (&self->conn);
        free (self);
        *self_p = NULL;
    }
}

int
zmtp_dealer_send (zmtp_dealer_t *self, zmtp_msg_t *msg)
{
    assert (self);
    assert (self->conn);

    return zmtp_connection_send (self->conn, msg);
}

zmtp_msg_t *
zmtp_dealer_recv (zmtp_dealer_t *self)
{
    assert (self);
    assert (self->conn);

    return zmtp_connection_recv (self->conn);
}
