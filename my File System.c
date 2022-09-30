#include<stdio.h>
#include<errno.h>
#include<fcntl.h>
#include<sys/types.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>
#include"parse.h"

							// VD  :  Virtual Disk
							// AD  :  Actual Disk

char a;
char *errorMessage;

off_t disksize = 104857600L;
size_t blockSize=512;
unsigned long int noOfBlocks;
char *diskName = "disk.teasage";

char nullbyte[]="\0";

struct freeBlocksDetails{

short *blocksStatus;			// 1 : block is free  0 : block is allocated
unsigned long int availableFreeBlocks; //  no of available free blocks
};

struct freeBlocksDetails *freeBlocksNode;

struct fileNode
{

char *fileName;
unsigned long int *blockNos; 	// array of block indexes(by sequence) used to stored file on VD.

size_t size;

struct fileNode *next;
};


struct fileNode *head_fileList=NULL;        // linked list of file records(struct fileNode) on VD


int createDisk();
struct fileNode *createFileNode(char *fileName, unsigned long int *blockNos, unsigned long int requiredNoOfBlocks ,size_t size);

int insert_fileNode(struct fileNode *newNode );  // insert new new record (fileNode) in linked list.
int remove_fileNode(char *fileName);		     // remove record (fileNode) from linked list.(used while deleting file from VD)

int getFileNode(char *fileName, struct fileNode **fNode);   // used to check wheather file with given name present on VD

int vdls(char *filesName);
int vdcpto(char *adFilePath, char* newFileName);
int vdcpfrom(char *vdFileName, char *adFilePath );

int compareFile(char *adFilePath, struct fileNode *fNode);

int readDiskBlock(int fd, int blockNo, unsigned char  *buffer);
int writeDiskBlock(int fd, int blockNo, unsigned char *buffer);

int deleteFileFromDisk(char *fileName);

int getFreeBlocks(size_t fileSize,unsigned long int **freeBlocksIndexes);       // if enough memory available store those block's indexes in freeBlocksIndexes array.
int freeAllocatedBlocks(unsigned long allocatedBlocksIndexes[],unsigned long noOfBlocks, unsigned long int *noOfblocksFreed  ); // change blocks status allocated to free(used while deleting file from VD)

int getFileNameFromFilePath(char* AD_filePath, char **newFileName );    // extract file name from fullPath

void print_filesList(unsigned char *filesList);     // print buffer of vdls();

int seperatePathAndFileName(char *AD_filePath, char **dirPath, char **fileName);

int getFileName(char *path, char *currentFileName, char **newFileName);

int validateFileName(char *fileName);

int renameVDFile(char *currentFileName , char *newFileName);


/*
1 on of the block appeares to be already free
2 failed to free all allocated bloks blocks
3 unnecessary blocks freed
*/

int freeAllocatedBlocks(unsigned long allocatedBlocksIndexes[],unsigned long int noOfBlocks, unsigned long int *noOfblocksFreed ) // returns no Of Blocks freed
{
    unsigned long int index=0;
    unsigned long int freedBlockCnt=0;

    for(index=0, freedBlockCnt=0; index<noOfBlocks; index++)
    {
        if(freeBlocksNode->blocksStatus[allocatedBlocksIndexes[index]] == 0)
        {
            freeBlocksNode->blocksStatus[allocatedBlocksIndexes[index]] = 1;
            freedBlockCnt++;
        }

        else
           return 1;
   }

   freeBlocksNode->availableFreeBlocks+= noOfBlocks;
   *noOfblocksFreed = freedBlockCnt;

  if (freedBlockCnt < noOfBlocks)
    return 2;

  if (freedBlockCnt > noOfBlocks)
    return 3;

    return 0;
}



int renameVDFile(char *currentFileName , char *newFileName)
{
struct fileNode *node1 =NULL;
struct fileNode *node2 =NULL;

int result = validateFileName(newFileName);

if(result)
{
if(result==1)
	return 1; // invalid filename for, having large length
if(result==2)
	return 2;// invlid,contains'/'
}

getFileNode(currentFileName,&node1);

getFileNode(newFileName,&node2);

if(!node1)
return 3; // file currentFileName doesnt exist


if(node2)
return 4; // new file name already exist, choose different file name


node1->fileName=newFileName;

return 0;// no error
}




int validateFileName(char *fileName)
{

if(strlen(fileName) >255)
	return 1;// invlaid, length is  >255

for(int i=0; i<255; i++)
{
if(fileName[i]=='/')
	return 2; //invalid,conatins '/'
}


return 0; // no error, valid filename
}


struct fileNode *createFileNode(char *fileName, unsigned long int *blockNos, unsigned long int requiredNoOfBlocks ,size_t size)
{
struct fileNode *fNode = malloc(sizeof(struct fileNode ));

fNode->fileName = malloc(sizeof(char)*(strlen(fileName)+1));
strcpy(fNode->fileName,fileName);

fNode->blockNos = malloc(sizeof(unsigned long int)*requiredNoOfBlocks);
memcpy(fNode->blockNos,blockNos, sizeof(unsigned long int)*requiredNoOfBlocks );

fNode->size = size;
fNode->next = NULL;

return fNode;
}

