/*
 * Copyright (c) 2013, Simone Margaritelli <evilsocket at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Gibson nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "gibson.h"
#include "linenoise/linenoise.h"
#include <getopt.h>
#include <string.h>

static char address[0xFF] = {0},
			unixsocket[0xFF] = {0};

static unsigned short port = 10128;

static char history[0xFF] = {0},
		   *home = NULL;

typedef void (*gbc_op_handler)(char *);

struct gbc_op_handler {
	char *op;
	gbc_op_handler handler;
};

static char *op_descriptions[] = {
	"",
	"SET <ttl> <key> <value>",
	"TLL <key> <ttl>",
	"GET <key>",
	"DEL <key>",
	"INC <key>",
	"DEC <key>",
	"LOCK <key> <seconds>",
	"UNLOCK <key>",
	"MSET <prefix> <value>",
	"MTTL <prefix> <ttl>",
	"MGET <prefix>",
	"MDEL <prefix>",
	"MINC <prefix>",
	"MDEC <prefix>",
	"MLOCK <prefix> <seconds>",
	"MUNLOCK <prefix>",
	"COUNT <prefix>",
	"STATS",
	"PING",
	"META <key> size|encoding|access|created|ttl|left|lock",
	"KEYS <prefix>"
};

static void gbc_help( char **argv, int exitcode ){
	printf( "Gibson client utility.\n\n" );

	printf( "%s [-h|--help] [-a|--address ADDRESS] [-p|--port PORT] [-u|--unix UNIX_SOCKET_PATH]\n\n", argv[0] );

	printf("  -h, --help            	  Print this help and exit.\n");
	printf("  -a, --address ADDRESS   	  TCP address of Gibson instance.\n" );
	printf("  -p, --port PORT   		  TCP port of Gibson instance.\n" );
	printf("  -u, --unix UNIX_SOCKET_PATH Unix socket path of Gibson instance ( overrides TCP arguments ).\n\n" );

	exit(exitcode);
}

static void gbc_commandline( int argc, char **argv ){
	static struct option long_options[] =
	{
		{"help",    no_argument,       0, 'h'},
		{"address", required_argument, 0, 'a'},
		{"port",    required_argument, 0, 'p'},
		{"unix",    required_argument, 0, 'u'},
		{0, 0, 0, 0}
	};

	static char *short_options = "ha:p:u:";

	int c, option_index = 0;
	while(1){
		c = getopt_long( argc, argv, short_options, long_options, &option_index );
		if( c == -1 )
			break;

		switch (c)
		{
			case 0:
				/* If this option set a flag, do nothing else now. */
				if (long_options[option_index].flag != 0)
					break;

			break;

			case 'h':

				gbc_help(argv,0);

			break;

			case 'a':

				strncpy( address, optarg, 0xFF );

			break;

			case 'p':

				port = atoi(optarg);

			break;

			case 'u':

				strncpy( unixsocket, optarg, 0xFF );

			break;

			default:
				gbc_help(argv,1);
		}
	}
}

static gbClient client;

static int gbc_connect(){
	int ret = -1;

	if( *unixsocket != 0x00 ){
		ret = gb_unix_connect( &client, unixsocket, 100 );
	}
	else {
		ret = gb_tcp_connect( &client, address, port, 100 );
	}

	if( ret != 0 ){
		printf( "ERROR: Could not connect to Gibson instance.\n");
	}

	return ret == 0;
}

static void gbc_print_string( unsigned char *buffer, size_t size ){
	unsigned char byte;
	unsigned int i;
	unsigned char prev_was_print = 1;

	for( i = 0; i < size; i++ ){
		byte = buffer[i];
		if( isprint(byte) ){
			printf( "%s%c", prev_was_print ? "" : " ", byte );
			prev_was_print = 1;
		}
		else {
			printf( " %02X", byte );
			prev_was_print = 0;
		}
	}
}

