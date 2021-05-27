# DRAM Request Manager for Multicore Processors
In this assignment we extended our earlier DRAM request manager to the multicore CPU case. Our architecture now consists of N CPU cores, each running a different MIPS program, and sending DRAM requests to a Memory Request Manager which interfaces with the DRAM. 
1. Extended our earlier MIPS simulator (with DRAM timings) to the multicore scenario. Our objective is to implement the Memory Request Manager in such a way that the instruction   throughput   (total   number   of   instructions   completed   by   the   whole system in a given period, say from Cycle 0 to Cycle M) is maximised.
2. Estimated   the   delay   (in   clock   cycles)   of   our    Memory   Request   Manager algorithm   and   incorporated   it   into   our   timing   model. 

