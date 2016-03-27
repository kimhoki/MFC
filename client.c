#include "tcp_ip.h"

void error_handler( char* err_msg )
{
	fputs( err_msg, stderr );
	fputc( '\n', stderr );
	exit(1);
}

void* send_msg( void* arg )
{
	char msg[BUFSIZE];

	int sock = (int) arg;        
	while (1)
	{
		fgets( msg, BUFSIZE, stdin );
		if ( !strcmp(msg, "q\n") )
		{
			close( sock );
			exit(0);
		}
		write( sock, msg, strlen(msg) );
	}
}

void* recv_msg( void* arg )
{
	int sock = (int) arg;
	char recieved[BUFSIZE+IDSIZE+15];
	int len;

	while (1)
	{
		len = read( sock, recieved, BUFSIZE+IDSIZE+14 );
		if ( len == -1 )
			return (void*)1;
		recieved[len] = 0;
		fputs( recieved, stdout );
	}
}

int main( int argc, char** argv )
{
	int sock;
	struct sockaddr_in serv_addr;
	pthread_t send_thrd, recv_thrd;
	void* thr_rtn_val;
	char id[IDSIZE];

	if ( argc != 4 )
	{
		printf( "Usage : %s <IP> <port> <ID>\n", argv[0] );
		exit(1);
	}

	sprintf( id, "%s", argv[3] );

	sock = socket( PF_INET, SOCK_STREAM, 0 );
	if ( sock == -1 )
		error_handler( "socket() error" );

	memset( &serv_addr, 0, sizeof(serv_addr) );
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr( argv[1] );
	serv_addr.sin_port = htons( atoi(argv[2]) );

	if ( connect(sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1 )
		error_handler( "connect() error" );

	write( sock, id, sizeof(id) );        // send my id

	pthread_create( &send_thrd, NULL, send_msg, (void*) sock );
	pthread_create( &recv_thrd, NULL, recv_msg, (void*) sock );

	pthread_join( send_thrd, &thr_rtn_val );
	pthread_join( recv_thrd, &thr_rtn_val );        // wait until threads die

	close( sock );
	return 0;
}
