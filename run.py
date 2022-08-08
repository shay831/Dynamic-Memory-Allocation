#!/usr/bin/python3                                                                                                                    

import time
import os

os.system("mkdir -p results")

exp_name = "exp"
os.system("mkdir -p results/"+exp_name)
timestr = time.strftime("%m.%d-%H_%M_%S")
folder = "results/"+exp_name+"/"+timestr
os.system("mkdir "+folder)

cap_lo = 8;
cap_high = 23; # up to but not including                                                                                              
bsize_lo = 6;
bsize_high = 7; # up to but not including                                                                                             
assoc = [1]

for i in range(cap_lo, cap_high):
    for j in assoc:
        for k in range(bsize_lo, bsize_high):
            os.system("./p5 -t trace.1t.long.txt -cache "+str(i)+" "+str(k)+" "+str(j)+" >> "+folder+"/"+str(i).zfill(2)+"_"+str(k).zfill(2)+"_"+str(j)+".out")