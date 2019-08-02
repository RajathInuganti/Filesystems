//
//  File.c
//  
//
//  Created by RAJATH on 4/8/19.
//

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
const int BLOCK_SIZE = 512;
const int NUM_BLOCKS = 4096;
const int INODE_SIZE = 32;

void writeBlock(FILE *disk, int blockNum, char* data, int size){
    
    fseek(disk, blockNum * BLOCK_SIZE, SEEK_SET); // Find the block to write to
    fwrite(data, size, 1, disk);
   	
}

void readBlock(FILE *disk, int blockNum, char* buffer){
    fseek(disk, blockNum * BLOCK_SIZE, SEEK_SET);
    fread(buffer, BLOCK_SIZE, 1, disk);
}

int findFreeBlock(FILE *disk){
    char *fbv = malloc(BLOCK_SIZE);
    readBlock(disk, 1, fbv);
    for(int i = 10; i < NUM_BLOCKS; i++){
        int byteno = (i - (i%8))/8;
        unsigned char byte = fbv[byteno];
        int block = 0;
        int x = byte;
        for(int c = 7; c >= 0; c--)
        {
            int y = x >> c;
            if (y & 1){
                block = (byteno * 8) + (7-c);
                free(fbv);
                return block;
            }
        }
    }
    free(fbv);
    return 0;
}

void setFreeBlockVector(FILE *disk, int block, int status){
    char *fbv = malloc(BLOCK_SIZE);
    readBlock(disk, 1, fbv);
    int byteno = (block - (block % 8))/8;
    int bit = 7 - (block % 8);
    unsigned char byte = fbv[byteno];
    int x = byte;
    if(status == 0){
        x = x - pow(2, bit);
    }
    else if(status == 1){
        x = x + pow(2, bit);
    }
    byte = x;
    fbv[byteno] = byte;
    writeBlock(disk, 1, fbv, BLOCK_SIZE);
    free(fbv);
}


char* UpdateImapSector(FILE *disk, int status, int InodeNo){ // Robust Version Control
	char *sb = malloc(BLOCK_SIZE);
	readBlock(disk, 0, sb);
	int maxInodeIndex;
	int numOfInodes;
	memcpy(&maxInodeIndex, sb + 8, sizeof(maxInodeIndex));
	memcpy(&numOfInodes, sb + 20, sizeof(maxInodeIndex));
	if(numOfInodes == 255){
		printf("ERROR: Out of Memory!");
		free(sb);
		exit(0);
	}
	char *imap = malloc(BLOCK_SIZE);
	int Curr_Imap; short int blockno; char *imap_entry = malloc(4); short int pos = 0;;
	memcpy(&Curr_Imap, sb + 4 * (sizeof(int)), sizeof(int));
	if(status == 0){
		char *imap_temp = malloc(BLOCK_SIZE);
		if(Curr_Imap == 2){
			readBlock(disk, 9, imap);
			for(int i = 0; i < 2*InodeNo-2; i++){
				imap_temp[i] = imap[i];
			}
			int temp;
			memcpy(&temp, imap + 2*InodeNo-2, sizeof(short int));
			setFreeBlockVector(disk, temp, 1);
			printf("1: %d\n",temp);
			for(int j = 2*InodeNo-2; j < 2*InodeNo; j++){
				imap_temp[j] = 0;
			}
			for(int k = 2*InodeNo; k < 2*maxInodeIndex; k++){
				imap_temp[k] = imap[k];
			}
			writeBlock(disk, 2, imap_temp, BLOCK_SIZE);
		}
		else{
			readBlock(disk, Curr_Imap-1, imap);
			for(int i = 0; i < 2*InodeNo-2; i++){
				imap_temp[i] = imap[i];
			}
			int temp;
			memcpy(&temp, imap + 2*InodeNo-2, sizeof(short int));
			setFreeBlockVector(disk, temp, 1);
			printf("1: %d\n",temp);
			for(int j = 2*InodeNo-2; j < 2*InodeNo; j++){
				imap_temp[j] = 0;
			}
			for(int k = 2*InodeNo; k < 2*maxInodeIndex; k++){
				imap_temp[k] = imap[k];
			}
			writeBlock(disk, Curr_Imap, imap_temp, BLOCK_SIZE);
		}
		numOfInodes--;
		free(imap_temp);
	}
	else if(status == 1){
		int flag = 0; 
		
		if(Curr_Imap == 2){
			readBlock(disk, 9, imap);
		}
		else{
			readBlock(disk, Curr_Imap-1, imap);
		}
		blockno = (short int)findFreeBlock(disk);
		setFreeBlockVector(disk, blockno, 0);
		short int check;
		for(int i = 0; i < maxInodeIndex; i++){
			memcpy(&check, imap + i*2, sizeof(short int));
			if(check == 0){
				pos = i;
				memcpy(imap + i*2, &blockno, sizeof(blockno));
				numOfInodes++;
				flag = 1;
			}
		}
		if(flag == 0){
			memcpy(imap + maxInodeIndex * sizeof(short int), &blockno, sizeof(blockno));
			numOfInodes++; maxInodeIndex++;
			pos = maxInodeIndex;
		}
		writeBlock(disk, Curr_Imap, imap, BLOCK_SIZE);
	}
	if(Curr_Imap==9){
		Curr_Imap = 2;
	}
	else
		Curr_Imap++;
	memcpy(sb + 4 * (sizeof(int)), &Curr_Imap, sizeof(int));
	memcpy(sb + 8, &maxInodeIndex, sizeof(int));
	memcpy(sb + 20, &numOfInodes, sizeof(int));
	writeBlock(disk, 0, sb, BLOCK_SIZE);
	memcpy(imap_entry, &pos, sizeof(short int));
	memcpy(imap_entry + sizeof(short int), &blockno, sizeof(short int));
	free(sb);
	free(imap);
	return imap_entry;
}
void makeEmpty(char *var){
	for(int i = 0; i < BLOCK_SIZE; i++){
		var[i] = 0;
	}
}




