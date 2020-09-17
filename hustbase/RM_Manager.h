#ifndef RM_MANAGER_H_H
#define RM_MANAGER_H_H

#include "PF_Manager.h"
#include "str.h"

//��¼��Ϣ����ҳ,�е���Ϣ�ṹ
typedef struct {
	int nRecords;		//��ǰ��¼�ļ��ļ�¼����
	int recordSize;		//ÿ����¼�Ĵ�С
	int recordsPerPage; //ÿ��ҳ�����װ�صļ�¼������
	int firstRecordOffset;//ÿҳ��һ����¼���������еĿ�ʼλ��
}RM_FileSubHeader;


typedef int SlotNum;

typedef struct {	
	PageNum pageNum;	//��¼����ҳ��ҳ��
	SlotNum slotNum;		//��¼�Ĳ�ۺ�
	bool bValid; 			//true��ʾΪһ����Ч��¼�ı�ʶ��
}RID;					    //��¼ID

typedef struct{
	bool bValid;		 // False��ʾ��δ�������¼
	RID  rid; 			 // ��¼�ı�ʶ�� 
	char *pData; 		 //��¼���洢������ 
}RM_Record;

//�����ṹ�壬���浥��ν������
typedef struct
{
	int bLhsIsAttr,bRhsIsAttr;//���������ԣ�1������ֵ��0��
	AttrType attrType;		  //�������е����ݵ�����
	int LattrLength,RattrLength;//�������ԵĻ�����ʾ���Եĳ���
	int LattrOffset,RattrOffset;//�������ԵĻ�����ʾ���Ե�ƫ����
	CompOp compOp;				//�Ƚϲ�����
	void *Lvalue,*Rvalue;		//����ֵ�Ļ���ָ���Ӧ��ֵ
}Con;

typedef struct{//�ļ����
	bool bOpen;//����Ƿ�򿪣��Ƿ����ڱ�ʹ�ã�
	
	char* fileName;//�þ���������ļ���
	PF_FileHandle* pfileHandle;//������ҳ���ļ����
	Frame *pHdrFrame;//�ü�¼�ļ���ͷ֡����¼�ļ��Ŀ���ҳ�ģ���ָ��
	Page* pHdrPage;  //�ü�¼�ļ���ͷҳ����¼�ļ��Ŀ���ҳ����ָ��
	char* pBitmap;	 //��¼�ļ��Ŀ���ҳ��λͼ��ָ��
	RM_FileSubHeader* fileSubHeader;//��¼�ļ�����������/�ṹ��Ϣ
}RM_FileHandle;

typedef struct{
	bool  bOpen;		//ɨ���Ƿ�� 
	RM_FileHandle  *pRMFileHandle;		//ɨ��ļ�¼�ļ����
	int  conNum;		//ɨ���漰���������� 
	Con  *conditions;	//ɨ���漰����������ָ��
    PF_PageHandle  PageHandle; //�����е�ҳ����
	PageNum  pn; 	    //ɨ�輴�������ҳ���
	SlotNum  sn;		//ɨ�輴������Ĳ�ۺ�
}RM_FileScan;



RC GetNextRec(RM_FileScan *rmFileScan,RM_Record *rec);

RC OpenScan(RM_FileScan *rmFileScan,RM_FileHandle *fileHandle,int conNum,Con *conditions);

RC CloseScan(RM_FileScan *rmFileScan);

RC UpdateRec (RM_FileHandle *fileHandle,const RM_Record *rec);

RC DeleteRec (RM_FileHandle *fileHandle,const RID *rid);

RC InsertRec (RM_FileHandle *fileHandle, char *pData, RID *rid); 

RC GetRec (RM_FileHandle *fileHandle, RID *rid, RM_Record *rec); 

RC RM_CloseFile (RM_FileHandle *fileHandle);

RC RM_OpenFile (char *fileName, RM_FileHandle *fileHandle);

RC RM_CreateFile (char *fileName, int recordSize);

#endif