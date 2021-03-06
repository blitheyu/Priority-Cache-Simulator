#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include "RankCache.h"
#include "hist.h"
#include "priority.h"
#include <assert.h>

int TIME_FLAG = 0;
double tt_time =0;

uint64_t trace_processing(RankCache_t* cache, FILE* fd) {

	char *keyStr;
	uint64_t key;
	char *sizeStr;
	uint32_t size;
	char* ret;
	char   line[256];

	uint64_t totMiss = 0;
	///////////////////progress bar////////////////////////////
	char bar[28];
	int i;
	uint32_t d = 0;
	//first line contain total reference number
	bar[0] = '[';
	bar[26] = ']';
	bar[27]='\0';
	for (i=1; i<=25; i++) bar[i] = ' ';
	ret=fgets(line, 256, fd);
	keyStr = strtok(line, " ");
	keyStr = strtok(NULL, " ");
	uint64_t total = strtoull(keyStr, NULL, 10);
	uint64_t star = total/100;
	///////////////////////////////////////////////////////////
	i=1;
	while ((ret=fgets(line, 256, fd)) != NULL)
	{
		keyStr = strtok(line, ",");
		key = strtoull(keyStr, NULL, 10);
		sizeStr = strtok(NULL, ",");
		if (sizeStr != NULL)	
			size = strtoul(sizeStr, NULL, 10);
		else
			size = 1;
		struct timeval  tv1, tv2;
		gettimeofday(&tv1, NULL);

		if (access(cache, key, size) == CACHE_MISS)	totMiss++;
		
		gettimeofday(&tv2, NULL);
		tt_time += (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec);
		d++;
		
		if(d % star == 0)
		{	
			if( d%(star*4) == 0)
				bar[i++] = '#';
			printf("\rProgress: %s%d%% %d", bar, (int)(d/(double)total*100)+1, d);
			fflush(stdout);
		}
		
	}
	printf("\n");
	return totMiss;
}

int main(int argc, char const *argv[])
{
	// char buf[1024];


	char* input_format = "arg1: input filename\n \
arg2: Cache Size\n \
arg3: Sample Size\n \
arg4: policy(hc, lru, lfu, lhd)\n\
arg5: timer flag (1 yes 0 no)\n\
This runnable accept filtered trace, \"key, size\"\n\
If size is not provided by default use logical size.\n";

	if(argc < 6) { 
		printf("%s", input_format);
		return 0;
	}

	if(atoi(argv[5]) == 1) TIME_FLAG = 1;
	else if(atoi(argv[5]) == 0) TIME_FLAG = 0;
	else {
		printf("%s", input_format);
		exit(-1);
	}

	FILE*       rfd = NULL;
	if((rfd = fopen(argv[1],"r")) == NULL)
	{ perror("open error for read"); return -1; }

	struct timeval  tv1, tv2;
	gettimeofday(&tv1, NULL);

	RankCache_t *cache;
	if (strcmp(argv[4], "lhd") == 0){
		//cache init
		//hist init temporary
		// hitHist = histInit(128,1024*1024*10, 128);
		// lifeTimeHist = histInit(128,1024*1024*10, 128);
		// lhdHist = histInit(128,1024*1024*10, 128);
		cache = cacheInit(strtoul(argv[2], NULL, 10),
									   strtoul(argv[3], NULL, 10),
									   LHD_initPriority,
									   LHD_updatePriorityOnHit,
									   LHD_updatePriorityOnEvict,
									   LHD_minPriorityItem);
		LHD_globalDataInit(cache);
	}else if (strcmp(argv[4], "lru") == 0) {
		cache = cacheInit(strtoul(argv[2], NULL, 10),
								   strtoul(argv[3], NULL, 10),
								   LRU_initPriority,
								   LRU_updatePriorityOnHit,
								   LRU_updatePriorityOnEvict,
								   LRU_minPriorityItem);
	}else if (strcmp(argv[4], "mru") == 0) {
		cache = cacheInit(strtoul(argv[2], NULL, 10),
								   strtoul(argv[3], NULL, 10),
								   MRU_initPriority,
								   MRU_updatePriorityOnHit,
								   MRU_updatePriorityOnEvict,
								   MRU_minPriorityItem); 
	}else if (strcmp(argv[4], "lfu") == 0) {
		cache = cacheInit(strtoul(argv[2], NULL, 10),
								   strtoul(argv[3], NULL, 10),
								   LFU_initPriority,
								   LFU_updatePriorityOnHit,
								   LFU_updatePriorityOnEvict,
								   LFU_minPriorityItem);
	} else if (strcmp(argv[4], "hc") == 0) {
			cache = cacheInit(strtoul(argv[2], NULL, 10),
								   strtoul(argv[3], NULL, 10),
								   HC_initPriority,
								   HC_updatePriorityOnHit,
								   HC_updatePriorityOnEvict,
								   HC_minPriorityItem);
	}else {
		printf("%s", input_format);
		return 0;
	}
	


	gettimeofday(&tv2, NULL);
	tt_time += (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec);


	uint64_t totMiss = trace_processing(cache, rfd);
	// fprintf(stderr, "update %d\n", upcnt);
	fprintf(stderr,"%f, %f\n",cache->capacity, totMiss/(double)cache->totRef);

	if(TIME_FLAG == 1)
		printf ("Total time = %f seconds\n", tt_time);

	cacheFree(cache);

	return 0;
}