char* createInode(FILE *disk, int size, int filetype) {
	
    char* inode = malloc(INODE_SIZE);
    memcpy(inode, &size, sizeof(size));
    memcpy(inode + sizeof(int), &filetype, sizeof(filetype)); // directory if filetype 0 and flat-file if filetype is 1
    if(filetype == 0){
    	short int block = (short int)findFreeBlock(disk);
    	setFreeBlockVector(disk, block, 0);
    	memcpy(inode + 2 * sizeof(int), &block, sizeof(short int));
    }
    else if(filetype == 1){
    	double alc = (double)size/BLOCK_SIZE;
    	double allocation = ceil(alc);
    	if((allocation <= 10)||(allocation > 10)){
    		if(allocation > 10){
    			for(int i = 0; i < 10; i++){
    				short int block = (short int)findFreeBlock(disk);
    				setFreeBlockVector(disk, block, 0);
    				memcpy(inode + 2*sizeof(int) + i*sizeof(short int), &block, sizeof(short int));
    			}
    		}
    		if(allocation <= 10){
    			for(int i = 0; i < allocation; i++){
    				short int block = (short int)findFreeBlock(disk);
    				setFreeBlockVector(disk, block, 0);
    				memcpy(inode + 2*sizeof(int) + i*sizeof(short int), &block, sizeof(short int));
    			}
    		}
    	}
    	if(((allocation > 10)&&(allocation <= 256))||(allocation > 256)){
    		short int block = (short int)findFreeBlock(disk);
    		setFreeBlockVector(disk, block, 0);
    		memcpy(inode + 2*sizeof(int) + 10 * sizeof(short int), &block, sizeof(short int));
    		char *blockinfo = malloc(BLOCK_SIZE);
    		makeEmpty(blockinfo);
    		if(allocation <= 266){
    			for(int i = 0; i < allocation-10; i++){
    				short int ind_block = (short int)findFreeBlock(disk);
    				setFreeBlockVector(disk, ind_block, 0);
    				memcpy(blockinfo + i*sizeof(short int), &ind_block, sizeof(short int));
    			}
    			writeBlock(disk, block, blockinfo, BLOCK_SIZE);
    			free(blockinfo);
    		}
    		if(allocation > 266){
    			for(int i = 0; i < 256; i++){
    				short int ind_block = (short int)findFreeBlock(disk);
    				setFreeBlockVector(disk, ind_block, 0);
    				memcpy(blockinfo + i*sizeof(short int), &ind_block, sizeof(short int));
    			}
    			writeBlock(disk, block, blockinfo, BLOCK_SIZE);
    			free(blockinfo);
    		}
    	}
    	if(allocation > 266){
    		short int block2 = (short int)findFreeBlock(disk);
    		setFreeBlockVector(disk, block2, 0);
    		memcpy(inode + 2*sizeof(int) + 11 * sizeof(short int), &block2, sizeof(short int));
    		int numBlocks = ceil((double)allocation/256);
    		int doubleIndBlocks = numBlocks*256-allocation;
    		char *indirectinfo = calloc(BLOCK_SIZE, 1);
    		for(int j = 0; j < numBlocks-1; j++){
    			char *blockinfo2 = malloc(BLOCK_SIZE);
    			short int ind2_block = (short int)findFreeBlock(disk);
    			setFreeBlockVector(disk, ind2_block, 0);
    			for(int i = 0; i < 256; i++){
    				short int ind2_blocks = (short int)findFreeBlock(disk);
    				setFreeBlockVector(disk, ind2_blocks, 0);
    				memcpy(blockinfo2 + i*sizeof(short int), &ind2_blocks, sizeof(short int));
    			}
    			memcpy(indirectinfo + j*sizeof(short int), &ind2_block, sizeof(short int));
    		}
    		
    		char *blockinfo2 = malloc(BLOCK_SIZE);
    		short int ind2_block = (short int)findFreeBlock(disk);
    		setFreeBlockVector(disk, ind2_block, 0);
    		for(int i = 0; i < doubleIndBlocks; i++){
    			short int ind2_blocks = (short int)findFreeBlock(disk);
    			setFreeBlockVector(disk, ind2_blocks, 0);
    			memcpy(blockinfo2 + i*sizeof(short int), &ind2_blocks, sizeof(short int));
    		}
    		memcpy(indirectinfo + (numBlocks-1)*sizeof(short int), &ind2_block, sizeof(short int));
    		writeBlock(disk, block2, indirectinfo, BLOCK_SIZE);
    	}
    }
	return inode;
}

