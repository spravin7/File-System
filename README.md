# File-System
file system using C


              Commands



h | help                                       // print list of commands
vdls                                           // print list of files on VD
vdcpto [AD FILEPATH] [NEW FILE NAME AFTER COPIED ON VD]   // copy file from AD to VD
vdcpfrom [AD FILEPATH] [VD FILE NAME]                     // copy file from VD to AD
vdrm [FILE] ......                                        // delete file/s from VD




-------------------------------------------------------------------------------------------------------------------------------------------------



abbrevation used 
	AD : Actual Disk(SSD/HDD)
	VD : Virtual Disk created for this program.




disk.tesage is created at the start of programm.


disk is divided in blocks, here block size is 512 bytes


------------------

	Following structure(linked list) is used to store file and it's metadata.

struct fileNode
{

char *fileName;
unsigned long int *blockNos; 	// array of block indexes(by sequence) used to stored file on VD.

size_t size;				// size of file in bytes

struct fileNode *next;
};

struct fileNode *head_fileList=NULL; 


---

	structure used to maintain logs of allocated and free blocks available.

struct freeBlocksDetails{

short *blocksStatus;			// 1 : block is free  0 : block is allocated
unsigned long int availableFreeBlocks; //  no of available free blocks
};

struct freeBlocksDetails *freeBlocksNode;



-----------------


copying file to VD




	- vdcpto fileSourcePath  fileDestPath

	function  getFreeBlocks(size_t fileSize, unsigned long int **freeBlocksIndexes) is called to get freeBlocksIndexes (i.e. get indexes of blocks those are free to write)
	and blocks which are assigned for new file are marked as allocated blocks(i.e.flag changed for those blocks from 1 to 0 )
	and availableFreeBlocks is reduced by block used by new file.
	
	- File node is created using  
	struct fileNode *createFileNode(char *fileName, unsigned long int *blockNos, unsigned long int requiredNoOfBlocks ,size_t size);
	
	and inserted into linked List of fileNode (i.e. struct fileNode *head_fileList )

 	- Function      int writeDiskBlock(int fd, int blockNo, unsigned char *buffer);  used to write file on VD blockwise(512bytes)
 		blocks
 	
 	
