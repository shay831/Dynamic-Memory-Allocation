import sys
import os
import time
import matplotlib as plt

# associativity range
assoc_range = [2]
# block size range
bsize_range = [b for b in range(6, 7)]
# capacity range
cap_range = [c for c in range(9, 23)]
# number of cores
cores = [1]
# coherence protocol
protocol = 'none'

expname = 'exp2'
figname= 'graph2.png'

def get_stats():
    for line in open(logfile):
        if line[2:].startswith(key):
            line = line.split()
            return float(line[1])
    return 0

def run_exp(logfile, core, cap, bsize, assoc):
    """
    Function that runs experiment and records results into logfile
    """
    trace = 'trace.%dt.long.text' % core
    cmd="./p5 -t %s -p %s -n %d -cache %d %d %d >> %s" % (
            trace, protocol, core, cap, bsize, assoc, logfile)
    print(cmd)
    os.system(cmd)

def graph():
    # Get time
    timestr = time.strftime("%m.%d-%H_%M_%S")
    # Set path to save graph
    folder = "results/"+expname+"/"+timestr+"/"
    os.system("mkdir -p "+folder)

    # ask during office hours: how to plot both write thru and write backs 
    # code only gives hit rates
    traffic = {a:[] for a in assoc_range}

    for a in assoc_range:
        for b in bsize_range:
            for c in cap_range:
                for d in cores:
                    logfile = folder + "%s-%02d-%02d-%02d-%02d.out" % (
                            protocol, d, c, b, a)
                    run_exp(logfile, d, c, b, a)
                    miss_rate[a].append(get_stats(logfile, 'traffic')/100)

    plots = []
    for a in traffic:
        p,=plt.plot([2**i for i in cap_range], miss_rate[a])
        plots.append(p)

    plt.legend(plots, ['assoc %d' % a for a in assoc_range])
    plt.xscale('log', base=2)
    plt.title("Graph #2: Bus Writes vs Cache Size")
    plt.xlabel('Capacity')
    plt.ylabel("Bus Writes")
    plt.savefig(figname)
    plt.show()

graph()