void Delete(FILE *disk, char *name){
	short int Curr_Imap;
	char *dir = malloc(BLOCK_SIZE);
	readBlock(disk, 0, dir);
	memcpy(&Curr_Imap, dir + 4 * sizeof(int), sizeof(short int));
	makeEmpty(dir);
	readBlock(disk, Curr_Imap-1, dir);
	short int RootInodeBlock;
	memcpy(&RootInodeBlock, dir, sizeof(short int));
	makeEmpty(dir);
	readBlock(disk, RootInodeBlock, dir);
	short int datadir;
	memcpy(&datadir, dir + 2 * sizeof(int), sizeof(short int));
	makeEmpty(dir);
	readBlock(disk, datadir, dir);
	int numJumps = 0;
    for(int i = 1; i < strlen(name); i++)
        if(name[i] == '/')numJumps++;
    
    char dirs[numJumps][strlen(name)];
    char temp[2]; int j = 0;
    char temp2[strlen(name)]; temp2[0] = '\0';
    for(int i = 1; i < strlen(name); i++){
        if(name[i] == '/'){
            strncpy(dirs[j], temp2, strlen(name));
            j = j + 1;
            temp2[0] = '\0';
        }
        else{
            temp[0] = name[i];
            temp[1] = '\0';
            strncat(temp2, temp, strlen(temp));
        }
    }
    short int blockOfData = datadir; 
     for(int i = 0; i < numJumps - 1; i++){
    	char *curr_dir = dirs[i];char temp[INODE_SIZE];
    	unsigned char inodenum;
    	for(int j = 1; j < BLOCK_SIZE; j = j + INODE_SIZE){
    		memcpy(&temp, dir + j, INODE_SIZE-1);
    		int cmp = strcmp(temp, curr_dir);
    		if(cmp == 0){
    			memcpy(&inodenum, dir + j-1, sizeof(char));
    			break;
    		}
    	}
    	int inode = inodenum;
    	makeEmpty(dir);
    	readBlock(disk, Curr_Imap-1, dir);
    	short int blockOfInode;
    	memcpy(&blockOfInode, dir+ 2*inode-2, sizeof(short int));
    	makeEmpty(dir);
    	readBlock(disk, blockOfInode, dir);
    	memcpy(&blockOfData, dir + 2 * sizeof(int), sizeof(short int));
    	makeEmpty(dir);
    	readBlock(disk, blockOfData, dir);
    }
    makeEmpty(dir);
    readBlock(disk, blockOfData, dir);
    unsigned char inodenum;
    char *curr_dir = name;
    for(int j = 1; j < BLOCK_SIZE; j = j + INODE_SIZE){
    	char temp[INODE_SIZE];
    	memcpy(&temp, dir + j, INODE_SIZE-1);
    	int cmp = strcmp(temp, curr_dir);
    	if(cmp == 0){
    		memcpy(&inodenum, dir + j-1, sizeof(char));
    		break;
    	}
    }
   	makeEmpty(dir);
   	readBlock(disk, Curr_Imap-1, dir);
   	short int inodeblocknum;
   	memcpy(&inodeblocknum, dir + inodenum*2-2, sizeof(short int));
   	char *del = UpdateImapSector(disk, 0, inodenum);
   	free(del);
   	makeEmpty(dir);
   	readBlock(disk, inodeblocknum, dir);
   	setFreeBlockVector(disk, inodeblocknum, 1);
   	int size;
   	memcpy(&size, dir, sizeof(int));
   	double alc = (double)size/BLOCK_SIZE;
    double allocation = ceil(alc);
    if((allocation <= 10)||(allocation > 10)){
    	for(int i = 0; i < 10; i++){
    		short int bn;
    		memcpy(&bn, dir + 2*sizeof(int)+ i*sizeof(short int), sizeof(short int));
    		setFreeBlockVector(disk, bn, 1);
    	}
    }
    if(((allocation > 10)&&(allocation <= 266))||(allocation > 266)){
    	short int bn;
    	memcpy(&bn, dir + 2*sizeof(int)+ 10*sizeof(short int), sizeof(short int));
    	setFreeBlockVector(disk, bn, 1);
    	char *bninfo = calloc(BLOCK_SIZE, 1);
    	readBlock(disk, bn, bninfo);
    	for(int i = 0; i < allocation-10; i++){
    		short int bn1;
    		memcpy(&bn1, bninfo + i*sizeof(short int), sizeof(short int));
    		setFreeBlockVector(disk, bn1, 1);
    	}
    }
    //if(allocation > 266){
    	//short int bn;
    	//memcpy(&bn, dir + 2*sizeof(int)+ 11*sizeof(short int), sizeof(short int));
    	//setFreeBlockVector(disk, bn, 1);
    	///int numBlocks = ceil((double)allocation/256);
    	//int doubleIndBlocks = numBlocks*256-allocation;
    	//for(int i = 0; i < numBlocks-1; i++){
    		//char *bn1 = calloc(BLOCK_SIZE);
    	//	readBlock(disk, bn1, BLOCK_SIZE);
    		
    	//}
    //}
}



