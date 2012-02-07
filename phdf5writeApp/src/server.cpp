/*
 * server.cpp
 *
 *  Created on: 6 Jan 2012
 *      Author: up45
 */

#include <cstdlib>
#include <iostream>

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


void Server::signal_got_frame(NDArray *frame)
{
    this->msg("signal_got_frame()");
}


void Server::signal_sending_response()
{
    this->msg("signal_sending_response()");
}


void Server::signal_completed_response(int status)
{
    this->msg("signal_completed_response()");
}


