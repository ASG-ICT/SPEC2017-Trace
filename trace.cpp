#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include "trace.h"

#define KERNEL_TAG_SIZE 16
#define MAXPPN ((64ULL << 30) >> 12)

using namespace std;

unsigned long long trace_id = 0;
#define VADDR


struct record_t record;

struct opt_record_t {
    unsigned int rw;
    unsigned int pblk;
    unsigned int vblk;
    unsigned int next_tm;
    unsigned long tm;
};
unsigned long long last_none_pte = 0;
#define USE_HUGEPAGE

unsigned int pidindex[100000] = {0};
int pid_len = 0;
unsigned long long pid_trace_num[100000][2] = {0};  //0 for pid trace 

#define KT_BUFFER_SIZE (13ULL << 20)
char kernel_trace_buffer[KT_BUFFER_SIZE + 100];
int kernel_trace_offset = 0;
int kernel_trace_len = 0;
unsigned long long has_read = 0;

char kt_ch[200] = {0};
int read_len;

FILE *ktfp = NULL;

//prepare for the next trace
int read_kt(){
	has_read += 13;
        return fread(kt_ch, 1, 13, ktfp);
}        



#ifndef VADDR
int find_index(unsigned int pid){
	int i;
	for(i = 0; i < pid_len ; i++){
		if(pidindex[i] == pid){
			return i;
		}	
	}
	return -1;
}
#endif

char *tag;
uint32_t tagp;
uint64_t tag_size = 0;

uint64_t duration_all = 0;
unsigned long long free_pte_num = 0;

uint64_t set_pte_cnt;
uint64_t free_pte_cnt;
uint64_t set_pte_tag_cnt;
uint64_t free_pte_tag_cnt;
uint64_t dump_set_pte_cnt;
uint64_t dump_free_pte_cnt;

int show_sync_tag_num = 0;

FILE *tracefp = NULL;
//FILE *ofp = NULL;
unsigned long *ppn2vpn = NULL;
int *ppn2pid = NULL;
char data[9] = {0};
unsigned long  long total_trace = 0;
unsigned long long nonpte = 0;
int tag_end = 0;
uint64_t tag_cnt = 0;
unsigned long *next_page_map;
unsigned long *prev_page_map;
unsigned long *alloc_stamp;
int addr_type = 2;
map<unsigned long, unsigned long> vpn2ppn;
unsigned long long skip_free_pte = 0;
unsigned long long skip_set_pt_pmd = 0;


unsigned long long sync_trace_num = 0;

void error(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(-1);
}

int next_record(FILE *fp)
{
    uint64_t trace_buf;
    uint64_t temp;
    uint64_t duration;
    unsigned long long       tmp;
    unsigned int seq_no,r_w;
        unsigned long paddr;
        unsigned long long timer;

    //uint64_t seq_num;
    uint64_t invalid_count = 0;
    while (1) {
        if (fread(&trace_buf, 6, 1, fp) == 0){ 
		printf("read hmtt trace end or error\n");
		return -1;
	}
	tmp = trace_buf;
        seq_no = (unsigned int) ((tmp >> 40) & 0xffU);
        timer  = (unsigned long long)((tmp >> 32) & 0xffULL);
        r_w    = (unsigned int)((tmp >> 31) & 0x1U);
        paddr   = (unsigned long)(tmp & 0x7fffffffUL);
        paddr   = (unsigned long)(paddr << 6);
	record.paddr = paddr;
	record.rw = r_w;
	duration = timer;

        if (record.paddr == 0 && duration == 0) {
            invalid_count++;
            continue;
        }
        else {
            duration += invalid_count * 256;
            invalid_count = 0;
        }
        duration_all += duration;
        record.tm = duration_all;
        break;
    }

//    record.paddr = record.paddr << 6;
    if (record.paddr >= (2ULL << 30)) {
        record.paddr += (2ULL << 30);
    }
//    printf(" seq_no = %d ,r_w = %d, paddr = %lx  \n", seq_no, r_w, paddr);
    //record.vaddr = record.paddr;
    return 0;
}

int next_tag(FILE *fp)
{
    int res = fread(tag, sizeof(char), KERNEL_TAG_SIZE, fp);
    if (res == 0) {
        fprintf(stderr, "start kt_begin");
        return 0;
    }
    return 1;
}