/*
1 not enough memory available
2
3
*/

int getFreeBlocks(size_t fileSize, unsigned long int **freeBlocksIndexes)			// returns indexes of freeblocks
{

    unsigned long int requiredNoOfBlocks = fileSize/blockSize + (  (fileSize%blockSize) ? 1 : 0 );
    *freeBlocksIndexes = malloc(sizeof(unsigned long int)*requiredNoOfBlocks);
    if( freeBlocksNode->availableFreeBlocks >= requiredNoOfBlocks)
    {
        unsigned long int index=0;
        unsigned long int freeBlocksCnt = 0;

        for( index =0 ; (index < noOfBlocks) && (freeBlocksCnt < requiredNoOfBlocks) ; index++  )
        {
            if(freeBlocksNode->blocksStatus[index] == 1 )
            {
                freeBlocksNode->blocksStatus[index] = 0;
                (*freeBlocksIndexes)[freeBlocksCnt++] = index;
             }
        }

        freeBlocksNode->availableFreeBlocks -=freeBlocksCnt;
         return 0;
    }

    else{
 return 1;		// not enough memory available
}

}
/*
1 lseek error occured in writeDiskBlock();
2 error occured while performing write() in  writeDiskBlock();
3
*/

int writeDiskBlock(int vd_fd, int blockNo, unsigned char *buffer)
{

if(lseek(vd_fd, (blockNo*blockSize), SEEK_SET) == -1)
{

return 1;
}

if(write(vd_fd,buffer,blockSize)<=0)
	return 2;
else		// error no set myperror}
	return 0;
}

/*
1	lseek error occured in readDiskBlock()
2   error occured while performing read() in readDiskBlock();
*/

int readDiskBlock( int vd_fd, int blockNo, unsigned char *buffer)
{

off_t location = blockNo*512;

if (lseek(vd_fd,location,SEEK_SET) == -1)
{
return 1;
}

if(read(vd_fd,buffer,512) <= 0)
return 2; 		// error no set myperror

return 0;
}

/*
1  VD is empty! (i.e. file doesnt exist)
2  file doesn't exist
*/

int getFileNode(char *fileName, struct fileNode **result_fNode)
{
/* returns file node
			NULL if file doesnt exist
*/
if(head_fileList==NULL)
{
return 1;   	// file doesnt exist
}

struct fileNode *fNode=head_fileList;
while((fNode) && strcmp(fNode->fileName,fileName)!=0)
{
	fNode=fNode->next;
}
if(!fNode)
{
return 2;   	// file doesnt exist
}
if(strcmp(fNode->fileName,fileName)==0){
	*result_fNode = fNode;
	return 0;
}

};
/*
1 newNode is null
2 failed to insert fileNode in file list
*/



int insert_fileNode(struct fileNode *newNode)
{

	if(!newNode)
		return 1;

	if(head_fileList==NULL)
	{
		//printf("insert_fileNode() add node as\n");  // debugg
		head_fileList = newNode;

		if(!head_fileList)
			return 2;

		return 0;
	}

	else{
		struct fileNode *fNode=head_fileList;

		while(fNode->next!=NULL)
		{
			fNode=fNode->next;
		}

		fNode->next = newNode;

		if(!(fNode->next))
			return 2;

		return 0;
	}
}


/*
1  VD is empty(i.e. file doesnt exist/ invalid file name )
2  file doesnt't exist/ invalid file name
*/

int remove_fileNode(char *fileName)
{
	if(head_fileList==NULL)
	{
	return 1;	// file doesnt exist
	}

else{

	if((strcmp(head_fileList->fileName,fileName)==0))
	{

        struct fileNode *fNode = head_fileList;


						// check this
		free(fNode->fileName);
		free(fNode->blockNos);

        head_fileList=head_fileList->next;

		free(fNode);


		return 0;
	}

	else
	{
		struct fileNode *fNode1=head_fileList;
		struct fileNode *fNode2=fNode1->next;

		while((fNode2) && (strcmp(fNode2->fileName,fileName)!=0))
		{
			fNode1=fNode2;
			fNode2=fNode2->next;
		}

		if(!fNode2)
		{
		 return 2; // file doesnt exist.
		}

else if(strcmp(fNode2->fileName,fileName)==0)
{
	fNode1->next=fNode2->next;

	free(fNode2->fileName);
	free(fNode2->blockNos);
	//free(fNode2->next);

	free(fNode2);

	return 0;
}

}
}

}


/*
1  file doesn't exist/ invalid file name
2 on of the block appeares to be already free
3 failed to free all allocated blocks
4 unnecessary blocks freed
5  file list is empty(i.e. file doesnt exist/ invalid file name )
6  file doesnt't exist/ invalid file name

*/
/*
1  VD doesn't containt any file (i.e. file doesnt exist)
2  file doesn't exist
*/

