/*
 * profiling.h
 *
 *  Created on: 13 Feb 2012
 *      Author: up45
 */

#ifndef PROFILING_H_
#define PROFILING_H_

#include <iostream>
#include <vector>
#include <time.h>

class Profiling {
public:
    Profiling(int frame_bytes = 0);
    Profiling(const Profiling& src);
    ~Profiling(){};
    Profiling& operator=(const Profiling& src);
    /** Stream operator: use to prints a string representation of this class */
    inline friend std::ostream& operator<<(std::ostream& out, Profiling& pf) {
        out << pf._str();
        return out;
    }
    std::string _str();  /** Return a string representation of the object */

    void reset(int frame_bytes = -1);
    double stamp_now();
    void dt_start(); /* to start measure a certain code section */
    void dt_set_startstamp(timespec& startstamp);
    double dt_end(); /* end of code section measure */
    int count();

    timespec get_start() { return this->start; };
    std::vector<double> vec_timestamps();
    const double* ptr_timestamps(size_t& nelements);
    std::vector<double> vec_deltatime();
    std::vector<double> vec_datarate();
    double tsdiff (timespec& start, timespec& end);

private:
    void _copy(const Profiling& src);
    timespec start;
    double framesize;
    double datasize;
    std::vector<double> timestamps;
    std::vector<double> datarate;
};

#endif /* PROFILING_H_ */