void CreateFile(FILE *disk, char *name, FILE *file){
	short int Curr_Imap;
	char *dir = malloc(BLOCK_SIZE);
	readBlock(disk, 0, dir);
	memcpy(&Curr_Imap, dir + 4 * sizeof(int), sizeof(short int));
	makeEmpty(dir);
	readBlock(disk, Curr_Imap-1, dir);
	short int RootInodeBlock;
	memcpy(&RootInodeBlock, dir, sizeof(short int));
	makeEmpty(dir);
	readBlock(disk, RootInodeBlock, dir);
	short int datadir;
	memcpy(&datadir, dir + 2 * sizeof(int), sizeof(short int));
	makeEmpty(dir);
	readBlock(disk, datadir, dir);
	int numJumps = 0;
    for(int i = 1; i < strlen(name); i++)
        if(name[i] == '/')numJumps++;
    
    char dirs[numJumps][strlen(name)];
    char temp[2]; int j = 0;
    char temp2[strlen(name)]; temp2[0] = '\0';
    for(int i = 1; i < strlen(name); i++){
        if(name[i] == '/'){
            strncpy(dirs[j], temp2, strlen(name));
            j = j + 1;
            temp2[0] = '\0';
        }
        else{
            temp[0] = name[i];
            temp[1] = '\0';
            strncat(temp2, temp, strlen(temp));
        }
    }
    short int blockOfData = datadir; 
    for(int i = 0; i < numJumps - 1; i++){
    	char *curr_dir = dirs[i];char temp[INODE_SIZE];
    	unsigned char inodenum;
    	for(int j = 1; j < BLOCK_SIZE; j = j + INODE_SIZE){
    		memcpy(&temp, dir + j, INODE_SIZE-1);
    		int cmp = strcmp(temp, curr_dir);
    		if(cmp == 0){
    			memcpy(&inodenum, dir + j-1, sizeof(char));
    			break;
    		}
    	}
    	int inode = inodenum;
    	makeEmpty(dir);
    	readBlock(disk, Curr_Imap-1, dir);
    	short int blockOfInode;
    	memcpy(&blockOfInode, dir+ 2*inode-2, sizeof(short int));
    	makeEmpty(dir);
    	readBlock(disk, blockOfInode, dir);
    	memcpy(&blockOfData, dir + 2 * sizeof(int), sizeof(short int));
    	makeEmpty(dir);
    	readBlock(disk, blockOfData, dir);
    }
    makeEmpty(dir);
    //#include <sys/stat.h>
	//struct stat st;
	//stat(filename, &st);
	//size = st.st_size;
    fseek(file, 0, SEEK_END); 
	int size = ftell(file); 
	fseek(file, 0, SEEK_SET);
    
    readBlock(disk, blockOfData, dir);
    char *imap_entry = UpdateImapSector(disk, 1, NUM_BLOCKS);
    short int alloc; memcpy(&alloc, imap_entry + sizeof(short int), sizeof(short int));
    char *inode = createInode(disk, size, 1);
    writeBlock(disk, alloc, inode, INODE_SIZE);unsigned char alloca;
    memcpy(&alloca, imap_entry, sizeof(short int));
    int offset = 0;
    for(int i = 0; i < 16; i++){
    	if(dir[offset] != 0)offset = offset + 32;
    }
    memcpy(dir + offset, &alloca, sizeof(char));
    strncpy(temp2, dirs[numJumps-1], strlen(dirs[numJumps-1]));
    for(int i = 1; i <= strlen(temp2); i++){
    	memcpy(dir + i * sizeof(char) + offset, &temp2[i-1], sizeof(char));
    }
    
    writeBlock(disk, blockOfData, dir, BLOCK_SIZE);
    double alc = (double)size/BLOCK_SIZE;
    double allocation = ceil(alc);
    if((allocation <= 10)||(allocation > 10)){
    	if(allocation <= 10){
    		for(int i = 0; i < allocation; i++){
    			short int currblock;
    			memcpy(&currblock, inode + 2*sizeof(int) + i*sizeof(short int), sizeof(short int));
    			char *buffer = calloc(BLOCK_SIZE, 1);
    			readBlock(file, i, buffer);
    			printf("current block: %d\n", currblock);
    			writeBlock(disk, currblock, buffer, BLOCK_SIZE);
    			free(buffer);
    		}
    	}
    	if(allocation > 10){
    		for(int i = 0; i < 10; i++){
    			short int currblock;
    			memcpy(&currblock, inode + 2*sizeof(int) + i*sizeof(short int), sizeof(short int));
    			char *buffer = calloc(BLOCK_SIZE, 1);
    			readBlock(file, i, buffer);
    			writeBlock(disk, currblock, buffer, BLOCK_SIZE);
    			free(buffer);
    		}
    	}
    }
    if(((allocation > 10)&&(allocation <= 266))||(allocation > 266)){
    	if(allocation <= 266){
    		short int currblock;
    		memcpy(&currblock, inode + 2*sizeof(int) + 10*sizeof(short int), sizeof(short int));
    		char *blockinfo = calloc(BLOCK_SIZE, 1);
    		readBlock(disk, currblock, blockinfo);
    		for(int i = 0; i < allocation - 10; i++){
    			short int currblock2;
    			char *buffer = calloc(BLOCK_SIZE, 1);
    			memcpy(&currblock2, blockinfo + i * sizeof(short int), sizeof(short int));
    			readBlock(file, i+10, buffer);
    			writeBlock(disk, currblock2, buffer, BLOCK_SIZE);
    			free(buffer);	
    		}
    		free(blockinfo);
    	}
    	if(allocation > 266){
    		short int currblock3;
    		memcpy(&currblock3, inode + 2*sizeof(int) + 10*sizeof(short int), sizeof(short int));
    		char *blockinfos = calloc(BLOCK_SIZE, 1);
    		readBlock(disk, currblock3, blockinfos);
    		for(int i = 0; i < 256; i++){
    			short int currblock2;
    			char *buffer = calloc(BLOCK_SIZE, 1);
    			memcpy(&currblock2, blockinfos + i * sizeof(short int), sizeof(short int));
    			readBlock(file, i+10, buffer);
    			writeBlock(disk, currblock2, buffer, BLOCK_SIZE);
    			free(buffer);	
    		}
    		free(blockinfos);
    	}
    }
   	if(allocation > 266){
   		short int currblock;
    	memcpy(&currblock, inode + 2*sizeof(int) + 11*sizeof(short int), sizeof(short int));
    	char *blockinfo = calloc(BLOCK_SIZE, 1);
    	readBlock(disk, currblock, blockinfo);
    	int numBlocks = ceil((double)allocation/256);
    	for(int i = 0; i < numBlocks; i++){
    		short int cb;
    		memcpy(&cb, blockinfo + i*sizeof(short int), sizeof(short int));
    		char *cbinfo = calloc(BLOCK_SIZE, 1);
    		readBlock(disk, cb, cbinfo);
    		for(int k = 0; k < 256; k++){
    			short int cb2;
    			memcpy(&cb2, cbinfo + k*sizeof(short int), sizeof(short int));
    			char *buffer = calloc(BLOCK_SIZE, 1);
    			readBlock(file, i+266, buffer);
    			writeBlock(disk, cb2, buffer, BLOCK_SIZE);
    			free(buffer);
    		}
    		free(cbinfo);
    	}
    	free(blockinfo);
   	}
    free(inode);
    free(imap_entry);
    free(dir);
}

