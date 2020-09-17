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

#define PF_PAGE_SIZE ((1<<12)-4)  //ҳ����������С��ҳ��Ϊ4KB-4,ÿҳ��ȥ4�ֽڴ��ҳ�ŵ������������4092B
#define PF_FILESUBHDR_SIZE (sizeof(PF_FileSubHeader))
#define PF_BUFFER_SIZE 50        //ҳ�滺�����Ĵ�С
#define PF_PAGESIZE (1<<12)		 //ҳ���С
typedef unsigned int PageNum;

typedef struct{
	PageNum pageNum;
	char pData[PF_PAGE_SIZE];
}Page;							//ҳ��ṹ

typedef struct{
	PageNum pageCount;			//βҳ��ҳ��
	int nAllocatedPages;		//�ѷ����ҳ������������ҳ0��
}PF_FileSubHeader;				//��0ҳ�еķ�����Ϣ�ṹ��

typedef struct{
	bool bDirty;				//��ҳ�������Ƿ��޸�
	unsigned int pinCount;		//���ʵ�ǰҳ����߳���
	clock_t  accTime;			//ҳ��������ʱ��
	char *fileName;				//����ҳ�����ڵ��ļ���
	int fileDesc;				//�ļ�������
	Page page;					//Ӳ���л�ȡ��ʵ��ҳ��
}Frame;

typedef struct{
	bool bopen;		   //�þ���Ƿ��һ���ļ�����---�Ƿ��
	char *fileName;	   //�������ļ���
	int fileDesc;      //�������ļ�������
	Frame *pHdrFrame;  //ָ����ļ�ͷ֡������ҳ��Ӧ��֡����ָ��
	Page *pHdrPage;	   //ָ����ļ�ͷҳ������ҳ����ָ��
	char *pBitmap;	   //ָ�����ҳ��λͼ��ָ��
	PF_FileSubHeader *pFileSubHeader;//���ļ��ĵ�0ҳ�ķ�����Ϣ�ṹ��
}PF_FileHandle;			//ҳ���ļ����

typedef struct{
	int nReads;
	int nWrites;
	Frame frame[PF_BUFFER_SIZE]; //��������ŵ�֡
	bool allocated[PF_BUFFER_SIZE];
}BF_Manager;		    //ҳ�滺��������

typedef struct{
	bool bOpen;
	Frame *pFrame;
}PF_PageHandle;        //ҳ���Ӧ�ľ��

const RC CreateFile(const char *fileName);
const RC openFile(char *fileName,PF_FileHandle *fileHandle);
const RC CloseFile(PF_FileHandle *fileHandle);

const RC GetThisPage(PF_FileHandle *fileHandle,PageNum pageNum,PF_PageHandle *pageHandle);//��ȡ�ļ��ĵ�pageNumҳ
const RC AllocatePage(PF_FileHandle *fileHandle,PF_PageHandle *pageHandle);
const RC GetPageNum(PF_PageHandle *pageHandle,PageNum *pageNum);

const RC GetData(PF_PageHandle *pageHandle,char **pData);
const RC DisposePage(PF_FileHandle *fileHandle,PageNum pageNum);

const RC MarkDirty(PF_PageHandle *pageHandle);//�����ҳ--�޸Ĺ�

const RC UnpinPage(PF_PageHandle *pageHandle);//ȡ��פ��������-1

#endif