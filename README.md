# SPEC2017-Trace
We use the HMTT to collect 42 physical trace files of the SPEC2017 and the related PTE(page table entry) update information. At the same time, we use the Simpoint to analyze the characteristics of the trace and provide a fragment of the entire trace.

# Trace Format
## HMTT trace
The size of each trace is 6 bytes.


[Seq_num][Duration][Read/Write][Phy_addr] --- [8 bits][8 bits][1 bit][31 bits]

Seq_num represents the sequence of eache trace. It should be between 0 and 255 and the sequence of adjacent traces should be continuous.Because we split the complete trace into many segments, the seq is continuous within the segments, but not continuous between the segments.
Duration represents the memory clock cycle interval between two traces. If it is 0 and both Read/Write and Phy_addr are 0, it means that the cycle interval is 256 full.
r_w: 1-

