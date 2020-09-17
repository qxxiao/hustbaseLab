#ifndef PF_MANAGER_H_H
#define PF_MANAGER_H_H

#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>

#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <malloc.h>
#include <windows.h>

#include "RC.h"

#define PF_PAGE_SIZE ((1<<12)-4)  //页面数据区大小，页面为4KB-4,每页除去4字节存放页号的数据区域带下4092B
#define PF_FILESUBHDR_SIZE (sizeof(PF_FileSubHeader))
#define PF_BUFFER_SIZE 50        //页面缓冲区的大小
#define PF_PAGESIZE (1<<12)		 //页面大小
typedef unsigned int PageNum;

typedef struct{
	PageNum pageNum;
	char pData[PF_PAGE_SIZE];
}Page;							//页面结构

typedef struct{
	PageNum pageCount;			//尾页的页号
	int nAllocatedPages;		//已分配的页数（包括控制页0）
}PF_FileSubHeader;				//第0页中的分配信息结构体

typedef struct{
	bool bDirty;				//该页面内容是否被修改
	unsigned int pinCount;		//访问当前页面的线程数
	clock_t  accTime;			//页面最后访问时间
	char *fileName;				//访问页面所在的文件名
	int fileDesc;				//文件描述符
	Page page;					//硬盘中获取的实际页面
}Frame;

typedef struct{
	bool bopen;		   //该句柄是否和一个文件关联---是否打开
	char *fileName;	   //关联的文件名
	int fileDesc;      //关联的文件描述符
	Frame *pHdrFrame;  //指向该文件头帧（控制页对应的帧）的指针
	Page *pHdrPage;	   //指向该文件头页（控制页）的指针
	char *pBitmap;	   //指向控制页中位图的指针
	PF_FileSubHeader *pFileSubHeader;//该文件的第0页的分配信息结构体
}PF_FileHandle;			//页面文件句柄

typedef struct{
	int nReads;
	int nWrites;
	Frame frame[PF_BUFFER_SIZE]; //缓冲区存放的帧
	bool allocated[PF_BUFFER_SIZE];
}BF_Manager;		    //页面缓冲区管理

typedef struct{
	bool bOpen;
	Frame *pFrame;
}PF_PageHandle;        //页面对应的句柄

const RC CreateFile(const char *fileName);
const RC openFile(char *fileName,PF_FileHandle *fileHandle);
const RC CloseFile(PF_FileHandle *fileHandle);

const RC GetThisPage(PF_FileHandle *fileHandle,PageNum pageNum,PF_PageHandle *pageHandle);//获取文件的第pageNum页
const RC AllocatePage(PF_FileHandle *fileHandle,PF_PageHandle *pageHandle);
const RC GetPageNum(PF_PageHandle *pageHandle,PageNum *pageNum);

const RC GetData(PF_PageHandle *pageHandle,char **pData);
const RC DisposePage(PF_FileHandle *fileHandle,PageNum pageNum);

const RC MarkDirty(PF_PageHandle *pageHandle);//标记脏页--修改过

const RC UnpinPage(PF_PageHandle *pageHandle);//取消驻留缓冲区-1

#endif