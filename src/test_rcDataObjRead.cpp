#include "rodsClient.h"
#include "dataObjRead.h"
#include "dataObjOpen.h"
#include "dataObjClose.h"
#include <utility>
#include "irods_at_scope_exit.hpp"
#include <iostream>

int main() {
    rodsEnv env;
    getRodsEnv(&env);
    rErrMsg_t errors;
    auto* comm = rcConnect(env.rodsHost, env.rodsPort, env.rodsUserName, env.rodsZone, 0, &errors);
    irods::at_scope_exit<std::function<void()>> at_scope_exit{[comm] {
            rcDisconnect(comm);
    }};
    char passwd[] = "rods";
    clientLoginWithPassword(comm, passwd);

    dataObjInp_t dataObjInp{};
    openedDataObjInp_t dataObjReadInp{};
    bytesBuf_t dataObjReadOutBBuf{};

    rstrcpy(dataObjInp.objPath, "/tempZone/home/rods/testfile", MAX_NAME_LEN);
    dataObjInp.openFlags = O_RDONLY;
    dataObjReadInp.l1descInx = rcDataObjOpen(comm, &dataObjInp);

    dataObjReadInp.len = 5;
    int bytes_read = rcDataObjRead(comm, &dataObjReadInp, &dataObjReadOutBBuf);
    std::cout << "bytes_read:" << bytes_read << std::endl;
    std::cout << "buf:" << static_cast<char*>(dataObjReadOutBBuf.buf) << std::endl;

    rcDataObjClose(comm, &dataObjReadInp);
}