int deleteFileFromDisk(char *fileName)
{

/*
getFileNode()

1  VD is empty! (i.e. file doesnt exist)
2  file %s doesn't exist on VD !



remove_fileNode()
3  VD is empty(i.e. file doesnt exist)
4  file %s doesnt't exist VD



5 on of the blocks appeares to be already free (terminate, disk corrupted)
6 failed to free all allocated bloks blocks (terminate, disk corrupted)
7 unnecessary blocks freed  (terminate, disk corrupted)
*/
//printf(" fileName in deleteFileFromDisk() : %s \n",fileName );// debugg
struct fileNode *fNode = NULL;
int result = getFileNode(fileName, &fNode);

if(result)
{
if(result==1)
    return 1;
if(result==2)
    return 2;
}
unsigned long int noOfBlocks = (fNode->size/blockSize) + (((fNode->size % blockSize)) ? 1 : 0) ;

unsigned long allocatedBlocksIndexes[noOfBlocks] ;
unsigned long int *noOfblocksFreed = malloc(sizeof(unsigned long int));

result = freeAllocatedBlocks(fNode->blockNos, noOfBlocks, noOfblocksFreed );
if((!result) &&  (*noOfblocksFreed == noOfBlocks))
{
 if (!(result = remove_fileNode(fileName)))
return 0;

else
{
    if(result == 1)
        return 3;
    else if(result == 2)
        return 4;
}

}
else {
    if(result == 1)
	  return 5;
	else if(result == 2)
      return 6;
    else if(result == 3)
      return 7;
     }
}



/*
1  unable to create grand disk
2  write() error in createDisk()
3  lseek() error in createDisk()
4
*/


int createDisk( )
{
int fd = open(diskName, O_WRONLY|O_CREAT);

if(fd==-1)
{
	printf("Error: cannot create the grand disk.\n Errorno: %d\n", errno);
	perror("Open error.");
	return 1;
}

if (write(fd, nullbyte, 1) != 1){
perror("write error");
return 2;
}

if (lseek(fd, disksize-1, SEEK_SET) == -1){
    perror("lseek error");
    return 3;
}

if (write(fd, nullbyte, 1) != 1){
    perror("write error");
    return 2;
}

noOfBlocks = (disksize/512);

freeBlocksNode = malloc(sizeof(struct freeBlocksDetails));		// pointer for struct freeBlocksDetails

freeBlocksNode->blocksStatus =(short *)malloc(sizeof(short )*noOfBlocks);
unsigned long int i;

for(i=0; i<noOfBlocks;i++)
{
(freeBlocksNode->blocksStatus)[i]=1;
}
freeBlocksNode->availableFreeBlocks = noOfBlocks;

printf("\nCongratulations!\nYour brand new harddisk is created fresh just now!! Enjoy!!!\n\n");


struct stat stat_VD;
int result = stat(diskName,&stat_VD);

if(  (access(diskName,R_OK)!=0) || (access(diskName,W_OK)!=0)   )
 printf("in order to work file system needs to have read, write permissions\n");

while((access(diskName,R_OK)!=0) || (access(diskName,W_OK)!=0))
{
    if((result==0) && (access(diskName,R_OK)!=0) && (access(diskName,W_OK)!=0) )

    {
        printf("please accquire read, write permissions  and press enter\n");
    }

    else if((result==0) && (access(diskName,R_OK)==0) && (access(diskName,W_OK)!=0) )
    {
        printf("please accquire write permission and press enter\n");
    }

    else if((result==0) && (access(diskName,R_OK)!=0) && (access(diskName,W_OK)==0) )
    {
        printf("please accquire read permission and press enter\n");
    }
scanf("%c",&a);
}



close(fd);

return 0;
}

/*
1 VD is empty
2 file %s doesnt exist
3 failed to accquire fileNode for file %s
4 failed to create adFilePath on AD
5 error occured while performing lseek() in readDiskBlock()
6 error occured while performing read() in readDiskBlock();
7 error occured while performing write() in vdcpfrom();

8 error occured while performing lseek() for last block in readDiskBlock();
9 error occured while performing read() for last block in readDiskBlock();

10 error occured while performing write() for last block in vdcpfrom();
*/

/*

failed to create file error when file with that name already exist on AD
change this for all files that you write/vdcpfrom()  must have atleast read permission to check md5sum
*/

