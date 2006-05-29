/*
 * $Id: $
 */

#include "compat.h"
#include "ff-plugins.h"

/* Forwards */
PLUGIN_INFO *plugin_info(PLUGIN_INPUT_FN *ppi);
void plugin_handler(int, int, void *, int);

#define PIPE_BUFFER_SIZE 4096

#define USE_UDP 0
#define USE_MAILSLOT 1

/* Globals */
PLUGIN_EVENT_FN _pefn = { plugin_handler };
PLUGIN_INPUT_FN *_ppi;

PLUGIN_INFO _pi = { 
    PLUGIN_VERSION,        /* version */
    PLUGIN_EVENT,          /* type */
    "w32-event/" VERSION,  /* server */
    NULL,                  /* url */
    NULL,                  /* output fns */
    &_pefn,                /* event fns */
    NULL,                  /* transocde fns */
    NULL,                  /* rend info */
    NULL                   /* codec list */
};

typedef struct tag_plugin_msg {
    int size;
    int event_id;
    int intval;
    char vp[1];
} PLUGIN_MSG;


PLUGIN_INFO *plugin_info(PLUGIN_INPUT_FN *ppi) {
    _ppi = ppi;
    return &_pi;
}

#if USE_UDP
/** NO LOG IN HERE!  We'll go into an endless loop.  :) */
void plugin_handler(int event_id, int intval, void *vp, int len) {
    int total_len = 3 * sizeof(int) + len + 1;
    PLUGIN_MSG *pmsg;
    int port = 9999;
    SOCKET sock;
    struct sockaddr_in servaddr;

    pmsg = (PLUGIN_MSG*)malloc(total_len);
    if(!pmsg) {
//        _ppi->log(E_LOG,"Malloc error in w32-event.c/plugin_handler\n");
        return;
    }

    memset(pmsg,0,total_len);
    pmsg->size = total_len;
    pmsg->event_id = event_id;
    pmsg->intval = intval;
    memcpy(&pmsg->vp,vp,len);

    sock = socket(AF_INET,SOCK_DGRAM,0);
    if(sock == INVALID_SOCKET) {
        free(pmsg);
        return;
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(port);

    sendto(sock,(char*)pmsg,total_len,0,(struct sockaddr *)&servaddr,sizeof(servaddr));

    closesocket(sock);
    free(pmsg);
    return;
}
#endif /* USE_UDP */

#if USE_MAILSLOT
#define MAILSLOT_NAME "\\\\.\\mailslot\\FireflyMediaServer--67A72768-4154-417e-BFA0-FA9B50C342DE"
/** NO LOG IN HERE!  We'll go into an endless loop.  :) */
void plugin_handler(int event_id, int intval, void *vp, int len) {
	HANDLE h = CreateFile(MAILSLOT_NAME, GENERIC_WRITE,
						 FILE_SHARE_READ, NULL, OPEN_EXISTING,
						 FILE_ATTRIBUTE_NORMAL, NULL);
	if (h != INVALID_HANDLE_VALUE)
	{
		DWORD bytes_written;
		const int packet_size = 12 + len;
		unsigned char *buffer = (unsigned char *)malloc(packet_size);
		if(!buffer)
			return;
		memset(buffer, 0, packet_size);

		buffer[0] = packet_size & 0xff;
		buffer[1] = (packet_size >> 8) & 0xff;
		buffer[2] = (packet_size >> 16) & 0xff;
		buffer[3] = (packet_size >> 24) & 0xff;

		buffer[4] = event_id & 0xff;
		buffer[5] = (event_id >> 8) & 0xff;
		buffer[6] = (event_id >> 16) & 0xff;
		buffer[7] = (event_id >> 24) & 0xff;

		buffer[8] = intval & 0xff;
		buffer[9] = (intval >> 8) & 0xff;
		buffer[10] = (intval >> 16) & 0xff;
		buffer[11] = (intval >> 24) & 0xff;

		memcpy(buffer + 12, vp, len);

		/* If this fails then there's nothing we can do about it anyway. */
		WriteFile(h, buffer, packet_size, &bytes_written, NULL);
		CloseHandle(h);
	}
}
#endif /* USE_MAILSLOT */