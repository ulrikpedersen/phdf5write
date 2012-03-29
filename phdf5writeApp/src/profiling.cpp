/*
 * profiling.cpp
 *
 *  Created on: 13 Feb 2012
 *      Author: up45
 */
#include <iostream>
#include <sstream>

using namespace std;

#include "profiling.h"

Profiling::Profiling(int frame_bytes)
{
    double mbytes = -1.0;
    if (frame_bytes > 0) mbytes = (double)frame_bytes/(1024. * 1024.);
    this->framesize = 0.0;
    this->start.tv_nsec = 0;
    this->start.tv_sec = 0;
    if (mbytes > 0.0) this->framesize = mbytes;
    this->datasize = 0.0;
    this->reset(frame_bytes);
}

Profiling::Profiling(const Profiling& src)
{
    this->_copy(src);
}

Profiling& Profiling::operator =(const Profiling& src)
{
    // Check for self-assignment
    if (this == &src)
        return *this;
    this->_copy(src);
    return *this;
}

string Profiling::_str()
{
    stringstream out(stringstream::out);
    out << "<Profiling  nstamps=" << this->timestamps.size();
    out << " size=" << this->framesize << "MB";
    vector<double>::const_iterator it=this->timestamps.begin();
    vector<double> delta = this->vec_deltatime();
    vector<double>::const_iterator dit = delta.begin();
    for (int i=0;
            it != this->timestamps.end();
            ++it, ++dit, i++)
    {
        //if (i%4 == 0) out << "\n\t";
        out << "\n\t" << *it << "s\t" << *dit << "s";
    }
    out << "\n/Profiling>";
    return out.str();
}

void Profiling::reset(int frame_bytes)
{
    double frame_mbytes = 0.0;
    if (frame_bytes > 0) {
        frame_mbytes = (double)frame_bytes/(1024. * 1024.);
    }
    this->framesize = frame_mbytes;
    this->datasize = 0.0;
    this->datarate.clear();
    this->timestamps.clear();
    clock_gettime( CLOCK_REALTIME, &this->start);
}

double Profiling::stamp_now()
{
    timespec now;
    clock_gettime( CLOCK_REALTIME, &now);

    double stampdiff = this->tsdiff(this->start, now);
    this->timestamps.push_back(stampdiff);

    // Calculate the datarate in MB/s (MegaByte per second)
    this->datasize += this->framesize;
    double rate = 0.0;
    if (stampdiff == 0.0) rate = 0.0;
    else rate = this->datasize/stampdiff;
    this->datarate.push_back(rate);

    return stampdiff;
}

void Profiling::dt_start()
{
    clock_gettime( CLOCK_REALTIME, &this->start);
    this->datasize = 0.0;
}

void Profiling::dt_set_startstamp(timespec& startstamp)
{
    this->start = startstamp;
    this->datasize = 0.0;
}

double Profiling::dt_end()
{
    return this->stamp_now();
}

int Profiling::count()
{
    return (int)this->timestamps.size();
}

vector<double> Profiling::vec_timestamps()
{
    return this->timestamps;
}

const double *Profiling::ptr_timestamps(size_t& nelements)
{
    if (nelements > this->timestamps.size()) nelements = this->timestamps.size();
    return &this->timestamps.front();
}

vector<double> Profiling::vec_deltatime()
{
    vector<double> delta;
    vector<double>::const_iterator it = this->timestamps.begin();
    double prev = 0.0;

    for (it = this->timestamps.begin();
            it != this->timestamps.end();
            ++it)
    {
        delta.push_back( *it - prev );
        //cout << *it << " - " << prev << " = " << *it-prev << endl;
        prev = *it;
    }
    return delta;
}

vector<double> Profiling::vec_datarate()
{
    return this->datarate;
}

/* ============ Private Methods of the Profiling Class ======================== */

void Profiling::_copy(const Profiling& src)
{
    this->start = src.start;
    this->framesize = src.framesize;
    this->timestamps = src.timestamps;
    this->datasize = src.datasize;
    this->datarate = src.datarate;
}

double Profiling::tsdiff (timespec& start, timespec& end)
{
    timespec result;
    double retval = 0.0;

    /* Perform the carry for the later subtraction by updating */
    if ((end.tv_nsec-start.tv_nsec)<0) {
        result.tv_sec = end.tv_sec-start.tv_sec-1;
        result.tv_nsec = (long int)(1E9) + end.tv_nsec-start.tv_nsec;
    } else {
        result.tv_sec = end.tv_sec-start.tv_sec;
        result.tv_nsec = end.tv_nsec-start.tv_nsec;
    }

    retval = result.tv_sec;
    retval += (double)result.tv_nsec / (double)1E9;

    return retval;
}
