################################################################################
# README                                                                       #
#                                                                              #
# Description: This file serves as a README and documentation for CP1          #
#              WHOHAS flooding and IHAVE responses.                            #
#                                                                              #
# Author: Yurui Zhou<yuruiz@andrew.cmu.edu>, Pan Sun<pans@andrew.cmu.edu>      #
#                                                                              #
################################################################################




[TOC-1] Table of Contents
--------------------------------------------------------------------------------

        [TOC-1] Table of Contents
        [DES-2] Description of Files
        [RUN-3] How to Run



[DES-2] Description of Files
--------------------------------------------------------------------------------

Here is a listing of extra source code file:

chunklist.c                      - manage chunk list.
request.c                        - build packets of different type and send it out.

[RUN-3] How to Run
--------------------------------------------------------------------------------
1. 'make' on one andrew machine.

2. Set up the spiffy.

3. run following command line. 
   ./peer -p ./nodes.map -c ./example/A.haschunks -f ./example/C.masterchunks -m 4 -i  
 
4. Then, enter the following command.
    GET example/B.chunks example/newB.tar
5. you will see the sent chunk hash. 

[DESING-4] Design
--------------------------------------------------------------------------------
WHOHAS:
In situation of too many required chunks, split the long packet into small ones
to fit with the 1500 length.
IHAVE:
keep the original offset of chunks in received packet.