int getFileName(char *path, char *currentFileName, char **newFileNameWithPath)
{

*newFileNameWithPath = (char *)malloc(strlen(path)+(sizeof(char)*(255+strlen(" copy 1000"))));
char *newFileName = (char *)malloc(sizeof(char)*(255+strlen(" copy 1000")));
char *tempPathStr= (char *)malloc(strlen(path)+(sizeof(char)*(255+strlen(" copy 1000"))));
int isPathValid;

struct stat path_stat;
sprintf(tempPathStr,"%s%s",path, currentFileName);
isPathValid = stat(tempPathStr, &path_stat);


if(isPathValid==-1)  //name is available
{
 sprintf(*newFileNameWithPath,"%s",tempPathStr);
  return 0;
}
//int limit=1000;

for(int i=1; i<1000; i++)
{
  sprintf(tempPathStr,"%s%s%s%d",path,currentFileName," copy ",i);

isPathValid = stat( tempPathStr, &path_stat);

  if(isPathValid ==-1 )  //name is available
  {
     sprintf(*newFileNameWithPath,"%s",tempPathStr);
    return 0;
  }
}

return 2; // choose filename for file
}


int vdcpfrom(char *adFilePath, char *vdFileName) //copy from vd to ad
{

struct fileNode *fNode =NULL ;
char *newFileNameWithPath=NULL;
int result=0;


char *dirPath;
char *fileName;

struct stat path_stat;
stat(adFilePath, &path_stat);


 if( (stat(adFilePath, &path_stat)==0) && (S_ISREG(path_stat.st_mode)==0))  //it is directory
{}

else {
        seperatePathAndFileName(adFilePath, &dirPath,  &fileName);

        stat(dirPath,&path_stat);
        if(dirPath!=NULL)
        {

          if(stat(dirPath, &path_stat)==-1)
            {//invalid path
               return 1;
            }

        }

sprintf(adFilePath,"%s",fileName);

if(stat(adFilePath,&path_stat)==0)//filename already exist
{
 //  file already exist with that name , choose different name
return 2;
}

result = validateFileName(fileName);

if(result)
{

if(result==1){
   return 3;// invalid file name, exceed maximum file name Length
}

}

// change this validate fileName for its lenghts, special characters
newFileNameWithPath = (char *)malloc(sizeof(char)*(strlen(adFilePath)+1));
strcpy(newFileNameWithPath,adFilePath);
      }


if( (stat(adFilePath, &path_stat)==0) && (S_ISREG(path_stat.st_mode)==0))
  {
     char *newPath=(char *)malloc(strlen(adFilePath+strlen(vdFileName))+10);

    if(access(adFilePath, W_OK)==0) // do you need read access too ? // change this
    {

    char *dirPath=NULL;
    char *fileName=NULL;

    {
      struct stat path_stat1;
      if(adFilePath[strlen(adFilePath)-1]!='/')
      {

        if(strlen(adFilePath)>499)
        {

          char *tempPath = (char*)malloc(sizeof(char)*510);
          strcpy(tempPath,adFilePath);
          free(adFilePath);
          adFilePath=tempPath;
        }
          strcat(adFilePath,"/");
      }
      stat(adFilePath, &path_stat);

    }
      getFileName(adFilePath, vdFileName, &newFileNameWithPath);

     }

    else // write what to do if no
    {
       free(newPath);
      free(newFileNameWithPath);
      return 4; //do not have write access to directory get write access or choose different path
    }
  }




result= getFileNode(vdFileName, &fNode);
if(result)
{

    if(result ==1)
        return 5;
    if(result ==2)
        return 6;
}

if(!fNode)
return 7;//  failed to acquired file ! report log


char *tempFilePath=NULL;

 int ad_fd = open(newFileNameWithPath,O_WRONLY|O_CREAT);

if(ad_fd<0)
{
 free(newFileNameWithPath);
 return 8;
}

unsigned char *buffer= (char *)malloc(513);
unsigned long int noOfBlocks = (((fNode->size)/blockSize)  + (((fNode->size)%blockSize)? 1 :0) );

int vd_fd = open(diskName,O_RDONLY);
if(vd_fd == -1)
{   // filed to access VD!

   free(newFileNameWithPath);
  free(buffer);

    return 9;
}

unsigned long int blockCnt = 0;

size_t noOfBytesInLastBlock = (((fNode->size)%blockSize)? ((fNode->size)%blockSize) : blockSize);



for(blockCnt = 0; blockCnt < noOfBlocks-1; blockCnt++ )
{
if(result = readDiskBlock(vd_fd,fNode->blockNos[blockCnt],buffer) )
	{
       if(result ==1)
        {
          free(newFileNameWithPath);
          free(buffer);
          return 10;
        }

    if(result ==2)
        {
          free(newFileNameWithPath);
          free(buffer);
          return 11;
        }
    }

if (write(ad_fd,buffer,blockSize) <= 0)
	{
    free(newFileNameWithPath);
    free(buffer);
    return 12;
  }
}
if(result = readDiskBlock(vd_fd,fNode->blockNos[blockCnt],buffer))
	{
       if(result ==1)
        {
          free(newFileNameWithPath);
          free(buffer);
          return 10;}
    if(result ==2)
        {
          free(newFileNameWithPath);
          free(buffer);
          return 11;}
    }

 if (write(ad_fd,buffer,noOfBytesInLastBlock) <= 0)
	{

    free(newFileNameWithPath);
    free(buffer);
    return 15;
  }


  result = compareFile(adFilePath,fNode);


  if(result==0)
  printf("File \"%s\" copied successfuly as \"%s\".\n",adFilePath,newFileNameWithPath);            // debugg
  else
  {
    return 13; // failed writing file to AD, file is not wrote properly
  }

 free(newFileNameWithPath);
  free(buffer);
  return 0; // no error
}

