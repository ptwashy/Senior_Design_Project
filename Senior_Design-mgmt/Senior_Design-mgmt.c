/*
 * Senior_Design-mgmt
 * ------------------
 *
 * Code written by Philip Washy for Dr. George Kusic's Senior Design Project.
 * Based very heavily upon the arduino-serial code produced by Tod E. Kurt as a
 * way to save a lot of time and effort. All credit for arduino-serial-lib.h
 * goes to Tod E. Kurt ( http://todbot.com/blog/ )
 *
 * Last updated:
 * 4/20/16
 */

#include <stdio.h>    // Standard input/output definitions
#include <stdlib.h>
#include <string.h>   // String function definitions
#include <unistd.h>   // for usleep()
#include <getopt.h>

#include "arduino-serial-lib.h"

#define MAX_BUFFER 256

void usage( void ) {
  printf( "Senor_Design-mgmt: sdm -b <baud> -p <port> [OPTIONS]\n"
          "\n"
          "Options:\n"
          "  -h, --help     Print this message\n"
          "  -b, --baud     Rate of Baud clock for serial communication\n"
          "  -p, --port     Serial port on which communication will occur\n"
          "  -s, --scale    Set the prescale value for calibration\n"
          "  -t, --timer    Get measurements on a timer from a given number\n"
          "                 in milliseconds\n"
          "  -r, --read     Return a single reading\n"
          "  -n, --rename   Rename output files\n"
          "\n" );
  exit( EXIT_SUCCESS );
}

void error( char * msg ) {
  fprintf( stderr, "%s\n", msg );
  exit( EXIT_FAILURE );
}

void read_value( int port, const char * filename ) {
    char buffer[ MAX_BUFFER ];
    int recieved;

	FILE * file = fopen( filename, "a+" );
    recieved = serialport_write( port, "a" );
    if( recieved == -1 ) error( "Write Error" );
    serialport_read_until( port, buffer, '\012', MAX_BUFFER, 1500000 );
    fprintf( file, "%s\n", buffer );
    fclose( file );
}

int main( int argc, char * argv[] ) {
  char port_name[ MAX_BUFFER ];
  char buffer   [ MAX_BUFFER ];
  char filename [ MAX_BUFFER ];

  int     port       = -1;
  int     baud       = 9600;
  uint8_t loop       = 0;
  int     timeout    = 5000;
  int     end        = 0;
  int     recieved;
  int     count;

  int option_index = 0;
  int current_option;
  static struct option option_list[] = {
    { "help",   no_argument,       0, 'h' },
    { "baud",   required_argument, 0, 'b' },
    { "port",   required_argument, 0, 'p' },
    { "rename", required_argument, 0, 'n' },
    { "scale",  required_argument, 0, 's' },
    { "timer",  required_argument, 0, 't' },
    { "read",   no_argument,       0, 'r' },
    { NULL,     0,                 0, 0 }
  };

  if( argc == 1 ) {
    usage();
  }
  strcpy( filename, "Pi1.dat");

  while( 1 ) {
    current_option = getopt_long( argc, argv,
                                  "hb:p:n:s:t:r",
                                  option_list,
                                  &option_index );
    if( current_option == -1 ) break;

    switch( current_option ) {
      case '0':
        break;
      case 'h':
        usage();
        break;
      case 'b':
        baud = strtol( optarg, NULL, 10 );
        break;
      case 'p': {
        if( port != -1 ) {
          serialport_close( port );
          printf( "Closed port %s\n", port_name );
        } //if( port != -1 )
        strcpy( port_name, optarg );
        port = serialport_init( optarg, baud );
        if( port == -1 ) error( "Coudln't Open Port" );
        printf( "Opened port %s\n", optarg );
        serialport_flush( port );
      } break;
      case 'n':
        strcpy( filename, optarg );
        break;
      case 's': {
        if( port == -1 ) error( "No Port Opened" );
        recieved = serialport_write( port, "c" );
        sprintf( buffer, "%s", optarg );
        recieved = serialport_write( port, buffer );
        if( recieved == -1 ) error( "Write Error" );
      } break;
      case 't': {
        if( port == -1 ) error( "No Port Opened" );
        count = strtol( optarg, NULL, 10 );
        while( count > 0 ) {
          read_value( port, filename );
          usleep( 1500000 );
          count -= 2;
        }//while( count > 0 )
      } break;
      case 'r': {
        if( port == -1 ) error( "No Port Opened" );
        read_value( port, filename );
      } break;
    } //switch( current_option )
  } //while( 1 )
}
