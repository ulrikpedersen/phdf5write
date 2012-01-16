/*
 * server.hpp
 *
 *  Created on: 6 Jan 2012
 *      Author: up45
 */

#ifndef SERVER_HPP_
#define SERVER_HPP_

#include <adtransferserver.h>

class Server: public AdTransferServer {
public:
    Server(int port):AdTransferServer(port),port(port){};
    virtual ~Server(){};

    // signals
    //virtual void signal_socket_open(int status);
    //virtual void signal_client_connect(int status);
    //virtual void signal_waiting_for_frame();
    //virtual void signal_got_frame(NDArray *frame);
    //virtual void signal_sending_response();
    //virtual void signal_completed_response(int status);

private:
    int port;
    virtual const char * classname() {return "Server";}; // derived classes override this
};


#endif /* SERVER_HPP_ */