void CreateDirectory(FILE *disk, char *name){
	short int Curr_Imap;
	char *dir = malloc(BLOCK_SIZE);
	readBlock(disk, 0, dir);
	memcpy(&Curr_Imap, dir + 4 * sizeof(int), sizeof(short int));
	makeEmpty(dir);
	readBlock(disk, Curr_Imap-1, dir);
	short int RootInodeBlock;
	memcpy(&RootInodeBlock, dir, sizeof(short int));
	makeEmpty(dir);
	readBlock(disk, RootInodeBlock, dir);
	short int datadir;
	memcpy(&datadir, dir + 2 * sizeof(int), sizeof(short int));
	makeEmpty(dir);
	readBlock(disk, datadir, dir);
	int numJumps = 0;
    for(int i = 1; i < strlen(name); i++)
        if(name[i] == '/')numJumps++;
    
    char dirs[numJumps][strlen(name)];
    char temp[2]; int j = 0;
    char temp2[strlen(name)]; temp2[0] = '\0';
    for(int i = 1; i < strlen(name); i++){
        if(name[i] == '/'){
            strncpy(dirs[j], temp2, strlen(name));
            j = j + 1;
            temp2[0] = '\0';
        }
        else{
            temp[0] = name[i];
            temp[1] = '\0';
            strncat(temp2, temp, strlen(temp));
        }
    }
    short int blockOfData = datadir; 
    for(int i = 0; i < numJumps - 1; i++){
    	char *curr_dir = dirs[i];char temp[INODE_SIZE];
    	unsigned char inodenum;
    	for(int j = 1; j < BLOCK_SIZE; j = j + INODE_SIZE){
    		memcpy(&temp, dir + j, INODE_SIZE-1);
    		int cmp = strcmp(temp, curr_dir);
    		if(cmp == 0){
    			memcpy(&inodenum, dir + j-1, sizeof(char));
    			break;
    		}
    	}
    	int inode = inodenum;
    	makeEmpty(dir);
    	readBlock(disk, Curr_Imap-1, dir);
    	short int blockOfInode;
    	memcpy(&blockOfInode, dir+ 2*inode-2, sizeof(short int));
    	makeEmpty(dir);
    	readBlock(disk, blockOfInode, dir);
    	memcpy(&blockOfData, dir + 2 * sizeof(int), sizeof(short int));
    	makeEmpty(dir);
    	readBlock(disk, blockOfData, dir);
    }
    makeEmpty(dir);
    
    readBlock(disk, blockOfData, dir);
    char *imap_entry = UpdateImapSector(disk, 1, NUM_BLOCKS);
    short int alloc; memcpy(&alloc, imap_entry + sizeof(short int), sizeof(short int));
    char *inode = createInode(disk, INODE_SIZE, 0);
    writeBlock(disk, alloc, inode, INODE_SIZE);unsigned char alloca;
    memcpy(&alloca, imap_entry, sizeof(short int));
    int offset = 0;
    for(int i = 0; i < 16; i++){
    	if(dir[offset] != 0)offset = offset + 32;
    }
    memcpy(dir + offset, &alloca, sizeof(char));
    strncpy(temp2, dirs[numJumps-1], strlen(dirs[numJumps-1]));
    for(int i = 1; i <= strlen(temp2); i++){
    	memcpy(dir + i * sizeof(char) + offset, &temp2[i-1], sizeof(char));
    }
    
    writeBlock(disk, blockOfData, dir, BLOCK_SIZE);
    free(imap_entry);
    free(dir);
}