static void gbc_print_buffer( gbBuffer *buffer ){
	if( buffer->encoding == GB_ENC_PLAIN ){
		printf( "<STRING> " );
		gbc_print_string( buffer->buffer, buffer->size );
		printf( "\n" );
	}
	else if( buffer->encoding == GB_ENC_NUMBER ){
		printf( "<NUMBER> %ld\n", gb_reply_number(buffer) );
	}
	else {
		printf( "<UNKNOWN ENCODING 0x%2X>\n", buffer->encoding );
	}
}

static void gbc_handle_response(){
	if( client.reply.code == REPL_ERR )
		printf( "<REPL_ERR>\n" );

	else if( client.reply.code == REPL_ERR_NOT_FOUND )
		printf( "<REPL_ERR_NOT_FOUND>\n" );

	else if( client.reply.code == REPL_ERR_NAN )
		printf( "<REPL_ERR_NAN>\n" );

	else if( client.reply.code == REPL_ERR_MEM )
		printf( "<REPL_ERR_MEM>\n" );

	else if( client.reply.code == REPL_ERR_LOCKED )
		printf( "<REPL_ERR_LOCKED>\n" );

	else if( client.reply.code == REPL_OK )
		printf( "<REPL_OK>\n" );

	else if( client.reply.code == REPL_VAL ) {
		if( client.reply.buffer == NULL ){
            printf( "<REPL_VAL> but NULL buffer, check that your client version is aligned to the server.\n" );
        }
        else
            gbc_print_buffer(&client.reply);
    }
	else if( client.reply.code == REPL_KVAL )
	{
        if( client.reply.buffer == NULL ){
            printf( "<REPL_KVAL> but NULL buffer, check that your client version is aligned to the server.\n" );
        }
        else {
            gbMultiBuffer mb;
            int i = 0;

            gb_reply_multi( &client, &mb );
            for( i = 0; i < mb.count; i++ ){
                printf( "%s => ", mb.keys[i] );
                gbc_print_buffer(&mb.values[i]);
            }

            gb_reply_multi_free(&mb);
        }
	}
}

#define gbc_op_args( s, fmt, n, op, ... ) if( sscanf( (s), (fmt), __VA_ARGS__ ) != (n) ){ \
											  printf( "ERROR: Invalid parameters, correct syntax is:\n\t%s\n", op_descriptions[(op)] ); \
											  return; \
									  	    }

void gbc_set_handler(char *input){
	char key[0xFF] = {0},
		 val[0xFF] = {0};
	long ttl;

	gbc_op_args( input, "%ld %s %s", 3, OP_SET, &ttl, key, val );

	if( gbc_connect() ){
		gb_set( &client, key, strlen(key), val, strlen(val), ttl );
		gbc_handle_response();
		gb_disconnect(&client);
	}
}

void gbc_mset_handler(char *input){
	char key[0xFF] = {0},
		 val[0xFF] = {0};

	gbc_op_args( input, "%s %s", 2, OP_MSET, key, val );

	if( gbc_connect() ){
		gb_mset( &client, key, strlen(key), val, strlen(val) );
		gbc_handle_response();
		gb_disconnect(&client);
	}
}

void gbc_ttl_handler(char *input){
	char key[0xFF] = {0};
	long ttl;

	gbc_op_args( input, "%s %ld", 2, OP_TTL, key, &ttl );

	if( gbc_connect() ){
		gb_ttl( &client, key, strlen(key), ttl );
		gbc_handle_response();
		gb_disconnect(&client);
	}
}

void gbc_mttl_handler(char *input){
	char key[0xFF] = {0};
	long ttl;

	gbc_op_args( input, "%s %ld", 2, OP_MTTL, key, &ttl );

	if( gbc_connect() ){
		gb_mttl( &client, key, strlen(key), ttl );
		gbc_handle_response();
		gb_disconnect(&client);
	}
}