//#define cfg_start 0x00000680000000ULL
//#define cfg_end (0x00000600000000ULL + (1ULL << 15))

#define MEMNUM                  (0x500000000ULL)  // forddr3 20G
//#define MEMNUM                  (0xA00000000ULL)  // forddr4 40G
#define cfg_end                 (MEMNUM + (1ULL << 15))  // for ddr3
#define KERNEL_TRACE_ENTRY_ADDR ((cfg_end >> 20) + 32ULL + 32ULL)
#define KERNEL_TRACE_CONFIG_ENTRY_ADDR ((cfg_end >> 20) + 64 + 5248ULL)
unsigned long long kernel_config_entry = KERNEL_TRACE_CONFIG_ENTRY_ADDR << 20;
#define MALLOC_TAG_ENTRY_ADDR ((cfg_end >> 20) + 64 + 5120ULL)
unsigned long long malloc_tag_addr = MALLOC_TAG_ENTRY_ADDR << 20;
int topmc_tag = 0;




#define KERNEL_TRACE_CONFIG_SIZE (1LLU)
#define KERNEL_TRACE_SEQ_NUM     (64LLU)
unsigned long long kernel_config_end =
        (KERNEL_TRACE_CONFIG_ENTRY_ADDR + KERNEL_TRACE_CONFIG_SIZE * KERNEL_TRACE_SEQ_NUM) << 20;
#define TAG_ACCESS_SIZE (4096LLU)
#define TAG_ACCESS_STEP (2048LLU)   //in Byte,
#define TAG_ACCESS_TIMES (2U)
#define TAG_MAX_POS (256U)

#define SET_PTE_TAG                     0
#define FREE_PTE_TAG                    1

#define DUMP_PAGE_TABLE_TAG             2
#define KERNEL_TRACE_END_TAG            3
#define SET_PT_ADDR_TAG                 5
#define FREE_PT_ADDR_TAG                6

char set_pt_addr_magic      = 0xed;

    char free_pt_addr_magic     = 0xfd;

char set_page_table_magic   = 0xec;
char free_page_table_magic  = 0xfc;
char free_page_table_get_clear = 0x22;
char free_page_table_get_clear_full = 0x33;

unsigned long long set_after_dump = 0, free_after_dump = 0;
unsigned long long miss_free_pte = 0;
unsigned long long miss_set_pte = 0;



int is_kernel_tag_trace(unsigned long long addr) {
    return addr >= kernel_config_entry && addr < kernel_config_end;
}

