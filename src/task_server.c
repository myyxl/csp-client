#include <csp/csp.h>

void * task_server(void * parameters) {

	/* Create socket */
	csp_socket_t * sock = csp_socket(0);

	/* Bind every port to socket*/
	csp_bind(sock, CSP_ANY);

	/* Create 10 connections backlog queue */
	csp_listen(sock, 10);

	/* Pointer to current connection and packet */
	csp_conn_t * conn;
	csp_packet_t * packet;

	/* Process incoming connections */
	while (1) {

		/* Wait for connection, 1000 ms timeout */
		if ((conn = csp_accept(sock, 1000)) == NULL)
			continue;

		/* Read packets. Timout is 1000 ms */
		while ((packet = csp_read(conn, 1000)) != NULL) {

			switch(csp_conn_dport(conn)) {

				default: {
					csp_service_handler(conn, packet);
					csp_close(conn);
					break;
				}

			}

		}

		/* Close current connection, and handle next */
		csp_close(conn);

	}

	return NULL;

}