void InitRootDir(FILE *disk){
	char *imap_entry = UpdateImapSector(disk, 1, NUM_BLOCKS);
	short int blockno;
	memcpy(&blockno, imap_entry + sizeof(short int), sizeof(short int));
    char *inode = createInode(disk, INODE_SIZE, 0); //size is same as block size for directory and filetype is 0
    writeBlock(disk, blockno, inode, INODE_SIZE);
    free(inode);
}

void initFreeBlockVector(FILE *disk){
    char *fb = malloc(BLOCK_SIZE);
    char c = 0;
    memcpy(fb, &c, sizeof(char));
    c = 0x3f;
    memcpy(fb + sizeof(char), &c, sizeof(char));
    c = 0xff;
    for(int i = 2; i < 512; i++){
        memcpy(fb + 2 + (i-2) * sizeof(char), &c, sizeof(char));
    }
    writeBlock(disk, 1, fb, BLOCK_SIZE);
    free(fb);
}


void initSuperBlock(FILE *disk){
    
    char *sb = malloc(BLOCK_SIZE);
    int magic = 32;
    int numOfBlocks = BLOCK_SIZE;
    int maxInodeIndex = 0;
    int freeBlockVector = 1;
    int imapNoOfRoot = 2;
    int numOfInodes = 0;
    
    memcpy(sb, &magic, sizeof(magic));
    memcpy(sb + sizeof(magic), &numOfBlocks, sizeof(numOfBlocks));
    memcpy(sb + 2 * sizeof(magic), &maxInodeIndex, sizeof(numOfBlocks));
    memcpy(sb + 3 * sizeof(magic), &freeBlockVector, sizeof(numOfBlocks));
    memcpy(sb + 4 * sizeof(magic), &imapNoOfRoot, sizeof(numOfBlocks));
    memcpy(sb + 5 * sizeof(magic), &numOfInodes, sizeof(numOfBlocks));
    
    writeBlock(disk, 0, sb, BLOCK_SIZE);
    free(sb);
}


int main(int argc, char* argv[]) {
  	FILE *disk;
    char *fileName = "vdisk";
    disk = fopen(fileName, "wb"); // Open the file to be written to in binary mode
    char* init = calloc(NUM_BLOCKS, BLOCK_SIZE);
    fwrite(init, BLOCK_SIZE, NUM_BLOCKS, disk);
    free(init);
    fclose(disk);
    disk = fopen(fileName, "rb+");
   

    initSuperBlock(disk);
    initFreeBlockVector(disk);
    InitRootDir(disk);
    
    

    
    fclose(disk);
    return 0;
}