/*
1 VD is empty/ doesnt contain any file.
2
*/



/*
buffptr will point to NULL after ending fileNode linkedList
but this will loose pointer to the assigned memory through malloc
so before pointing it to null free the assigned memory.
*/


int vdls(char *bufferPtr) 	      	  // using first 4 bytes to define size of buffer
{                                        // next 1 byte as flag for is there yet other file names to copy.


if(head_fileList == NULL)                // 0 : no file names    1: there are file names to copy
{

    return 1;   // no files on the disk
}
 struct fileNode *fNode  = *(struct fileNode **)(bufferPtr+4);



char *filesName = bufferPtr+5;
 int bufferLength = 12;            // used lenght , 4bytes for total bufferCapacity(int) + 8bytes for (fileNode *fNode)
 int bufferCapacity = *( int *)(bufferPtr);  // total lenght

 int fileNameLength=0;

int f_cnt=0;
int b_cnt;

while(fNode)
{
     size_t temp_bufferLength = bufferLength;
    fileNameLength = strlen(fNode->fileName);



if((bufferCapacity-bufferLength) >= (fileNameLength+2) )       // extra 2 for comma(delimeter) and nullbyte.
{
       if(bufferLength > 12)
        bufferPtr[bufferLength++] = ',';

    for( f_cnt=0, b_cnt=bufferLength; f_cnt<fileNameLength; /*b_cnt++*/)
    {
         if(fNode->fileName[f_cnt] == '\\')
        {
             if(bufferLength+3 >= bufferCapacity )
            {
                memset(bufferPtr + temp_bufferLength,0,bufferLength-temp_bufferLength);  // clearing partialy wrote last file name as no enogh space avaiable
                bufferLength = temp_bufferLength;
                 return 0;
            }

            else
            {
             strncpy(bufferPtr+b_cnt,"\\\\\0",3);
            f_cnt++;;
            b_cnt+=2;
            bufferLength+=2;
            }
        }

        else if(fNode->fileName[f_cnt] == ',')
        {
                 if(bufferLength+3 >= bufferCapacity )
                {
                     memset(bufferPtr+ temp_bufferLength,0,bufferLength-temp_bufferLength);  // clearing partialy wrote last file name as no enogh space avaiable
                    bufferLength = temp_bufferLength;
                    return 0;
                }

                else
                {
                    strncpy(bufferPtr+b_cnt,"\\,\0",strlen("\\,"));

                    f_cnt++;
                    b_cnt+=2;
                    bufferLength+=2;
                }
        }
else
{
    if(bufferLength+1 > bufferCapacity )
        {
            memset(bufferPtr + temp_bufferLength,0,bufferLength-temp_bufferLength);  // clearing partialy wrote last file name as no enogh space avaiable
            bufferLength = temp_bufferLength;
            return 0;
        }

    bufferPtr[b_cnt] = (fNode->fileName)[f_cnt];
    f_cnt++;
    b_cnt++;
    bufferLength++;
}
}
}
else
{
    return 0;
}
fNode=fNode->next;

memcpy(bufferPtr+4, &fNode,sizeof(struct node *));
}

 return 0;


}





/*

1  failed to open adFilePath  perror();
2	file named adFilePath already exist, want to copy anyway with another name?
3	lseek error occured for adfilepath
4   not enough memory available to copy file on VD
5   failed to access VD
6   failed to create fileNode(to store record of file)
7   Invalied node provided(node is NULL)
8   Failed to insert fileNode in files List(to store record of file)
9   Failed to read() data from adFilepath

*/


