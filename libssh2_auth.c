/* [Thu May 31, 2012]: Adapted from libssh2/example/ssh2_exec.c */

/*
 * Sample showing how to use libssh2 to execute a command remotely.
 *
 * The sample code has fixed values for host name, user name, password
 * and command to run.
 *
 * Run it like this:
 *
 * $ ./ssh2_exec 127.0.0.1 user password "uptime"
 *
 */

#include <libssh2.h>
#ifdef WIN32
#else
#define HAVE_SYS_SOCKET_H 1
#define HAVE_NETINET_IN_H 1
#define HAVE_SYS_SELECT_H 1
#define HAVE_UNISTD_H 1
#define HAVE_ARPA_INET_H 1
#endif

#ifdef HAVE_WINSOCK2_H
# include <winsock2.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_SYS_SELECT_H
# include <sys/select.h>
#endif
# ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif

#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include "strbuf.h"

int ssh2_auth(const char *hostip,
              const char *username,
              const char *password, 
              char **errmsg)
{
    unsigned long hostaddr;
    int sock;
    struct sockaddr_in sin;
    LIBSSH2_SESSION *session;
    int rc;
    LIBSSH2_KNOWNHOSTS *nh;
    struct StringBuffer *sberrmsg = new_string_buffer();

#ifdef WIN32
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2,0), &wsadata);
#endif

    rc = libssh2_init (0);
    if (rc != 0) {
        sbprintf (sberrmsg, "libssh2 initialization failed (%d)\n", rc);
        *errmsg = free_string_buffer(sberrmsg, 0);
        return 1;
    }

    hostaddr = inet_addr(hostip);

    /* Ultra basic "connect to port 22 on localhost"
     * Your code is responsible for creating the socket establishing the
     * connection
     */
    sock = socket(AF_INET, SOCK_STREAM, 0);

    sin.sin_family = AF_INET;
    sin.sin_port = htons(22);
    sin.sin_addr.s_addr = hostaddr;
    if (connect(sock, (struct sockaddr*)(&sin),
                sizeof(struct sockaddr_in)) != 0) {
        sbprintf(sberrmsg, "failed to connect!\n");
        *errmsg = free_string_buffer(sberrmsg, 0);
        return -1;
    }

    /* Create a session instance */
    session = libssh2_session_init();
    if (!session)
        return -1;

    /* tell libssh2 we want it all done non-blocking */
    libssh2_session_set_blocking(session, 0);

    /* ... start it up. This will trade welcome banners, exchange keys,
     * and setup crypto, compression, and MAC layers
     */
    while ((rc = libssh2_session_handshake(session, sock)) ==
           LIBSSH2_ERROR_EAGAIN);
    if (rc) {
        sbprintf(sberrmsg, "Failure establishing SSH session: %d\n", rc);
        *errmsg = free_string_buffer(sberrmsg, 0);
        return -1;
    }

    nh = libssh2_knownhost_init(session);
    if(!nh) {
        /* eeek, do cleanup here */
        return 2;
    }

    /* We could authenticate via password */
    while ((rc = libssh2_userauth_password(session, username, password)) ==
           LIBSSH2_ERROR_EAGAIN);
    if (rc) {
        sbprintf(sberrmsg, "Authentication by password failed.\n");
        *errmsg = free_string_buffer(sberrmsg, 0);
    }
    else {
        free_string_buffer(sberrmsg, 1);
    }


    libssh2_session_disconnect(session,
                               "Normal Shutdown, Thank you for playing");
    libssh2_session_free(session);

#ifdef WIN32
    closesocket(sock);
#else
    close(sock);
#endif

    libssh2_exit();

    return rc;
}
