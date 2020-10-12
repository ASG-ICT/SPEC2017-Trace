#include "trace.h"
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    if (argc != 5) {
        printf("./sim tracefile ktfile simn skipn\n");
        return 0;
    }
    printf("%s\n", argv[1]);
    trace_init(argv[1], argv[2]);
    unsigned long long i;
    unsigned long long simn = atol(argv[3]);
    unsigned long long skipn = atol(argv[4]);
    printf("start skip!\n");
    for (i = 0; i < skipn; i++) {
        int res = next_translate();
        if (res == 0) {
            printf("\n");
	    printf("res= %d\n",res);
	    break;
        }
    }
    printf("end skip!\n");
    unsigned valid = 0;
    printf("*******************************\n");

    //trace_finish();

    printf("*******************************\n");
    unsigned long long last_non = 0;
    unsigned long long last_miss_free_pte = 0, last_skip_set_pt_pmd = 0;
    nonpte = 0;
    trace_id = 0;
    miss_free_pte = 0;
    skip_set_pt_pmd = 0;
    for (i = 0; i < simn; i++) {
        int res = next_translate();
        if(i % 200000000 == 0){
		double tmp_d = nonpte - last_non;
	        
                printf("**<%llu> nonpte = %llu , percent = %lf , miss_free_pte = %llu, skip_set_pt_pmd = %llu\n", i / 200000000 ,nonpte - last_non ,  tmp_d /200000000.0, miss_free_pte - last_miss_free_pte , skip_set_pt_pmd - last_skip_set_pt_pmd);
                last_non = nonpte;
		last_miss_free_pte = miss_free_pte;
		last_skip_set_pt_pmd = skip_set_pt_pmd;
		miss_free_pte = 0;
            }




        if (res == 0) break;
        if (record.rw == 1) {
            valid++;
        }
        if (record.paddr >> 12 == 494597)
        printf("trace_py %d %lx %lx %lld\n",
            record.rw, record.paddr >> 6, record.vaddr >> 6, record.tm);
    }

    double tmp_d = nonpte - last_non;
                printf("**<%llu> nonpte = %llu , percent = %lf , miss_free_pte = %llu, skip_set_pt_pmd = %llu\n", i / 200000000 ,nonpte - last_non ,  tmp_d /200000000.0, miss_free_pte - last_miss_free_pte , skip_set_pt_pmd - last_skip_set_pt_pmd);


    printf("\n");
    fprintf(stderr, "\n\n%ld\n", nonpte);
    printf("trace_id = %llu\n",trace_id);
    trace_finish();

}
