/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/*
  This is an interface to the user information (metadata).
*/

#include "rods.h"
#include "rodsClient.h"
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"
#include "irods_user_info.hpp"

#include "irods_query.hpp"
#include <boost/format.hpp>

int debug = 0;

rcComm_t *Conn;
rodsEnv myEnv;

void usage();

int
showUser(const std::string& name) {
    try {
        printf("%s", irods::get_printable_user_info_string(Conn, name).c_str());
    } catch(const irods::exception& e) {
        printf("%s", e.client_display_what());
        return e.code();
    }
    return 0;
}

int
main( int argc, char **argv ) {

    signal( SIGPIPE, SIG_IGN );

    int status, nArgs;
    rErrMsg_t errMsg;

    rodsArguments_t myRodsArgs{};

    rodsLogLevel( LOG_ERROR );

    status = parseCmdLineOpt( argc, argv, "vVh", 0, &myRodsArgs );
    if ( status ) {
        printf( "Use -h for help.\n" );
        exit( 1 );
    }
    if ( myRodsArgs.help == True ) {
        usage();
        exit( 0 );
    }

    status = getRodsEnv( &myEnv );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "main: getRodsEnv error. status = %d",
                 status );
        exit( 1 );
    }

    // =-=-=-=-=-=-=-
    // initialize pluggable api table
    irods::api_entry_table&  api_tbl = irods::get_client_api_table();
    irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    init_api_table( api_tbl, pk_tbl );

    Conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
                      myEnv.rodsZone, 0, &errMsg );

    if ( Conn == NULL ) {
        exit( 2 );
    }

    status = clientLogin( Conn );
    if ( status != 0 ) {
        if ( !debug ) {
            exit( 3 );
        }
    }

    nArgs = argc - myRodsArgs.optind;

    if (nArgs > 0) {
        status = showUser(std::string{argv[myRodsArgs.optind]});
    }
    else {
        status = showUser((boost::format("%s#%s") %
                          myEnv.rodsUserName % myEnv.rodsZone).str());
    }

    printErrorStack( Conn->rError );
    rcDisconnect( Conn );

    exit( status );
}

/*
Print the main usage/help information.
 */
void usage() {
    char *msgs[] = {
        "Usage: iuserinfo [-vVh] [user]",
        " ",
        "Show information about your iRODS user account or the entered user",
        ""
    };
    int i;
    for ( i = 0;; i++ ) {
        if ( strlen( msgs[i] ) == 0 ) {
            break;
        }
        printf( "%s\n", msgs[i] );
    }
    printReleaseInfo( "iuserinfo" );
}
