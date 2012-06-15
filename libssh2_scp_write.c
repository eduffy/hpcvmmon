/*
 * Sample showing how to do an SCP upload.
 */
/* [ Fri Jun 08, 2012]: Adapted from libssh2/example/scp_write.c */

#include <libssh2.h>

#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#endif

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include "strbuf.h"

int ssh2_scp_write(const char *hostip,
                   const char *username,
                   const char *password,
                   const char *loclfile,
                   const char *scppath,
                   char **errmsg)
{
    unsigned long hostaddr;
    int sock;
    struct sockaddr_in sin;
    LIBSSH2_SESSION *session = NULL;
    LIBSSH2_CHANNEL *channel;
    FILE *local;
    int rc;
    char mem[1024];
    size_t nread;
    char *ptr;
    struct stat fileinfo;
    struct StringBuffer *sberrmsg = new_string_buffer();

#ifdef WIN32
    WSADATA wsadata;

    WSAStartup(MAKEWORD(2,0), &wsadata);
#endif

    hostaddr = inet_addr(hostip);

    rc = libssh2_init (0);
    if (rc != 0) {
        sbprintf (sberrmsg, "libssh2 initialization failed (%d)\n", rc);
        goto shutdown;
    }

    local = fopen(loclfile, "rb");
    if (!local) {
        sbprintf(sberrmsg, "Can't open local file %s\n", loclfile);
        goto shutdown;
    }

    stat(loclfile, &fileinfo);

    /* Ultra basic "connect to port 22 on localhost"
     * Your code is responsible for creating the socket establishing the
     * connection
     */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == sock) {
        sbprintf(sberrmsg, "failed to create socket!\n");
        goto shutdown;
    }

    sin.sin_family = AF_INET;
    sin.sin_port = htons(22);
    sin.sin_addr.s_addr = hostaddr;
    if (connect(sock, (struct sockaddr*)(&sin),
            sizeof(struct sockaddr_in)) != 0) {
        sbprintf(sberrmsg, "failed to connect!\n");
        goto shutdown;
    }

    /* Create a session instance
     */
    session = libssh2_session_init();
    if(!session)
	goto shutdown;

    /* ... start it up. This will trade welcome banners, exchange keys,
     * and setup crypto, compression, and MAC layers
     */
    rc = libssh2_session_handshake(session, sock);
    if(rc) {
        sbprintf(sberrmsg, "Failure establishing SSH session: %d\n", rc);
        goto shutdown;
    }

    /* At this point we havn't yet authenticated.  The first thing to do
     * is check the hostkey's fingerprint against our known hosts Your app
     * may have it hard coded, may go to a file, may present it to the
     * user, that's your call
     */
/*
    fingerprint = libssh2_hostkey_hash(session, LIBSSH2_HOSTKEY_HASH_SHA1);
    fprintf(stderr, "Fingerprint: ");
    for(i = 0; i < 20; i++) {
        fprintf(stderr, "%02X ", (unsigned char)fingerprint[i]);
    }
    fprintf(stderr, "\n");
*/

    /* We could authenticate via password */
    if (libssh2_userauth_password(session, username, password)) {
        sbprintf(sberrmsg, "Authentication by password failed.\n");
        goto shutdown;
    }

    /* Send a file via scp. The mode parameter must only have permissions! */
    channel = libssh2_scp_send(session, scppath, fileinfo.st_mode & 0777,
                               (unsigned long)fileinfo.st_size);

    if (!channel) {
        char *errmsg;
        int errlen;
        int err = libssh2_session_last_error(session, &errmsg, &errlen, 0);
        sbprintf(sberrmsg, "Unable to open a session: (%d) %s\n", err, errmsg);
        goto shutdown;
    }

   fprintf(stderr, "SCP session waiting to send file\n");
    do {
        nread = fread(mem, 1, sizeof(mem), local);
        if (nread <= 0) {
            /* end of file */
            break;
        }
        ptr = mem;

        do {
            /* write the same data over and over, until error or completion */
            rc = libssh2_channel_write(channel, ptr, nread);
            if (rc < 0) {
                sbprintf(sberrmsg, "ERROR %d\n", rc);
                break;
            }
            else {
                /* rc indicates how many bytes were written this time */
                ptr += rc;
                nread -= rc;
            }
        } while (nread);

    } while (1);

    fprintf(stderr, "Sending EOF\n");
    libssh2_channel_send_eof(channel);

    fprintf(stderr, "Waiting for EOF\n");
    libssh2_channel_wait_eof(channel);

    fprintf(stderr, "Waiting for channel to close\n");
    libssh2_channel_wait_closed(channel);

    libssh2_channel_free(channel);
    channel = NULL;

 shutdown:

    if(session) {
        /* libssh2_session_disconnect(session, "Normal Shutdown, Thank you for playing"); */
        libssh2_session_free(session);
    }
#ifdef WIN32
    closesocket(sock);
#else
    close(sock);
#endif
    if (local)
        fclose(local);
    /* fprintf(stderr, "all done\n"); */

    libssh2_exit();
    if(string_buffer_is_empty(sberrmsg)) {
       free_string_buffer(sberrmsg, 1);
       return 0;
    }

    *errmsg = free_string_buffer(sberrmsg, 0);
    return 1;
}