int next_translate()
{
    unsigned int pid;
    unsigned long ppn;
    unsigned long vpn;
    uint64_t val;
    char magic;


    while (1) {
        if (next_record(tracefp) != 0) {
            return 0;
        }
        total_trace += 1;
        trace_id ++;
        uint64_t addr = record.paddr;
        if(addr == malloc_tag_addr + 64){
//             printf("***************** trace_id = %lld seq_no = %d ,r_w = %d, paddr = %lx,  last_seq_no = %d \n",trace_id, seq_no, r_w, paddr, last_seq);
             topmc_tag ++;
        }



        if (is_kernel_tag_trace(addr) && tag_end == 0) {
            tag_cnt += 1;
            // is sync tag
            int kernel_trace_seq = (addr - kernel_config_entry) / (KERNEL_TRACE_CONFIG_SIZE << 20);
            if (kernel_trace_seq >= KERNEL_TRACE_SEQ_NUM) {
                fprintf(stderr, "#### Invalid kernel trace seq:addr=0x%llx, seq=%d\n", addr, kernel_trace_seq);
                exit(-1);
            }

            unsigned long long kernel_trace_seq_entry =
                    addr - kernel_config_entry - kernel_trace_seq * (KERNEL_TRACE_CONFIG_SIZE << 20);

            int kernel_trace_tag = (kernel_trace_seq_entry) / TAG_ACCESS_SIZE;
            if (kernel_trace_tag >= TAG_MAX_POS) {
                fprintf(stderr, "#### Invalid kernel_trace_tag:addr=0x%llx,tag=%d\n", addr, kernel_trace_tag);
                exit(-1);
            }
            int hmtt_kt_type = ((kernel_trace_seq_entry) % TAG_ACCESS_SIZE) / TAG_ACCESS_STEP;
            if (hmtt_kt_type > 1) {
                fprintf(stderr, "#### Invalid hmtt_kt_type:addr=0x%llx,tag=%d,type=%d\n", addr, kernel_trace_tag,
                        hmtt_kt_type);
                exit(-1);
            } else if (hmtt_kt_type == 1)
                continue;

            if (tagp + 13 >= tag_size) {
		printf("trace_id= %llu  tagp = %lu ,in tagp error\n",trace_id, tagp);
//		trace_finish();
//		return 1;		
                error("tag pos");
            }



	    if(show_sync_tag_num > 0){
                                printf("real_trace_id = %lld  ,r_w = %d, paddr = %lx,  sync_tag_num = %d \n",trace_id,  record.rw, record.paddr, tag_cnt);
                                show_sync_tag_num --;
                        }
	    if(kernel_trace_tag == FREE_PTE_TAG)
		free_pte_num ++;;
	    
//	    getchar();
//	    printf("tagp =%llu, has_read = %llu \n", tagp,has_read);
//	    char ttt_ch = (*(char*)(kt_ch));
  ///          printf("magic in KT, %x in KT\n", ttt_ch);
//	    getchar();

            switch (kernel_trace_tag) {
                case SET_PTE_TAG:
                    set_pte_cnt += 1;
		    magic =  (*(char*)(kt_ch));
		    if(magic ==  '$' ){
			miss_set_pte ++;
			break;
		    }



		    if(magic != set_page_table_magic){

			printf("set_pte in hmmt, %x in KT\n",magic);
			printf("tagp =%llu, has_read = %llu \n", tagp,has_read);
			error("bug\n");
			if(magic == free_page_table_magic || magic == free_page_table_get_clear || magic == free_page_table_get_clear_full){
//				printf("set_pte_cnt = %llu, free_pte_cnt = %llu \n", set_pte_cnt ,free_pte_cnt);
				read_len = read_kt();	
				tagp +=13;
				break;
			}
//			printf("set_pte miss match!\n");
			break;
//			while(magic == free_pt_addr_magic)	
//			{
//				printf("SET_PTE in HMTT meet free_pt_addr_magic in KT");
//				tagp += 13;
//				magic = (*(char*)(&(tag[tagp])));  //skip 
//				trace_finish();
//				error("set_page_table offset of hmtt & KT not equal");
//			}
		    }
                    tagp += 1;
		    pid = (*(int*)((kt_ch + 1)));
                    tagp += 4;
		    val = (*(uint64_t*)(kt_ch + 5));
                    ppn = val & 0xffffff;
                    vpn = (val >> 24) & 0xffffffffff;
                    tagp += 8;

		    read_len = read_kt(); 

                    if (ppn >= MAXPPN) {
                        error("invalid ppn");
                    }
                    ppn2pid[ppn] = pid;
                    ppn2vpn[ppn] = vpn;
                    vpn2ppn[vpn] = ppn;
                    if (vpn2ppn[vpn-1] > 0) {
                        unsigned long last_ppn = vpn2ppn[vpn - 1];
                        if (last_ppn != ppn - 1) {
                            next_page_map[last_ppn] = ppn;
                        }
                    }
                    else if (vpn2ppn[vpn - 2] > 0) {
                        unsigned long last_ppn = vpn2ppn[vpn - 2];
                        if (last_ppn != ppn - 2) {
                            next_page_map[last_ppn] = ppn;
                        }
                    }
                    alloc_stamp[ppn] = total_trace;
                    break;

                case FREE_PTE_TAG:
		    magic =  (*(char*)(kt_ch));
		    if(magic ==  '$' ){
                        miss_free_pte ++;
                        break;
                    }

		    if(magic != free_page_table_magic && magic != free_page_table_get_clear && magic != free_page_table_get_clear_full){
//			printf("miss match free tag in KT \n");
//		        printf("free_pte in hmmt, %x in KT\n",magic);
			skip_free_pte ++;
			miss_free_pte = miss_free_pte + 1;
			break;
		    }
                    free_pte_cnt += 1;
		    pid = (*(int*)((kt_ch + 1)));
        	    val = (*(uint64_t*)(kt_ch + 5));
                    ppn = val & 0xffffff;
                    vpn = (val >> 24) & 0xffffffffff;
                    tagp += 13;
		    read_len = read_kt();
                    if (ppn >= MAXPPN) {
                        error("invalid ppn");
                    }
                    ppn2pid[ppn] = 0;
                    ppn2vpn[ppn] = 0;
                    break;


                case DUMP_PAGE_TABLE_TAG:
                    break;
                case KERNEL_TRACE_END_TAG:
                    if (strncmp(kt_ch, "$$$$$$$$$$$$$", 13) == 0) {
			fread(kt_ch, 1, 3, ktfp);
			has_read += 3;
            		int tmp_l = read_kt();
                        printf("kernel trace end.\n");
                    } else {
                        printf("error: kernel trace end\n");
                    }
                    tag_end = 1;
                    break;
                default:;
                    fprintf(stdout, "UnIdentifitable kernel trace tag:addr=0x%llx, tag=%d, type=%d\n",
                            addr,
                            kernel_trace_tag,
                            hmtt_kt_type);
            }
        }
	else if(addr == malloc_tag_addr + 64){
		;
	}
	else {
            // normal trace
            uint64_t ppn = record.paddr >> 12;
	    uint64_t pmd_ppn =(ppn >> 8);
	     if (ppn2vpn[ppn] == 0 && ppn2pid[ppn] == 0) {
                nonpte += 1;
                record.pid = -1;
                record.vaddr = 0;
            } else {
#ifndef VADDR
		if(find_index( ppn2pid[ppn] ) == -1 )
		{
			pidindex[ pid_len] = ppn2pid[ppn];
			pid_len ++;
		}
		pid_trace_num[ find_index( ppn2pid[ppn] ) ][0] ++;
#endif
                record.pid = ppn2pid[ppn];
                record.vaddr = (ppn2vpn[ppn] << 12) | (record.paddr & 0xfff);
            }
            //fwrite(&record, sizeof(record_t), 1, ofp);
            return 1;
        }
    }
}



