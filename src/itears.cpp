#include "dataObjOpen.h"
#include "dataObjWrite.h"
#include "dataObjClose.h"
#include "parseCommandLine.h"
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"
#include "irods_parse_command_line_options.hpp"
#include "irods_random.hpp"
#include "rodsClient.h"
#include "putUtil.h"

#include <cstring>
#include <iostream>
#include <string_view>

#define COUNT 10

namespace {
    
    int open_write_close(
        rcComm_t& _comm,
        const rodsPathInp_t& _rods_path,
        dataObjInp_t& _inp)
    {
        std::string_view target_path = _rods_path.destPath->outPath;

        std::memset(_inp.objPath, 0, sizeof(_inp.objPath));
        snprintf(_inp.objPath, sizeof(_inp.objPath), "%s.%s",
            target_path.data(),
            std::to_string(irods::getRandom<unsigned int>()).c_str());
        std::cout << "opening [" << _inp.objPath << "]" <<  std::endl;

        _inp.openFlags = O_CREAT | O_RDWR | O_TRUNC;

        // open
        auto idx = rcDataObjOpen(&_comm, &_inp);
        if (idx < 0) {
            std::cout << "open returned error" << std::endl;
            return idx;
        }

        // write
        openedDataObjInp_t w_inp{};

        w_inp.l1descInx = idx;

        const std::string contents = std::to_string(irods::getRandom<unsigned int>());

        bytesBuf_t buf{};
        buf.len = contents.size();
        buf.buf = (void*)contents.data();

        if (int status = rcDataObjWrite(&_comm, &w_inp, &buf); status < 0) {
            std::cout << "write returned error" << std::endl;
            return status;
        }

        // close
        if (int status = rcDataObjClose(&_comm, &w_inp); status < 0) {
            std::cout << "close returned error" << std::endl;
            return status;
        }

        return 0;
    } // open_write_close

}

void usage() {
    const char * const msgs[] = {
        "Usage: ihelp [-ah] [icommand]",
        "Display iCommands synopsis or a particular iCommand help text",
        "Options are:",
        " -h  this help",
        " -a  print the help text for all the iCommands",
        " ",
        "Run with no options to display a synopsis of the iCommands"
    };

    for ( unsigned int i = 0; i < sizeof( msgs ) / sizeof( msgs[0] ); ++i ) {
        printf( "%s\n", msgs[i] );
    }
    printReleaseInfo( "ihelp" );
}

int main(int argc, char** argv)
{

    signal( SIGPIPE, SIG_IGN );
 
    rodsArguments_t myRodsArgs;
    rodsPathInp_t rodsPathInp;

    rodsEnv myEnv{};
    int status = getRodsEnv( &myEnv );
    if ( status < 0 ) {
        return status;
    }

    int p_err = parse_opts_and_paths(
        argc, argv, myRodsArgs, &myEnv, UNKNOWN_FILE_T, UNKNOWN_OBJ_T, 0, &rodsPathInp );
    if ( p_err < 0 ) {
        usage();
        return EXIT_FAILURE;
    }
    else if ( myRodsArgs.help ) {
        usage();
        return EXIT_SUCCESS;
    }

    irods::api_entry_table&  api_tbl = irods::get_client_api_table();
    irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    init_api_table( api_tbl, pk_tbl );

    rErrMsg_t errMsg{};
    rcComm_t* comm = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName, myEnv.rodsZone, 0, &errMsg );

    if ( comm == NULL ) {
        return 2;
    }

    status = clientLogin( comm );
    if ( status != 0 ) {
        rcDisconnect( comm );
        return 7;
    }

    dataObjInp_t o_inp{};
    initCondForPut(comm, &myEnv, &myRodsArgs, &o_inp, nullptr, nullptr);

    const auto owc = [&]() {
        int status = open_write_close(*comm, rodsPathInp, o_inp);
        if (status < 0) {
            std::cout << "failed to open/write/close [" << rodsPathInp.srcPath->outPath << "] to [" << rodsPathInp.destPath->outPath << "] with [" << status << "]" << std::endl;
        }
        return status;
    };

    // open/write/close a file
    for (int i = 0; i < COUNT; i++) {
        owc();
    }

    printErrorStack( comm->rError );
    rcDisconnect( comm );
   
    return 0;
}

