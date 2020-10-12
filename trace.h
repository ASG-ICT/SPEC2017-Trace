#ifndef TRACE_H
#define TRACE_H
#include <map>
struct record_t {
    unsigned int rw;
    int pid;
    unsigned long tm; 
    unsigned long paddr;
    unsigned long long vaddr;
};
int trace_init(char *tracefile, char *ktfile);
void trace_finish();
int next_translate();
extern struct record_t record;
extern unsigned long *next_page_map;
extern unsigned long *prev_page_map;
extern std::map<unsigned long, unsigned long> vpn2ppn;
extern unsigned long *ppn2vpn;
extern int addr_type;
extern unsigned long *alloc_stamp;
extern unsigned long long  total_trace;
extern unsigned long long  nonpte;
extern unsigned long long trace_id;
extern unsigned long long miss_free_pte;
extern unsigned long long skip_set_pt_pmd;

#endif