char* read_kernel_trace(){

	if(kernel_trace_offset >= KT_BUFFER_SIZE)
	{
		fread(kernel_trace_buffer, 1, KT_BUFFER_SIZE, ktfp) ;//read again
		kernel_trace_offset = 0;
	}
	else{
		kernel_trace_offset += 13;
	}
	return kernel_trace_buffer + kernel_trace_offset; 
}




int trace_init(char *tracefile, char *ktfile)
{
    kernel_trace_offset = 0;
    kernel_trace_len = 0;
    memset(kernel_trace_buffer, 0 , sizeof(char) * 1024);

    tracefp = fopen(tracefile, "rb");
    ktfp = fopen(ktfile, "rb");
    //ofp = fopen(argv[3], "wb");
    ppn2pid = new int[MAXPPN];
    ppn2vpn = new uint64_t[MAXPPN];


    //vpn2ppn = new uint64_t[MAXPPN];
    next_page_map = new unsigned long[MAXPPN];
    prev_page_map = new unsigned long[MAXPPN];
    memset(ppn2pid, 0, sizeof(int) * MAXPPN);
    memset(ppn2vpn, 0, sizeof(uint64_t) * MAXPPN);

    //memset(vpn2ppn, 0, sizeof(uint64_t) * MAXPPN);
    memset(next_page_map, 0, sizeof(unsigned long) * MAXPPN);
    memset(prev_page_map, 0, sizeof(unsigned long) * MAXPPN);

    alloc_stamp = new unsigned long[MAXPPN];
    memset(alloc_stamp, 0, sizeof(unsigned long) * MAXPPN);
    memset(kt_ch, 0, sizeof(char) * 200);

    fseek(ktfp, 0, SEEK_END);
    tag_size = ftell(ktfp);
    printf("tag_size = %llu\n",tag_size);
    fseek(ktfp, 0, SEEK_SET);
//    tag = (char*)malloc(sizeof(char) * tag_size);
//    fread(tag, 1, tag_size, ktfp);
    //first read
    int tmp_l = read_kt();

    tagp = 0; 
    printf("start trace_init\n");
    printf("%s\n",kt_ch);
    if(strncmp(kt_ch , "@@", 2) == 0)
	printf("yes\n");

	int tmp_i = 0;
    unsigned int pid;
    unsigned long ppn;
    unsigned long vpn;
    uint64_t val;
    unsigned long long h_set = 0, h_free = 0,k_set = 0,k_free = 0;

    int ret_len = 0;
    while (tagp + 16 < tag_size) {
        if (strncmp(kt_ch , "@@@@@@@@@@@@@", 13) == 0) {
            tagp += 16;
	    fread(kt_ch, 1, 3, ktfp);
	    has_read += 3;
            printf("start collect trace  flag, tagp = %lu, has_read = %llu\n", tagp, has_read);
	    ret_len = read_kt();
            break;
        }
        tagp += 13;
	ret_len = read_kt();
    }
    while (tagp + 16 < tag_size) {
        if (strncmp(kt_ch , "#############", 13) == 0) {
            tagp += 16;
	    fread(kt_ch, 1, 3, ktfp);
	    has_read += 3;
	    printf("start dump page talbe flag, tagp = %lu, has_read = %llu\n", tagp, has_read);
	    ret_len = read_kt();
	    printf("ret_len = %d\n",ret_len);
	    char ttt_ch = (*(char*)(kt_ch));
	    printf("magic in KT, %x in KT\n", ttt_ch);

            break;
        }
        tagp += 13;
	ret_len = read_kt();
    }
    unsigned long long dump_pte_trace = 0;
    char free_pt_addr_magic     = 0xfd;
    char set_page_table_magic   = 0xec;
    char free_page_table_magic  = 0xfc;
    unsigned long long set_pt_num = 0,free_pt_num = 0, set_page_table_num = 0, free_page_table_num = 0;


    while(tagp + 16 < tag_size) {
        if (strncmp(kt_ch, "&&&&&&&&&&&&&", 13) == 0) {
            tagp += 16;
	    fread(kt_ch, 1, 3, ktfp);
	    has_read += 3;
	    printf("tagp =%llu, has_read = %llu \n", tagp,has_read);
	    ret_len = read_kt();
	    printf("ret_len = %d\n",ret_len);
	    char ttt_ch = (*(char*)(kt_ch));
	    printf("magic in KT, %x in KT\n", ttt_ch);
	    printf("end dump page talbe flag, ,dump_pte_trace = %llu, tagp = %lu\n",dump_pte_trace,tagp);
	    printf("h_set = %llu , h_free = %llu\n",h_set,h_free);
	    printf("set_pt_num = %llu,free_pt_num = %llu, set_page_table_num = %llu, free_page_table_num = %llu\n",set_pt_num,free_pt_num, set_page_table_num, free_page_table_num);
            break;
        }
	char tmp_magic =  (*(char*)(kt_ch));
	if(tmp_magic == set_page_table_magic){
		set_page_table_num ++;

	}
	else if(tmp_magic == free_page_table_magic){
		free_page_table_num ++;
	}
	else {printf("****************************error !!!\n");}


        pid = (*(int*)((kt_ch + 1)));;
	val = (*(uint64_t*)(kt_ch + 5));
        ppn = val & 0xffffff;
        vpn = (val >> 24) & 0xffffffffff;
        ppn2pid[ppn] = pid;
        ppn2vpn[ppn] = vpn;
        vpn2ppn[vpn] = ppn;
        if (vpn2ppn[vpn-1] > 0) {
            unsigned long last_ppn = vpn2ppn[vpn - 1];
            if (last_ppn != ppn - 1) {
                next_page_map[last_ppn] = ppn;
            }
        }
        else if (vpn2ppn[vpn - 2] > 0) {
            unsigned long last_ppn = vpn2ppn[vpn - 2];
            if (last_ppn != ppn - 2) {
                next_page_map[last_ppn] = ppn;
            }
        }
        //printf("setpte: %llu %d %llx %lx %lx\n",
                //total_trace, pid, val, vpn, ppn);
        tagp += 13;
	ret_len = read_kt();
        dump_pte_trace ++;
        alloc_stamp[ppn] = total_trace;
    }   

  /*
    while(tagp + 16 < tag_size) {
        if (strncmp(&(tag[tagp]), "$$$$$$$$$$$$$$$$", 16) == 0) {
            tagp += 16;
            printf("end collect trace, ,dump_pte_trace = %llu, tagp = %lu\n",dump_pte_trace,tagp);
            printf("h_set = %llu , h_free = %llu\n",h_set,h_free);
            printf("set_pt_num = %llu,free_pt_num = %llu, set_page_table_num = %llu, free_page_table_num = %llu\n",set_pt_num,free_pt_num, set_page_table_num, free_page_table_num);
            break;
        }
        char tmp_magic =  (*(char*)(&(tag[tagp])));
        if(tmp_magic == set_pt_addr_magic){
                set_pt_num ++;
        }
        else if(tmp_magic == free_pt_addr_magic){
                free_pt_num ++;
        }
        else if(tmp_magic == set_page_table_magic){
                set_page_table_num ++;
        }
        else if(tmp_magic == free_page_table_magic){
                free_page_table_num ++;
        }
        else {printf("****************************error !!!\n");}
	tagp +=13;
    }
*/
    unsigned long long tmp_sync = 0;

    while (1) {
        if (next_record(tracefp) != 0) {
	    printf("finish in trace_init\n");
            break;
        }
        uint64_t addr = record.paddr;
        if (is_kernel_tag_trace(addr)) {
            // is sync tag
            int kernel_trace_seq = (addr - kernel_config_entry) / (KERNEL_TRACE_CONFIG_SIZE << 20);
            if (kernel_trace_seq >= KERNEL_TRACE_SEQ_NUM) {
                fprintf(stderr, "#### Invalid kernel trace seq:addr=0x%llx, seq=%d\n", addr, kernel_trace_seq);
                exit(-1);
            }

            tmp_sync ++;

            unsigned long long kernel_trace_seq_entry =
                    addr - kernel_config_entry - kernel_trace_seq * (KERNEL_TRACE_CONFIG_SIZE << 20);

            int kernel_trace_tag = (kernel_trace_seq_entry) / TAG_ACCESS_SIZE;
            if (kernel_trace_tag >= TAG_MAX_POS) {
                fprintf(stderr, "#### Invalid kernel_trace_tag:addr=0x%llx,tag=%d\n", addr, kernel_trace_tag);
                exit(-1);
            }
            int hmtt_kt_type = ((kernel_trace_seq_entry) % TAG_ACCESS_SIZE) / TAG_ACCESS_STEP;
            if (hmtt_kt_type > 1) {
                fprintf(stderr, "#### Invalid hmtt_kt_type:addr=0x%llx,tag=%d,type=%d\n", addr, kernel_trace_tag,
                        hmtt_kt_type);
                exit(-1);
            } else if (hmtt_kt_type == 1);


            if(kernel_trace_tag == SET_PTE_TAG){
                    h_set ++;
            }
            else if(kernel_trace_tag == FREE_PTE_TAG){
	            h_free ++;
            }

	

            if (kernel_trace_tag == DUMP_PAGE_TABLE_TAG) {
		printf("find DUMP_PAGE_TABLE_TAG in hmtt trace, trace_idd = %llu\n",trace_id);
		
	        printf("h_set = %llu , h_free = %llu , temp_sync = %llu\n",h_set,h_free, tmp_sync);
                break;
            }
        }
    }

    printf("dump page done.\n");
    printf("trace init done.\n");

    return 0;
}

