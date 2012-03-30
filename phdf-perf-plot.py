#!/bin/env dls-python2.6

import os, sys
from pkg_resources import require
require("matplotlib")

from pylab import *
import h5py

#TSNAME="/entry/instrument/performance/timestamp"
TSNAME="/profiling"
DSETNAME="/mydset"

def timestamps(filename):
    hdffile = h5py.File(filename, 'r')

    performance = hdffile[TSNAME]
    dataset = hdffile[DSETNAME]
    print "dataset dims: ", dataset.shape
    framesize = dataset.shape[1] * dataset.shape[2] * 2.0 / (1024. * 1024.)
    #framesize = dataset.shape[0] * dataset.shape[1] * 2.0 / (1024. * 1024.)
    for proc in range(len(performance[:,0,0])):
        x = performance[proc,0,1:]
        y1 = performance[proc,1,1:]
        y2 = performance[proc,2,1:]
        print "timestamp mean: %.3f dt mean: %.3f"%( y1.mean(), y2.mean())
        #framesize = 4096 * 1024 * 2 * 4.0 / (1024. * 1024)
        print "size = %.1fMB rate: %.1f (%.1f) MB/s"%( framesize, framesize/y1.mean(), framesize/y2.mean() )
    x = arange(len(y1))+1
    hdffile.close()
    #print x,y1
    return (["period", "dt"], [x,y1,x,y2])
    
def allprocess(filename):
    hdffile = h5py.File(filename, 'r')

    performance = hdffile[TSNAME]
    dataset = hdffile[DSETNAME]
    print "dataset dims: ", dataset.shape
    framesize = dataset.shape[0] * dataset.shape[1] * 2.0 / (1024. * 1024.)
    result = ([],[])
    for proc in range(len(performance[:,0,0])):
        y1 = performance[proc,1,1:]
        y2 = performance[proc,2,1:]
        #x = performance[proc,0,1:]
        x = arange(len(y1))+1
        result[0].append("proc%d"%proc)
        result[1].append(x)
        result[1].append(y1)
    return result
    
def writesteps(filename):
    hdffile = h5py.File(filename, 'r')

    performance = hdffile[TSNAME]
    dataset = hdffile[DSETNAME]
    result = ([],[])
    for step in range(len(performance[0,5:,0]) ):
        perf = performance[0, step+5, 1:]
        if step == 0:
            y = perf
        else:
            y = perf - latch_perf
        latch_perf = perf
        x = arange(len(y))+1
        result[0].append("step%d"%step)
        result[1].append(x)
        result[1].append(y)
        
    result[0].append("period")
    result[1].append( x )
    result[1].append( performance[0,1,1:] )
    return result
        

    
def datarate(filename):
    hdffile = h5py.File(filename, 'r')

    performance = hdffile[TSNAME]

    x = performance[0,0,:]
    y1 = performance[0,3,:]
    y2 = performance[0,4,:]
    
    
    hdffile.close()
    return (["period", "dt"], [x,y1,x,y2])


def multiplefiles(files):
    result = ([],[])
    for filename in files:
        hdffile = h5py.File(filename, 'r')
        performance = hdffile[TSNAME]
        x = performance[:,0]
        y = performance[:,3]
        hdffile.close()
        result[0].append( filename )
        result[1].append(x)
        result[1].append(y)
    return result

def main():
    if len(sys.argv) < 2:
        print "ERROR: not enough arguments. Specify HDF5 file as input arg."
        sys.exit(-1)
        
    result = ([],[])
    
    fname = sys.argv[1]

    figure(1)
    
    ### First plot is datarate ####
    subplot(411)
    result = datarate( fname )
    
    # now for the plotting
    plotArgs = result[1]
    
    plot( *plotArgs )
    #legend( result[0], shadow = True, loc = (0.4, 0.02) )
    legend( result[0], shadow = True )
    for l in gca().get_legend().get_texts():
        setp(l, fontsize = 12)

    ylabel('data rate [MB/s]')
    xlabel('time [s]')
    hold(False)
    grid(True)
    
    
    ### Second plot is timestamps ####
    subplot(412)
    result = timestamps( fname )
    
    # now for the plotting
    plotArgs = result[1]
    
    plot( *plotArgs )
    #legend( result[0], shadow = True, loc = (0.4, 0.02) )
    legend( result[0], shadow = True )
    for l in gca().get_legend().get_texts():
        setp(l, fontsize = 12)

    ylabel('write time [s]')
    xlabel('frame')
    hold(False)
    grid(True)

    # Third plot contain all timestamps
    subplot(413)
    result = allprocess(fname)
    # now for the plotting
    plotArgs = result[1]
    
    plot( *plotArgs )
    #legend( result[0], shadow = True, loc = (0.4, 0.02) )
    legend( result[0], shadow = True )
    for l in gca().get_legend().get_texts():
        setp(l, fontsize = 12)

    ylabel('write time [s]')
    xlabel('frame')
    hold(False)
    grid(True)
    
    # fourth plot contain all timestamps
    subplot(414)
    result = writesteps(fname)
    # now for the plotting
    plotArgs = result[1]
    
    plot( *plotArgs )
    #legend( result[0], shadow = True, loc = (0.4, 0.02) )
    legend( result[0], shadow = True )
    for l in gca().get_legend().get_texts():
        setp(l, fontsize = 12)

    ylabel('step time [s]')
    xlabel('frame')
    hold(False)
    grid(True)

    # finally show (and block)
    show()

if __name__=="__main__":
    main()
    
