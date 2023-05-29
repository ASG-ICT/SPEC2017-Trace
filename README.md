# Physical Memory Access traces of SPEC2017
We use the HMTT(http://asg.ict.ac.cn/hmtt/) to collect 42 physical memory access trace files of the SPEC2017 and the related PTE(page table entry) update information. 

At the same time, we use TopMC(https://github.com/ASG-ICT/TopMC) to collect 4 kinds of program feature information(L2_RQSTS.DEMAND_DATA_RD_ HIT[Event num:24,Umask:41], LLC Reference[Event num:2E,Umask:4F],Unhalted reference cycles[Event num:3C,Umask:01],Instruction retired[Event num:C0,Umask 00]) every 1 second.

Finally,we use the Simpoint3.0(http://cseweb.ucsd.edu/~calder/simpoint/) to analyze the characteristics of the trace and provide a fragment of the entire trace.

# Examples of using trace

```
tar -xzvf hmtt_trace/548-topmc-cut.trace.tar.gz
tar -xzvf kernel_trace/548-topmc.kt.tar.gz
make
./sim 548-topmc-cut.trace 548-topmc.kt 999999999999 0
```



# Trace Format
## HMTT trace
The size of each trace is 6 bytes.

![](https://github.com/ASG-ICT/SPEC2017-Trace/blob/main/pic/hmtt-trace.png)

Seq_num: the sequence of eache trace. It should be between 0 and 255 and the sequence of adjacent traces should be continuous.Because we split the complete trace into many segments, the seq is continuous within the segments, but not continuous between the segments.

Duration: the memory clock cycle interval between two traces. If it is 0 and both Read/Write and Phy_addr are 0, it means that the cycle interval is 256 full.

Read/Write: Read and write mark bit, 0 means write, 1 means read.

Phy_addr: 31-bit raw physical address. In some cases there is a certain offset from the system physical address.

```
seq_no = (unsigned int) ((tmp >> 40) & 0xffU);
timer  = (unsigned long long)((tmp >> 32) & 0xffULL);
r_w    = (unsigned int)((tmp >> 31) & 0x1U);
paddr   = (unsigned long)(tmp & 0x7fffffffUL);
paddr   = (unsigned long)(paddr << 6);
if (paddr >= (2ULL << 30)) {
    paddr += (2ULL << 30);
}
```

## Kernel trace
The size of each kernel trace is 13 bytes.

[magic][pid][virtual page number(vpn)][pysical page number(ppn)]  ---- [1 Byte][4 Bytes][4 Bytes][4 Bytes]

magic represents the type of trace. When magic equals 0xec, it represents set PTE kernel trace. When magic equals 0xfc、0X22 or 0X33, it represents free PTE kernel trace.

# Contact us
If you need more details about spec2017 trace, please contact this email:lutianyue@ict.ac.cn