void trace_finish()
{
    printf(" set_pte_cnt = %llu,free_pte_cnt = %llu, set_pte_tag_cnt = %llu,free_pte_tag_cnt = %llu, dump_set_pte_cnt = %llu,dump_free_pte_cnt = %llu\n", set_pte_cnt,free_pte_cnt, set_pte_tag_cnt,  free_pte_tag_cnt, dump_set_pte_cnt, dump_free_pte_cnt);
    printf("ALL_nonpte = %llu\n",nonpte);
    printf("total trace = %llu,  trace_id = %llu ,tag_cnt = %llu, tagp = %lu \n",total_trace, trace_id,tag_cnt, tagp);
    printf("free pte_num = %llu\n",free_pte_num);
    printf("miss_free_pte = %llu,  miss_set_pte = %llu\n",miss_free_pte, miss_set_pte); 
    printf("skip_set_pt_pmd = %llu\n",skip_set_pt_pmd);
    printf("topmc tag [%d]\n",topmc_tag);
    int i;

#ifndef VADDR 
    for(i = 0 ; i < pid_len ; i++){
	printf("pid = %lu , trace_num = %llu\n", pidindex[i], pid_trace_num[i]);
    }
#endif

    delete next_page_map;
    delete prev_page_map;
    delete ppn2vpn;
//    delete vpn2ppn;
    delete ppn2pid;
    fclose(tracefp);
    fclose(ktfp);
    return;
}