int vdcpto(char *adFilePath, char* newFileName) // copy to vd from ad
{


char *vd_fileName = malloc(strlen(newFileName)+1); strcpy(vd_fileName,newFileName);
int ad_fd;
int result=0;

struct stat statOf_adFilePath;
result = stat(adFilePath,&statOf_adFilePath);


if( (result==0) && (statOf_adFilePath.st_mode & S_IFDIR))
{

    free(vd_fileName);
    return 1;
    // DIRECTORY
}

else if((result==0) && (statOf_adFilePath.st_mode & S_IFREG))
{

ad_fd = open(adFilePath,O_RDONLY);
 if((ad_fd==-1) && (access(adFilePath,R_OK)!=0))
{
    free(vd_fileName);
    return 2;
}

}

else {

 free(vd_fileName);
 return 3;
}




struct fileNode *fNode = NULL;
getFileNode(vd_fileName, &fNode);

if(fNode )
{
 free(vd_fileName);
return 4 ;
}


if((lseek(ad_fd,0,SEEK_SET)) ==-1)
{
//printf(" lseek error");//	exit(1);			// debugg

free(vd_fileName);
return 5;
}

off_t sizeOfFile;

if((sizeOfFile = lseek(ad_fd,0,SEEK_END))==-1)
{
//printf(" lseek error");//	exit(1);			// debugg
  free(vd_fileName);
  return 5;
}

if((lseek(ad_fd,0,SEEK_SET)) == -1)		// add validation
{
//	printf(" lseek error");	//exit(1);			// debugg
  free(vd_fileName);
  return 5;
}


unsigned long int *freeBlocksIndexes;

 if(result = getFreeBlocks(sizeOfFile,&freeBlocksIndexes))
 {
 free(vd_fileName);
 return 6;
 	// handle error
 }

/*
4 not enough memory

5 failed to open Virtual Disk %s(diskNames)
6 failed to create fileNode
7 filaed to insert node
8 error occured while reading from adFilePath

10 error occured while performong lseek() in writeDiskBlock()

11 error occured while performing write() in writeDiskBlock()
*/

int vd_fd = open(diskName , O_WRONLY);

if(vd_fd == -1)
{
  free(vd_fileName);
	return 7;
}

unsigned long int requiredNoOfBlocks = sizeOfFile/blockSize + (  (sizeOfFile % blockSize) ? 1 : 0 );


fNode = createFileNode(vd_fileName, freeBlocksIndexes, requiredNoOfBlocks,sizeOfFile);

if(!fNode)
{
  free(vd_fileName);
return 8;
}
result = insert_fileNode(fNode);
if(result)
{
   if(result == 1)
{
free(vd_fileName);
  return 9;}

if(result == 2)
{
  free(vd_fileName);
  return 10;}
}

unsigned long int itr=0;
 ssize_t bytesRead=0;
unsigned char *buffer= malloc(blockSize+1);
ssize_t noOfBytesInLastBlock =  ((sizeOfFile % blockSize) ? (sizeOfFile % blockSize) : 0 );

for(itr=0; itr < requiredNoOfBlocks; itr++)
{
if (read(ad_fd,buffer,blockSize)<=0)
	{
    free(vd_fileName);
    free(buffer);
    return 11;}
 if(result = writeDiskBlock(vd_fd, freeBlocksIndexes[itr], buffer))
{
if(result ==1)
    {
      free(vd_fileName);
      free(buffer);
      return 12;}
if(result ==2)
{
  free(vd_fileName);
  free(buffer);
  return 13;}
}
}


result = compareFile(adFilePath,fNode);


if(result==0)
printf("File \"%s\" copied successfuly as \"%s\".\n",adFilePath,newFileName);            // debugg


close(ad_fd);
close(vd_fd);

free(vd_fileName);
free(buffer);
return 0;

}



int debuggFunc()			/// debugg
{

struct fileNode *fNode = head_fileList;


if(fNode ==  NULL)
{
return 0;
}
int cnt1=1;
printf("---------------\n");
printf("List of files on VD  |   blocks used to store file(index no)   |   file size(in bytes) \n");

while(fNode!=NULL)
{
printf(" * %s\t\t\t\t", fNode->fileName);
int noOfBlocks = ((fNode->size/blockSize) + ((fNode->size%blockSize)? 1 : 0 ) );

if(noOfBlocks==0)
    printf("empty file (no blocks used)");

for (int i = 0; i < noOfBlocks ; ++i)
{
	printf("%ld", fNode->blockNos[i]);
if(i< noOfBlocks-1)
        printf(",");
}
printf("\t\t\t%zu\n",fNode->size );
printf("\n");
fNode=fNode->next;
}

}


int seperatePathAndFileName(char *AD_filePath, char **dirPath, char **fileName)
{

  size_t len = strlen(AD_filePath);
  int i;
  for(i=len-1; i>=0; i--)
  {
  if(AD_filePath[i]=='/')
  break;
  }

  if(i==-1)
  {
    *dirPath = NULL;
  }
  else{
      *dirPath = (char *)malloc(sizeof(char)*(i+2));
      strncpy(*dirPath,AD_filePath,i+1);
      }
  *fileName = (char *)malloc(sizeof(char)*(len-i));
  strncpy(*fileName,AD_filePath+i+1,(len-i));


  return 0;

}

int getFileNameFromFilePath(char* AD_filePath, char **newFileName )
{
*newFileName = AD_filePath;

size_t len = strlen(AD_filePath);
for(int i=len; i>=0; i--)
{
if(AD_filePath[i]=='/')
    *newFileName=&(AD_filePath[i+1]);
}
    return 0;
}


