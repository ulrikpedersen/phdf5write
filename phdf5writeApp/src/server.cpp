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
    this->msg("signal_socket_open()");
}

void Server::signal_client_connect(int status)
{
    this->msg("signal_client_connect()");
}


void Server::signal_waiting_for_frame()
{
    this->msg("signal_waiting_for_frame()");
}

void Server::signal_open_cmd(std::string& filename, int mode)
{
    int retcode = 0;
    this->msg("signal_open_cmd()");
    cout <<   "                 |_ Receive buf: " << this->recv_buffer << endl;
    cout <<   "                  |_ Open file: " << filename << endl;
    if (this->recv_buffer != NULL) {
        this->recv_buffer->report(11);
        if (this->hdf) this->hdf->h5_configure(*this->recv_buffer);
    }

    if (this->hdf) retcode = this->hdf->h5_open(filename.c_str());
    if (retcode < 0) {
        this->msg("Failed to open file");
    }
}

void Server::signal_close_cmd()
{
    int retcode = 0;
    this->msg("signal_close_cmd()");
    if (this->hdf) retcode = this->hdf->h5_close();
    if (retcode < 0) {
        this->msg("Problem closing file");
    }
}


void Server::signal_got_frame(NDArray *frame)
{
    int retcode = 0;
    this->msg("signal_got_frame()");
    if (frame == NULL) return;
    if (this->hdf) retcode = this->hdf->h5_write(*frame);
    if (retcode < 0) {
        this->msg("Problem writing frame");
    }
}


void Server::signal_sending_response()
{
    this->msg("signal_sending_response()");
}


void Server::signal_completed_response(int status)
{
    this->msg("signal_completed_response()");
}