void gbc_get_handler(char *input){
	char key[0xFF] = {0};

	gbc_op_args( input, "%s", 1, OP_GET, key );

	if( gbc_connect() ){
		gb_get( &client, key, strlen(key) );
		gbc_handle_response();
		gb_disconnect(&client);
	}
}

void gbc_mget_handler(char *input){
	char key[0xFF] = {0};

	gbc_op_args( input, "%s", 1, OP_MGET, key );

	if( gbc_connect() ){
		gb_mget( &client, key, strlen(key) );
		gbc_handle_response();
		gb_disconnect(&client);
	}
}

void gbc_del_handler(char *input){
	char key[0xFF] = {0};

	gbc_op_args( input, "%s", 1, OP_DEL, key );

	if( gbc_connect() ){
		gb_del( &client, key, strlen(key) );
		gbc_handle_response();
		gb_disconnect(&client);
	}
}

void gbc_mdel_handler(char *input){
	char key[0xFF] = {0};

	gbc_op_args( input, "%s", 1, OP_MDEL, key );

	if( gbc_connect() ){
		gb_mdel( &client, key, strlen(key) );
		gbc_handle_response();
		gb_disconnect(&client);
	}
}

void gbc_inc_handler(char *input){
	char key[0xFF] = {0};

	gbc_op_args( input, "%s", 1, OP_INC, key );

	if( gbc_connect() ){
		gb_inc( &client, key, strlen(key) );
		gbc_handle_response();
		gb_disconnect(&client);
	}
}

void gbc_minc_handler(char *input){
	char key[0xFF] = {0};

	gbc_op_args( input, "%s", 1, OP_MINC, key );

	if( gbc_connect() ){
		gb_minc( &client, key, strlen(key) );
		gbc_handle_response();
		gb_disconnect(&client);
	}
}

void gbc_mdec_handler(char *input){
	char key[0xFF] = {0};

	gbc_op_args( input, "%s", 1, OP_MDEC, key );

	if( gbc_connect() ){
		gb_mdec( &client, key, strlen(key) );
		gbc_handle_response();
		gb_disconnect(&client);
	}
}

void gbc_dec_handler(char *input){
	char key[0xFF] = {0};

	gbc_op_args( input, "%s", 1, OP_DEC, key );

	if( gbc_connect() ){
		gb_dec( &client, key, strlen(key) );
		gbc_handle_response();
		gb_disconnect(&client);
	}
}

void gbc_lock_handler(char *input){
	char key[0xFF] = {0};
	long time;

	gbc_op_args( input, "%s %ld", 2, OP_LOCK, key, &time );

	if( gbc_connect() ){
		gb_lock( &client, key, strlen(key), time );
		gbc_handle_response();
		gb_disconnect(&client);
	}
}

void gbc_mlock_handler(char *input){
	char key[0xFF] = {0};
	long time;

	gbc_op_args( input, "%s %ld", 2, OP_MLOCK, key, &time );

	if( gbc_connect() ){
		gb_mlock( &client, key, strlen(key), time );
		gbc_handle_response();
		gb_disconnect(&client);
	}
}

void gbc_unlock_handler(char *input){
	char key[0xFF] = {0};

	gbc_op_args( input, "%s", 1, OP_UNLOCK, key );

	if( gbc_connect() ){
		gb_unlock( &client, key, strlen(key) );
		gbc_handle_response();
		gb_disconnect(&client);
	}
}

void gbc_munlock_handler(char *input){
	char key[0xFF] = {0};

	gbc_op_args( input, "%s", 1, OP_MUNLOCK, key );

	if( gbc_connect() ){
		gb_munlock( &client, key, strlen(key) );
		gbc_handle_response();
		gb_disconnect(&client);
	}
}

void gbc_count_handler(char *input){
	char key[0xFF] = {0};

	gbc_op_args( input, "%s", 1, OP_COUNT, key );

	if( gbc_connect() ){
		gb_count( &client, key, strlen(key) );
		gbc_handle_response();
		gb_disconnect(&client);
	}
}

