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

static int waitsocket(int socket_fd, LIBSSH2_SESSION *session)
{
    struct timeval timeout;
    int rc;
    fd_set fd;
    fd_set *writefd = NULL;
    fd_set *readfd = NULL;
    int dir;

    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    FD_ZERO(&fd);

    FD_SET(socket_fd, &fd);

    /* now make sure we wait in the correct direction */
    dir = libssh2_session_block_directions(session);

    if(dir & LIBSSH2_SESSION_BLOCK_INBOUND)
        readfd = &fd;

    if(dir & LIBSSH2_SESSION_BLOCK_OUTBOUND)
        writefd = &fd;

    rc = select(socket_fd + 1, readfd, writefd, NULL, &timeout);

    return rc;
}

int ssh2_exec(const char *hostip,
              const char *username,
              const char *password, 
              const char *command,
              char **result,
              char **errmsg)
{
    unsigned long hostaddr;
    int sock;
    struct sockaddr_in sin;
    LIBSSH2_SESSION *session;
    LIBSSH2_CHANNEL *channel;
    int rc;
    int exitcode;
    char *exitsignal=(char *)"none";
    int bytecount = 0;
    LIBSSH2_KNOWNHOSTS *nh;
    struct StringBuffer *sbresult = new_string_buffer();
    struct StringBuffer *sberrmsg = new_string_buffer();

#ifdef WIN32
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2,0), &wsadata);
#endif

    rc = libssh2_init (0);
    if (rc != 0) {
        sbprintf (sberrmsg, "libssh2 initialization failed (%d)\n", rc);
        free_string_buffer(sbresult, 1);
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
        free_string_buffer(sbresult, 1);
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
        free_string_buffer(sbresult, 1);
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
        free_string_buffer(sbresult, 1);
        *errmsg = free_string_buffer(sberrmsg, 0);
        goto shutdown;
    }
#if 0
    libssh2_trace(session, ~0 );
#endif

    /* Exec non-blocking on the remove host */
    while( (channel = libssh2_channel_open_session(session)) == NULL &&
           libssh2_session_last_error(session,NULL,NULL,0) ==
           LIBSSH2_ERROR_EAGAIN )
    {
        waitsocket(sock, session);
    }
    if( channel == NULL )
    {
        sbprintf(sberrmsg,"Error\n");
        free_string_buffer(sbresult, 1);
        *errmsg = free_string_buffer(sberrmsg, 0);
        return 1;
    }
    while( (rc = libssh2_channel_exec(channel, command)) ==
           LIBSSH2_ERROR_EAGAIN )
    {
        waitsocket(sock, session);
    }
    if( rc != 0 )
    {
        sbprintf(sberrmsg,"Error\n");
        free_string_buffer(sbresult, 1);
        *errmsg = free_string_buffer(sberrmsg, 0);
        return 1;
    }
    for( ;; )
    {
        /* loop until we block */
        int rc;
        do
        {
            char buffer[0x4000];
            rc = libssh2_channel_read( channel, buffer, sizeof(buffer) );
            if( rc > 0 )
            {
                int i;
                bytecount += rc;
                /* fprintf(stderr, "We read:\n");
                for( i=0; i < rc; ++i )
                    fputc( buffer[i], stderr);
                fprintf(stderr, "\n"); */
                for( i=0; i < rc; ++i )
                    sbprintf(sbresult, "%c", buffer[i]);
            }
            else {
               // if( rc != LIBSSH2_ERROR_EAGAIN )
               //     /* no need to output this for the EAGAIN case */
               //     fprintf(stderr, "libssh2_channel_read returned %d\n", rc);
            }
        }
        while( rc > 0 );

        /* this is due to blocking that would occur otherwise so we loop on
           this condition */
        if( rc == LIBSSH2_ERROR_EAGAIN )
        {
            waitsocket(sock, session);
        }
        else
            break;
    }
    exitcode = 127;
    while( (rc = libssh2_channel_close(channel)) == LIBSSH2_ERROR_EAGAIN )
        waitsocket(sock, session);

    if( rc == 0 )
    {
        exitcode = libssh2_channel_get_exit_status( channel );
        libssh2_channel_get_exit_signal(channel, &exitsignal,
                                        NULL, NULL, NULL, NULL, NULL);
    }

    if (exitsignal)
        printf("\nGot signal: %s\n", exitsignal);
    else 
        printf("\nEXIT: %d bytecount: %d\n", exitcode, bytecount);

    libssh2_channel_free(channel);
    channel = NULL;

shutdown:

    libssh2_session_disconnect(session,
                               "Normal Shutdown, Thank you for playing");
    libssh2_session_free(session);

#ifdef WIN32
    closesocket(sock);
#else
    close(sock);
#endif

    libssh2_exit();

    *result = free_string_buffer(sbresult, 0);
    free_string_buffer(sberrmsg, 1);
    return 0;
}
