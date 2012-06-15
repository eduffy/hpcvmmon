
/* [Thu May 31, 2012]: Adapted from libssh2/example/direct_tcpip.c */

#include <libssh2.h>

#ifdef WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#endif

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include <sys/select.h>

#include "strbuf.h"

#ifndef INADDR_NONE
#define INADDR_NONE (in_addr_t)-1
#endif

int ssh2_forward_port(const char *hostip,
                      const char *username,
                      const char *password,
                      int listenport,
                      const char *desthost,
                      int destport,
                      int *initialized,
                      int *finished,
                      char **errmsg)
{
    int rc, sock = -1, listensock = -1, forwardsock = -1, i;
    struct sockaddr_in sin;
    socklen_t sinlen;
    LIBSSH2_SESSION *session;
    LIBSSH2_CHANNEL *channel = NULL;
    const char *shost;
    unsigned int sport;
    fd_set fds;
    struct timeval tv;
    ssize_t len, wr;
    char buf[16384];
    struct StringBuffer *sberrmsg = new_string_buffer();

#ifdef WIN32
    char sockopt;
    WSADATA wsadata;

    WSAStartup(MAKEWORD(2,0), &wsadata);
#else
    int sockopt;
#endif

    rc = libssh2_init (0);
    if (rc != 0) {
        sbprintf (sberrmsg, "libssh2 initialization failed (%d)\n", rc);
        return 1;
    }

    /* Connect to SSH server */
    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    sin.sin_family = AF_INET;
    if (INADDR_NONE == (sin.sin_addr.s_addr = inet_addr(hostip))) {
        perror("inet_addr");
        return -1;
    }
    sin.sin_port = htons(22);
    if (connect(sock, (struct sockaddr*)(&sin),
                sizeof(struct sockaddr_in)) != 0) {
        sbprintf(sberrmsg, "failed to connect!\n");
        return -1;
    }

    /* Create a session instance */
    session = libssh2_session_init();
    if(!session) {
        sbprintf(sberrmsg, "Could not initialize SSH session!\n");
        return -1;
    }

    /* ... start it up. This will trade welcome banners, exchange keys,
     * and setup crypto, compression, and MAC layers
     */
    rc = libssh2_session_handshake(session, sock);
    if(rc) {
        sbprintf(sberrmsg, "Error when starting up SSH session: %d\n", rc);
        return -1;
    }

    if (libssh2_userauth_password(session, username, password)) {
        sbprintf(sberrmsg, "Authentication by password failed.\n");
        goto shutdown;
    }

    listensock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(listenport);
    if (INADDR_NONE == (sin.sin_addr.s_addr = inet_addr("127.0.0.1"))) {
        sbprintf(sberrmsg, "inet_addr: %s.", strerror(errno));
        goto shutdown;
    }
    sockopt = 1;
    setsockopt(listensock, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt));
    sinlen=sizeof(sin);
    if (-1 == bind(listensock, (struct sockaddr *)&sin, sinlen)) {
        sbprintf(sberrmsg, "bind: %s.", strerror(errno));
        goto shutdown;
    }
    if (-1 == listen(listensock, 2)) {
        sbprintf(sberrmsg, "listen: %s.", strerror(errno));
        goto shutdown;
    }

    printf("Waiting for TCP connection on %s:%d...\n",
        inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
    *initialized = 1;

    forwardsock = accept(listensock, (struct sockaddr *)&sin, &sinlen);
    if (-1 == forwardsock) {
        sbprintf(sberrmsg, "accept: %s.", strerror(errno));
        goto shutdown;
    }

    shost = inet_ntoa(sin.sin_addr);
    sport = ntohs(sin.sin_port);

    printf("Forwarding connection from %s:%d here to remote %s:%d\n", shost,
        sport, desthost, destport);

    channel = libssh2_channel_direct_tcpip_ex(session, desthost,
        destport, shost, sport);
    if (!channel) {
        sbprintf(sberrmsg, "Could not open the direct-tcpip channel!\n"
                "(Note that this can be a problem at the server!"
                " Please review the server logs.)\n");
        goto shutdown;
    }

    /* Must use non-blocking IO hereafter due to the current libssh2 API */
    libssh2_session_set_blocking(session, 0);

    while (1) {
        if(*finished) libssh2_channel_send_eof(channel);
        FD_ZERO(&fds);
        FD_SET(forwardsock, &fds);
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        rc = select(forwardsock + 1, &fds, NULL, NULL, &tv);
        if (-1 == rc) {
            sbprintf(sberrmsg, "select: %s.", strerror(errno));
            goto shutdown;
        }
        if (rc && FD_ISSET(forwardsock, &fds)) {
            len = recv(forwardsock, buf, sizeof(buf), 0);
            if (len < 0) {
                perror("read");
                goto shutdown;
            } else if (0 == len) {
                printf("The client at %s:%d disconnected!\n", shost, sport);
                goto shutdown;
            }
            wr = 0;
            do {
                i = libssh2_channel_write(channel, buf, len);
                if (i < 0) {
                    sbprintf(sberrmsg, "libssh2_channel_write: %d\n", i);
                    goto shutdown;
                }
                wr += i;
            } while(i > 0 && wr < len);
        }
        while (1) {
            len = libssh2_channel_read(channel, buf, sizeof(buf));
            if (LIBSSH2_ERROR_EAGAIN == len)
                break;
            else if (len < 0) {
                sbprintf(sberrmsg, "libssh2_channel_read: %d", (int)len);
                goto shutdown;
            }
            wr = 0;
            while (wr < len) {
                i = send(forwardsock, buf + wr, len - wr, 0);
                if (i <= 0) {
                    perror("write");
                    goto shutdown;
                }
                wr += i;
            }
            if (libssh2_channel_eof(channel)) {
                printf("The server at %s:%d disconnected!\n",
                    desthost, destport);
                goto shutdown;
            }
        }
    }

shutdown:
 fprintf(stderr, "finished = %d\n", *finished);
#ifdef WIN32
    closesocket(forwardsock);
    closesocket(listensock);
#else
    close(forwardsock);
    close(listensock);
#endif
    if (channel)
        libssh2_channel_free(channel);
    libssh2_session_disconnect(session, "Client disconnecting normally");
    libssh2_session_free(session);

#ifdef WIN32
    closesocket(sock);
#else
    close(sock);
#endif

    libssh2_exit();

    if(string_buffer_is_empty(sberrmsg)) {
       free_string_buffer(sberrmsg, 1);
       return 0;
    }

    *errmsg = free_string_buffer(sberrmsg, 0);
    return 1;
}