void print_filesList(unsigned char *filesList)
{

int len = strlen(filesList);




printf("\n * ");
for(int cnt=0; cnt<len; cnt++)
{

if(filesList[cnt] == '\\' )
{
if ( filesList[cnt+1]=='\\')
    printf("%c",'\\' );

else if ( filesList[cnt+1]==',')
    printf("%c",',' );

    cnt++;
}

else if(filesList[cnt] == ',' )
  printf("\n * ");

else
    printf("%c", filesList[cnt]);

}

printf("\n");

}


 int compareFile(char *adFilePath, struct fileNode *fNode)
{

struct stat statOf_adFilePath;

int result = stat(adFilePath,&statOf_adFilePath);
int ad_fd;
int vd_fd;

char adBuffer[513];
char vdBuffer[513];

    if(result==-1)
    {
// invalid path , path doesn't exit  to check
        return 1;
    }
    if((result==0) && (statOf_adFilePath.st_mode & S_IFREG))
    {
        ad_fd = open(adFilePath,O_RDONLY);
        if((ad_fd==-1) && (access(adFilePath,R_OK)!=0))
        {
            // dont have read access
            return 2;
        }
    }

vd_fd = open(diskName,O_RDONLY);
if(vd_fd==-1)
{
if((result = stat(adFilePath,&statOf_adFilePath))==-1);
    return 65465;   // VD doesnt exist

   if((vd_fd==-1) && (access(adFilePath,R_OK)!=0))

return 7987;
    // dont have read access
}


if(!fNode)
return 3;
// file doesn't exist on VD.

size_t ad_sizeOfFile = lseek(ad_fd,0,SEEK_END);
size_t vd_sizeOfFile = fNode->size;


if(ad_sizeOfFile==-1)
{
return 4; // lseek() error occured in compareFile();
}

if(lseek(ad_fd,0,SEEK_SET)==-1)
    return 4; // lseek() error occured in compareFile();


if(ad_sizeOfFile != vd_sizeOfFile)
{
return 5; // size is doesn't match
}


unsigned long int noOfBlocks = (fNode->size/blockSize) + ( (fNode->size%512)?  1 : 0  );

int noOfBytesInLastBlock = ((fNode->size%blockSize)? (fNode->size%512) : blockSize)  ;


int blockCnt=0;
int cnt1=0;




for(blockCnt=0; blockCnt<noOfBlocks; blockCnt++)  // readDiskBlock(int fd, int blockNo, unsigned char  *buffer);
{
    if(result = readDiskBlock(vd_fd,(fNode->blockNos)[blockCnt], vdBuffer)  )
    {
         return 123;    //
    }


     result = read(ad_fd,adBuffer,blockSize);
    if( (blockCnt < noOfBlocks-1) && ( result < blockSize || result == -1) )
    {
         return 124;
    }

    else if( (blockCnt == noOfBlocks-1) && ( result != noOfBytesInLastBlock) )
    {
        if(result == -1)
            return 456;
        else
            return 789;

    }

     for(cnt1=0;          cnt1< ((blockCnt < noOfBlocks-1)? blockSize : noOfBytesInLastBlock);     cnt1++)
    {
        if(adBuffer[cnt1] != vdBuffer[cnt1])
           {
            printf("not same byte  blockCnt: %d  cnt1:%d\n",blockCnt,cnt1);
             return 125;
               }

                  }
}


    return 0;
}

int getNewFileName(char *path, char *fileName)
{

}



