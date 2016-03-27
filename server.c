#include "tcp_ip.h"

struct clnt_info
{
	int sock;
	char* id;
};

int clnt_count = 0;
struct clnt_info* clients[CLNT_LIMIT];
pthread_mutex_t mutx;	//뮤텍스 사용 

void error_handler( char* msg )
{
	fputs( msg, stderr );
	fputc( '\n', stderr );
	exit(1);
}

void sendtoall_ex( char* msg, int sender, int len )                // send to all except sender
{
	int i;
	char id_msg[BUFSIZE+IDSIZE+3];	//소켓 통신 모든 사이즈

	pthread_mutex_lock( &mutx );
	for ( i=0; i<clnt_count; i++ )
		if ( clients[i]->sock == sender )
		{
			sprintf( id_msg, "[%s] %s", clients[i]->id, msg );        // attach sender's id
			len += strlen(clients[i]->id) + 3;
			break;
		}
	for ( i=0; i<clnt_count; i++ )
		if ( clients[i]->sock != sender )
			write( clients[i]->sock, id_msg, len );
	pthread_mutex_unlock( &mutx );
}

void sendtoone( char* msg, int sender, int len )        // usage : /id msg
{
	int i, target_sock, id_len;
	char id_msg[BUFSIZE+IDSIZE+15];
	char target_id[IDSIZE];

	id_len = strchr(msg, ' ') - msg - 1;
	strncpy( target_id, msg+1, id_len );
	target_id[id_len] = 0;
	msg = msg + id_len + 2;                // detach target id

	pthread_mutex_lock( &mutx );
	for ( i=0; i<clnt_count; i++ )
	{
		if ( clients[i]->sock == sender )        // attach sender's id
		{
			sprintf( id_msg, "[Secret from:%s] %s", clients[i]->id, msg );
			len += strlen(clients[i]->id) + 15;
		}
		if ( !strcmp(target_id, clients[i]->id) )
			target_sock = clients[i]->sock;
	}        
	pthread_mutex_unlock( &mutx );

	write( target_sock, id_msg, len );
}

void* clnt_msg_process(void* arg)
{
	int clnt_sock = (int) arg;
	char msg[BUFSIZE];
	int i, len;

	while ( (len=read(clnt_sock, msg, sizeof(msg))) != 0 )
	{
		if ( msg[0] == '/' )
			sendtoone( msg, clnt_sock, len );        // whisper
		else
			sendtoall_ex( msg, clnt_sock, len );
	}

	pthread_mutex_lock( &mutx );
	for ( i=0; i<clnt_count; i++)
		if ( clnt_sock == clients[i]->sock )
		{
			printf( "%s's connection closed.\n", clients[i]->id );
			free( clients[i]->id );
			free( clients[i] );
			for ( ; i<clnt_count-1; i++)
				clients[i] = clients[i+1];                        
			break;
		}
	clnt_count--;
	pthread_mutex_unlock( &mutx );        

	close( clnt_sock );
	return 0;
}

int main( int argc, char** argv )
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_addr;
	struct sockaddr_in clnt_addr;
	int clnt_addr_size;
	pthread_t thread;

	if ( argc != 2 )
	{
		printf( "Usage : %s <port>\n", argv[0] );
		exit(1);
	}

	if ( pthread_mutex_init(&mutx, NULL) )
		error_handler( "pthread_mutex_init() error" );

	serv_sock = socket( PF_INET, SOCK_STREAM, 0 );

	memset( &serv_addr, 0, sizeof(serv_addr) );
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl( INADDR_ANY );
	serv_addr.sin_port = htons( atoi(argv[1]) );

	if ( bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) )
		error_handler( "bind() error" );

	if ( listen(serv_sock, CLNT_LIMIT) )
		error_handler( "listen() error" );

	while (1)
	{
		clnt_addr_size = sizeof(clnt_addr);
		clnt_sock = accept( serv_sock, (struct sockaddr*) &clnt_addr, &clnt_addr_size );

		pthread_mutex_lock( &mutx );
		clients[clnt_count] = (struct clnt_info*) malloc( sizeof(struct clnt_info) );
		clients[clnt_count]->id = (char*) malloc( IDSIZE );
		clients[clnt_count]->sock = clnt_sock;
		read( clnt_sock, clients[clnt_count++]->id, IDSIZE );        // recv clnt's id
		pthread_mutex_unlock( &mutx );

		pthread_create( &thread, NULL, clnt_msg_process, (void*)clnt_sock );
		printf( "new client connected : %s\n", clients[clnt_count-1]->id );
	}

	return 0;
}