void gbc_meta_handler(char *input){
	char key[0xFF] = {0},
		 val[0xFF] = {0};

	gbc_op_args( input, "%s %s", 2, OP_META, key, val );

	if( gbc_connect() ){
		gb_meta( &client, key, strlen(key), val, strlen(val) );
		gbc_handle_response();
		gb_disconnect(&client);
	}
}

void gbc_keys_handler(char *input){
	char key[0xFF] = {0};

	gbc_op_args( input, "%s", 1, OP_KEYS, key );

	if( gbc_connect() ){
		gb_keys( &client, key, strlen(key) );
		gbc_handle_response();
		gb_disconnect(&client);
	}
}


void gbc_stats_handler(char *input){
	if( gbc_connect() ){
		gb_stats( &client );
		gbc_handle_response();
		gb_disconnect(&client);
	}
}

void gbc_ping_handler(char *input){
	if( gbc_connect() ){
		gb_ping( &client );
		gbc_handle_response();
		gb_disconnect(&client);
	}
}

static struct gbc_op_handler op_handlers[] = {
	{ "set", gbc_set_handler },
	{ "mset", gbc_mset_handler },
	{ "ttl", gbc_ttl_handler },
	{ "mttl", gbc_mttl_handler },
	{ "get", gbc_get_handler },
	{ "mget", gbc_mget_handler },
	{ "del", gbc_del_handler },
	{ "mdel", gbc_mdel_handler },
	{ "inc", gbc_inc_handler },
	{ "minc", gbc_minc_handler },
	{ "mdec", gbc_mdec_handler },
	{ "dec", gbc_dec_handler },
	{ "lock", gbc_lock_handler },
	{ "mlock", gbc_mlock_handler },
	{ "unlock", gbc_unlock_handler },
	{ "munlock", gbc_munlock_handler },
	{ "count", gbc_count_handler },
	{ "stats", gbc_stats_handler },
	{ "ping", gbc_ping_handler },
    { "meta", gbc_meta_handler },
    { "keys", gbc_keys_handler },
	{ NULL, NULL }
};

static void gbc_parse_exec( char *line ){
	struct gbc_op_handler *handler = &op_handlers[0];
	size_t i = 0;
	char *p = NULL;

	while( handler->op != NULL ){
		if( strncasecmp( handler->op, line, strlen(handler->op) ) == 0 ){
			p = line + strlen(handler->op);
			while( isspace(*p) ) ++p;

			handler->handler( p );
			return;
		}
		handler = &op_handlers[++i];
	}

	printf( "? Unknown command\n" );
}

int main( int argc, char **argv )
{
	gbc_commandline(argc, argv);

	if( *address == 0x00 && *unixsocket == 0x00 )
		gbc_help(argv,1);

	home = getenv("HOME");

	sprintf( history, "%s%s.gibson_history", home ? home : "", home ? "/" : "" );

	linenoiseHistoryLoad(history);

	printf( "\ntype :help or :h for a list of commands, :quit or :q to quit.\n\n" );

	char prompt[0xFF] = {0}, *input = NULL;

	if( *address )
		sprintf( prompt, "%s:%d> ", address, port );
	else
		sprintf( prompt, "local> " );

	while(1){
		input = linenoise( prompt );
		if( input != NULL ){
			if( strcmp( input, ":quit" ) == 0 || strcmp( input, ":q" ) == 0 ){
				exit(0);
			}
			else if( strcmp( input, ":help" ) == 0 || strcmp( input, ":h" ) == 0 ){
				int i;
				for( i = OP_SET; i <= OP_META; ++i ){
					printf( "\t%s\n", op_descriptions[i] );
				}
			}
			else if( *input != 0x00 ){
				linenoiseHistoryAdd(input);
				linenoiseHistorySave(history);

				gbc_parse_exec(input);
			}

			free(input);
		}
	}

	return 0;
}

