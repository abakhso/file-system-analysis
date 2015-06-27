#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/md5.h>
#include "uthash.h"

int byte1024 = 0;
int byte2048 = 0;
int byte4096 = 0;
int allbyte = 0;
int MAX_SIZE = 1024;
int BlockSize ;
int wastedBytes = 0;

typedef struct fileOffset{
	char filename[256];
	int offset;
};


typedef struct hash{
	struct hash * next;
	unsigned char c[33];
	int count;
	struct fileOffset files[4096];
	UT_hash_handle hh;
};

struct hash * table = NULL;

struct hash * first = NULL;
struct hash * last = NULL;

void calculateHash( char * filename, int size){
    int i;
    unsigned char c[33];

    FILE *inFile = fopen (filename, "rb");
    int bytes;
    unsigned char data[size];

    if (inFile == NULL) {
        printf ("%s can't be opened.\n", filename);
        return ;
    }

    int k = 0;
    while ((bytes = fread (data, 1, size, inFile)) != 0){

        MD5_CTX mdContext;
    	MD5_Init (&mdContext);
        MD5_Update (&mdContext, data, bytes);
        MD5_Final (c,&mdContext);
      //  for(i = 0; i < MD5_DIGEST_LENGTH; i++) printf("%02x", c[i]);
      //  printf (" %s \n",filename);


    	struct hash *curr;
    	
    	if(first ==NULL){
    		curr = malloc(sizeof(struct hash));
    		first= curr;
    		last = first;
    		strcpy(first->c,c);
    		strcpy(first->files[0].filename,filename);
    		first->files[0].offset=0;
    		first->next = NULL;
    		first->count = 1;

    	} else {
    		int check =0;
    		for(curr=first; curr!=NULL; curr=curr->next){
    			if(strcmp(curr->c, c)==0){
    				strcpy(curr->files[curr->count].filename,filename);
    				curr->files[curr->count].offset=k;
   					curr->count++;
   					check = 1;
   					break;
    			}
    		}
    		if(check==0){
    			curr = malloc(sizeof(struct hash));
    			strcpy(curr->c,c);
    			strcpy(curr->files[0].filename, filename);
    			curr->files[0].offset=k;
    			curr->count = 1;
    			last->next=curr;
    			last=curr;
    		}
    	}
	k++;

    }
    fclose (inFile);


    

}


void findProgram(char *dir,  char* path)
{
    DIR *d;
    struct dirent *curr;
    struct stat sta;

    if((d = opendir(dir)) == NULL) {
        fprintf(stderr,"cannot open directory: %s\n", dir);
        return;
    }
    chdir(dir);
    while((curr = readdir(d)) != NULL) {
        lstat(curr->d_name,&sta);
        if(S_ISDIR(sta.st_mode)) {
            if(strcmp(".",curr->d_name) == 0 ||
                strcmp("..",curr->d_name) == 0)
                continue;
            path[strlen(path)+1] = '\0';
            path[strlen(path)] = '/';
            strcpy(path+strlen(path),curr->d_name);

            if(strcmp(curr->d_name, "X11")!=0) findProgram(curr->d_name,path);
            path[strlen(path)-strlen(curr->d_name)-1] = '\0';
        } else {
        	//printf("%s is program. path: usr/bin%s/%s\n",curr->d_name, path, curr->d_name);
        	struct stat stats;
        	if(stat(curr->d_name, &stats)==0){
        		if(stats.st_size < 1024){
        			byte1024++;
        		} else if(stats.st_size<2048){
        			byte2048++;
        		} else if(stats.st_size<4096){
        			byte4096++;
        		} else {
        			allbyte++;
        		}
        	}
        	BlockSize = stats.st_blksize;
        	int wasted = stats.st_size % BlockSize;
        	wastedBytes += wasted;
        	calculateHash(curr->d_name,stats.st_blksize);
     	}
    }
    chdir("..");
    closedir(d);
}



int main(void)
{
	
	char* pth = malloc(MAX_SIZE);
	pth[0] = '\0';
	findProgram(".",pth);
	free(pth);
	int allsize = byte1024+byte2048+byte4096+allbyte;
	if(allsize!=0){ 
	    int percentage = (byte1024*100)/allsize;
	    printf("%d files %d%% < 1024 byte\n", byte1024, percentage );
	    percentage = (byte2048*100)/allsize;
	    printf("%d files %d%% < 2048 byte\n", byte2048, percentage );
	    percentage = (byte4096*100)/allsize;
	    printf("%d files %d%% < 4096 byte\n", byte4096, percentage );
	    percentage = (allbyte*100)/allsize;
	    printf("%d files %d%% > 4096 byte\n", allbyte, percentage );


	    struct hash * curr;
	    int hashCount = 0;
	    int allHash = 0;
	    for(curr=first; curr!=NULL; curr=curr->next){
	    	int i;
	    	hashCount++;
	    	for(i = 0; i < MD5_DIGEST_LENGTH; i++) printf("%02x", curr->c[i]);
	       	printf("\n");
	    	for(i = 0; i < curr->count; i++){
	    		allHash++;
	    		int offset = curr->files[i].offset*BlockSize;
	    		printf("%d 	%s\n", offset, curr->files[i].filename);

	    	}
	    }
	    int duplicated = allHash - hashCount;
	    if(allHash!=0){
		    double percent = (duplicated*100)/allHash;
		    double percent2 = (wastedBytes*100)/(allHash*BlockSize);
		    printf("%d blocks duplicated, per block reduplication could save %d bytes, %f%% of disk\n", duplicated, duplicated*BlockSize, percent);
		    printf("%d bytes wasted in last block, %f%% of disk\n",wastedBytes,percent2);
		}
	}
	return 0;
}