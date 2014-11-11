/*
 * server.cpp
 *
 *  Created on: 6 Jan 2012
 *      Author: up45
 */

#include <cstdlib>
#include <iostream>

#include <osiSock.h>

#include "ByteBuffer2.h"
#include "PVDataTiny.h"
#include "PrintHandler.h"
#include "Channel.h"
#include "server.hpp"

using namespace std;

void Server::signal_socket_open(int status)
{
    LOG4CXX_INFO(log, "signal_socket_open()");
}

void Server::signal_client_connect(int status)
{
    LOG4CXX_INFO(log, "signal_client_connect()");
}


void Server::signal_waiting_for_frame()
{
    LOG4CXX_TRACE(log, "signal_waiting_for_frame()");
}

int Server::signal_open_cmd(std::string& filename, int mode)
{
    int retcode = 0;
    LOG4CXX_INFO(log, "Open command received:"
                      << "\n\t\t\t\t\t|_ Receive buf: " << this->recv_buffer
                      << "\n\t\t\t\t\t  |_ Open file: " << filename);
    if (this->recv_buffer != NULL) {
        //this->recv_buffer->report(11);
        if (this->hdf) this->hdf->h5_configure(*this->recv_buffer);
    }

    if (this->hdf) retcode = this->hdf->h5_open(filename.c_str());
    if (retcode < 0) {
        LOG4CXX_INFO(log, "Failed to open file");
    }
    return retcode;
}

int Server::signal_close_cmd()
{
    int retcode = 0;
    LOG4CXX_TRACE(log, "signal_close_cmd()");
    if (this->hdf) retcode = this->hdf->h5_close();
    if (retcode < 0) {
        LOG4CXX_WARN(log, "Problem closing file");
    }
    return retcode;
}

int Server::signal_reset_cmd()
{
    int retcode = 0;
    LOG4CXX_TRACE(log, "signal_reset_cmd()");
    if (this->hdf) retcode = this->hdf->h5_reset();
    if (retcode < 0) {
        LOG4CXX_WARN(log, "Problem resetting hdf5 library");
    }
    return retcode;
}


int Server::signal_got_frame(NDArray *frame)
{
    int retcode = 0;
    int close_retcode=0;
    LOG4CXX_TRACE(log, "signal_got_frame()");
    if (frame == NULL) {
    	LOG4CXX_WARN(log, "Received empty NDArray");
    	return 0;
    }
    if (this->hdf) retcode = this->hdf->h5_write(*frame);
    if (retcode < 0) {
        LOG4CXX_WARN(log, "Problem writing frame. Closing and aborting write.");
        // In case of any error during a write, we close the file in order to
        // reset the state of the writer.
        close_retcode = this->hdf->h5_close();
        if (close_retcode < 0) {
            LOG4CXX_WARN(log, "Problem closing file");
        }
    }
    return retcode;
}


void Server::signal_sending_response()
{
    LOG4CXX_TRACE(log, "signal_sending_response()");
}


void Server::signal_completed_response(int status)
{
    LOG4CXX_TRACE(log, "signal_completed_response(" << status << ")");
}