int main()
{

int result;

if(result = createDisk())
{
printf("Terminating program....\n");
exit(1);
}

char *cli_input = ( char *)malloc(1000);
char *command = (unsigned char *)malloc(10);

char **arguments = (char **)malloc(sizeof(char *)*2);
 for(int i=0;i<2;i++)
 {
    arguments[i] = (char *)malloc(sizeof(char )*500);
 }
int noOfArguments;


printf("Enter command for file system operation\n\n");

while(1)
{

printf("\n$: ");
fgets(cli_input,1000,stdin);



if (isTooLongInput(cli_input))
{
    printf("Too long input! \n");
    continue;
}


if(result = parse_cliInput(cli_input,command,arguments,&noOfArguments))
{

if((result ==1) || (result ==2))
    printf("Invalid \"%s\" command\n",command);

else if(result ==3)
    printf("Invalid \"%s\" command, AD-filePath and VD-fileName operator missing\n",command);

else if(result ==4)
    printf("Invalid \"%s\" command, VD-fileName operator missing\n",command);

else if(result ==5)
    printf("Invalid %s command, too many arguments \n",command);

else if(result ==6)
    printf("Invalid \"%s\" command, file operator missing \n",command);

else if(result ==7)
    printf("Operators buffer exceeded !\n");

else if(result ==8)
    printf("Invalid command\n");

printf("use \"h\" to see list of commands \n");

continue;
    // handle errors

}

 if((strcmp(command,"h")==0) || (strcmp(command,"help")==0))
{

    help();
}


else if(strcmp(command,"vdls")==0)
{
 
if(!head_fileList)
{
printf("VD is empty !\n");
continue;
}

unsigned char *bufferPtr =  (char *)malloc(sizeof(char )*300);
int bufferCapacity = 260;


strncpy((char *)bufferPtr, (char *)(&bufferCapacity), sizeof(  int));
strncpy((char *)bufferPtr+4, (char *)(head_fileList), sizeof( struct fileNode *));

memcpy( (bufferPtr+4),&head_fileList, sizeof(struct fileNode *));


while(  (*((struct fileNode **)(bufferPtr+4))) != NULL )
{
vdls(bufferPtr);

print_filesList(bufferPtr+12);

memset(bufferPtr+12,0,(bufferCapacity-12));
}

}




else if(strcmp(command,"vdcpto")==0)
{

 if(result = vdcpto(arguments[0],arguments[1]))
{

if(result == 1 )
    printf("Invalid ad file path, path leads to the directory.\n");

else if(result == 2 )
    printf("Unable to read file, you dont have read permission on %s\n",arguments[0]);

else if(result == 3 )
    printf("Invalid file path\n");

else if(result == 4 )
    printf("File named \"%s\"  already exist !\n",arguments[1]);

else if(result == 5 )
    printf("lseek() error occured for adfilepath\n");

else if(result == 6 )
    printf("Not enough memory available to copy %s file on VD\n",arguments[1]);

else if(result == 7 )
    printf("Failed to access VD \n");

else if(result == 8 )
    printf("Failed to create fileNode(to store record of file \"%s\")\n",arguments[1]);

else if(result == 9 )
    printf("Invalid node provided to insert_fileNode()(node is NULL)\n");

else if(result == 10 )
    printf("Failed to insert fileNode in files List(to store record of file \"%s\")\n",arguments[1]);

else if(result == 11 )
    printf("Failed to read() data from \"%s\"\n",arguments[0]);

else if(result == 12 )
    printf("Error occured while performong lseek() in writeDiskBlock()\n");

else if(result == 13 )
    printf("Error occured while performing write() in writeDiskBlock()\n");

}

}



else if(strcmp(command,"vdcpfrom")==0)
{

    if(strcmp(arguments[1],"\0")==0)
    {
        strcpy(arguments[1],arguments[0]);
    }

 if(result = vdcpfrom(arguments[0],arguments[1]))
{
if(result == 1)
    printf(" VD is empty ! \n");

else if(result == 2)
    printf("File \"%s\" doesnt exist \n",arguments[1]);

else if(result == 3)
    printf("Failed to accquire fileNode for file %s \n",arguments[0]);

else if(result == 4)
    printf("Failed to create file \"%s\" on AD \n",arguments[1]);

else if(result == 5)
    printf("Error occured while performing lseek() in readDiskBlock() \n");

else if(result == 6)
    printf("Error occured while performing read() in readDiskBlock(); \n");

else if(result == 7)
    printf("Error occured while performing write() in vdcpfrom(); \n");

else if(result ==8)
    printf("Error occured while performing lseek() for last block in readDiskBlock();\n");

else if(result ==9)
    printf("Error occured while performing read() for last block in readDiskBlock();\n");

else if(result ==10)
    printf("Error occured while performing write() for last block in vdcpfrom();\n");


else if(result ==454)
printf(" file \"%s\" already exist! choose different file name \n",arguments[0]);
 
continue;
}

}



else if(strcmp(command,"vdrm")==0)
{

for(int i=0; i<noOfArguments; i++)
{
 result = deleteFileFromDisk(arguments[i]);
/*
getFileNode()

1  VD is empty! (i.e. file doesnt exist)
2  file %s doesn't exist on VD !



remove_fileNode()
3  VD is empty(i.e. file doesnt exist)
4  file %s doesnt't exist VD



5 one of the blocks appeares to be already free (terminate, disk corrupted)
6 failed to free all allocated bloks (terminate, disk corrupted)
7 unnecessary blocks freed  (terminate, disk corrupted)
*/
if(result==0)
    printf("file \"%s\" deleted successfuly.\n",arguments[i]);

else if(result==1)
    printf("VD is empty! (i.e. file doesnt exist on VD)\n");

else if(result==2)
    printf("Can not delete file \"%s\", file doesn't exist on VD !\n",arguments[i]);

else if(result==3)
    printf("VD is empty! (i.e. file doesnt exist on VD)\n");

else if(result==4)
    printf("Can not delete file \"%s\", file doesn't exist on VD !\n",arguments[i]);

else if(result==5)
   {
    printf("VD corrupted, one of the blocks appeares to be already free ()\n");
    printf("Terminating...\n");     exit(1);
   }

else if(result==6)
   {
    printf("VD corrupted, failed to free all allocated blocks \n");
    printf("Terminating...\n");     exit(1);
   }

else if(result==7)
{
    printf("VD corrupted, unnecessary blocks freed\n");
    printf("Terminating...\n"); exit(1);
}

}

}

}

return 0;

}
