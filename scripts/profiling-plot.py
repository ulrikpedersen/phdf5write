#!/bin/env dls-python

import os, sys
from pkg_resources import require
require("matplotlib")
require("numpy")
require("cothread")

import numpy
from cothread.catools import *

from pylab import *
import h5py

#TSNAME="/entry/instrument/performance/timestamp"
TSNAME="/profiling"
DSETNAME="/entry/detector/data"
# [proc, field, step]

def smooth(x,window_len=11,window='hanning'):
    """smooth the data using a window with requested size.
    
    This method is based on the convolution of a scaled window with the signal.
    The signal is prepared by introducing reflected copies of the signal 
    (with the window size) in both ends so that transient parts are minimized
    in the begining and end part of the output signal.
    
    This is stolen from a SciPy cookbook recipe: http://wiki.scipy.org/Cookbook/SignalSmooth
    """

    if x.ndim != 1:
        raise ValueError, "smooth only accepts 1 dimension arrays."
    if x.size < window_len:
        raise ValueError, "Input vector needs to be bigger than window size."
    if window_len<3:
        return x
    if not window in ['flat', 'hanning', 'hamming', 'bartlett', 'blackman']:
        raise ValueError, "Window is on of 'flat', 'hanning', 'hamming', 'bartlett', 'blackman'"
    s=numpy.r_[x[window_len-1:0:-1],x,x[-1:-window_len:-1]]
    if window == 'flat': #moving average
        w=numpy.ones(window_len,'d')
    else:
        w=eval('numpy.'+window+'(window_len)')
    y=numpy.convolve(w/w.sum(),s,mode='valid')
    return y

# Indeices to the /profiling dataset columns
TIMESTAMP      = 0
DT_TIMESTAMP   = 1
WRITETIME      = 2
DT_TIME_RATE   = 3
WRITETIME_RATE = 4

def timestamps_xy(filename, performance, column=WRITETIME):
    plots = [] # A list of plots in the form [x,y1, x,y2, x,y3 ... ]
    x = arange(len(performance[0,0,1:]))
    processes = range(len(performance[:,0,0]))
    for proc in processes:
        y = performance[proc,column,1:]
        plots += [x,y]
    return (["process%s"% str(proc) for proc in processes], plots)

def timestamps(filename, performance, column=DT_TIMESTAMP):
    plots = [] # A list of plots in the form [x,y1, x,y2, x,y3 ... ]
    processes = range(len(performance[:,0,0]))
    for proc in processes:
        y = performance[proc,column,1:]
        plots += [y]
    return (["process%s"% str(proc) for proc in processes], plots)
    

def main():
    if len(sys.argv) < 2:
        print "No input file specified. Attempting to use caget to get latest file from Excalibur"
        #sys.exit(-1)
        path=caget("BL13J-EA-EXCBR-01:CONFIG:PHDF:FilePath_RBV", datatype=DBR_CHAR_STR)
        name=caget("BL13J-EA-EXCBR-01:CONFIG:PHDF:FileName_RBV", datatype=DBR_CHAR_STR)
        number=caget("BL13J-EA-EXCBR-01:CONFIG:PHDF:FileNumber")
        templ=caget("BL13J-EA-EXCBR-01:CONFIG:PHDF:FileTemplate_RBV", datatype=DBR_CHAR_STR)
        fname = templ%(path,name,number)
    else:
        fname = sys.argv[1]

    print "Opening file: %s" % str(fname)    
    hdffile = h5py.File(fname, 'r')
    performance = hdffile[TSNAME]
    dataset = hdffile[DSETNAME]
    print "dataset dims: ", dataset.shape
    framesize = dataset.shape[1] * dataset.shape[2] * 2.0 / (1024. * 1024.)
    process_framesize = framesize/len(performance[:,0,0])
    last_timestamps = performance[:, 0, -1]
    
    print "Overall avg datarate: ", framesize * dataset.shape[0] / max(last_timestamps)

    figure(1)
    title(fname)
    
    ### First plot is the period time ####
    #subplot(211)
    #result = timestamps_xy( fname, performance )
    
    # now for the plotting
    #plotArgs = result[1]
    #plot( *plotArgs, linestyle='None', marker='.' )
    #legend( result[0], shadow = True, loc = (0.1, 0.5) )
    #for l in gca().get_legend().get_texts():
    #    setp(l, fontsize = 12)

    #ylabel('period time [s]')
    #xlabel('frame')
    #hold(False)
    #grid(True)

    ### Second plot is the avg datarate based on period time ####
    #subplot(212)
    #result = timestamps_xy( fname, performance, column=DT_TIME_RATE )
    #plotArgs = result[1]
    #plot( *plotArgs )
    #ylabel('data rate [MB/s]')
    #xlabel('frame')
    #hold(False)
    #grid(True)


    #### Second window figure #####
    #figure(2)
    # First plot is the smoothed period time.
    # smoothing for 2s using a hanning window.
    #subplot(211)
    legends, result = timestamps( fname, performance, column=DT_TIMESTAMP )
    plotArgs = []
    smoothed_periods = []
    frequency = 1/result[0].mean()
    window_time = 10.0
    window_length = window_time * frequency
    print "Smoothing window lenght: %.1fs, %i frames "%(window_time, window_length)
    for period_time in result:
        print "Mean: ", period_time.mean(), "s ", process_framesize / period_time.mean(), "MB/s"
        niceline = smooth(period_time, window='hanning', window_len=window_length)
        plotArgs += [arange(len(niceline))]
        plotArgs += [ niceline ]
        smoothed_periods += [niceline]
    #plot( *plotArgs )
    #legend( legends, shadow = True )
    #for l in gca().get_legend().get_texts():
    #    setp(l, fontsize = 12)
    #ylabel('period time [s]')
    #xlabel('frame')
    #hold(False)
    #grid(True)

    # Second plot is datarate using the smoothed period time.
    #subplot(212)
    plotArgs=[]
    for smoothed_period in smoothed_periods:
        plotArgs += [arange(len(smoothed_period))]
        plotArgs += [ process_framesize / smoothed_period ]
    plot( *plotArgs )
    legend( legends, shadow = True )
    for l in gca().get_legend().get_texts():
        setp(l, fontsize = 12)
    ylabel('period time [s]')
    ylabel('data rate [MB/s]')
    xlabel('frame')
    hold(False)
    grid(True)

    hdffile.close()
    # finally show (and block)
    show()

if __name__=="__main__":
    main()
    
