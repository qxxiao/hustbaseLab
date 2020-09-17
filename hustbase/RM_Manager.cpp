#include "stdafx.h"
#include "RM_Manager.h"
#include "str.h"
#include<math.h>


//������������ȡ��һ����¼��λ�ã�������rmFileScan�Ѿ���
RC nextRec(RM_FileScan* rmFileScan)
{
	/*if (rmFileScan == nullptr || rmFileScan->bOpen == false)
		return FAIL;*/
	if (rmFileScan->pn == -1 && rmFileScan->sn == -1)
		return RM_EOF;//��ʾû�м�¼��
	int i, j;
	PF_FileHandle* pf = rmFileScan->pRMFileHandle->pfileHandle;
	PF_PageHandle pageHandle;
	pageHandle.bOpen = false;
	for (i = rmFileScan->pn; i <= pf->pFileSubHeader->pageCount; i++)
	{
		if ((pf->pBitmap[i / 8] & (1 << (i % 8))) == 0)//���Կ���ҳ
			continue;
		GetThisPage(pf, i, &pageHandle);
		if (i == rmFileScan->pn)    //������ϴε�ҳ������ҵĲ��±�+1
			j = rmFileScan->sn + 1;
		else
			j = 0;
		for (; j < rmFileScan->pRMFileHandle->fileSubHeader->recordsPerPage; j++)
		{
			if ((pageHandle.pFrame->page.pData[j / 8] & (1 << (j % 8))) == 0)//�ղ�
				continue;
			else
				break;
		}
		UnpinPage(&pageHandle);
		if (j < rmFileScan->pRMFileHandle->fileSubHeader->recordsPerPage)
			break;
	}
	if (i > pf->pFileSubHeader->pageCount)
	{
		rmFileScan->PageHandle.bOpen = false;
		rmFileScan->PageHandle.pFrame = nullptr;
		rmFileScan->pn = -1;
		rmFileScan->sn = -1;

		return RM_EOF;//��ʾû�м�¼��
	}
	else {
		rmFileScan->PageHandle = pageHandle;
		rmFileScan->pn = i;
		rmFileScan->sn = j;
	}
	return SUCCESS;
}
/*��һ���ļ�ɨ��*/
RC OpenScan(RM_FileScan *rmFileScan,RM_FileHandle *fileHandle,int conNum,Con *conditions)//��ʼ��ɨ��
{
	rmFileScan->bOpen = true;
	rmFileScan->pRMFileHandle = fileHandle;//��¼�ļ��ľ��
	rmFileScan->conNum = conNum;
	rmFileScan->conditions = conditions;//û�и���
	//��ʼ��2��-1����ʾ��һ��Ѱ�ҵ�λ��ǰһ��
	rmFileScan->PageHandle.bOpen = false;
	rmFileScan->PageHandle.pFrame = nullptr;
	rmFileScan->pn = 2;
	rmFileScan->sn = -1;
	return SUCCESS;
}

bool selected(Con* pCon, char* pData)
{
	//��������������һ���ģ�ֻ��һ��AttrType
	//�ַ���������ȷ����
	switch (pCon->compOp)
	{
	case EQual:
		if (pCon->attrType == ints)
		{
			int lvalue = pCon->bLhsIsAttr == 1 ? *(int*)(pData + pCon->LattrOffset) : *(int*)(pCon->Lvalue);
			int rvalue = pCon->bRhsIsAttr == 1 ? *(int*)(pData + pCon->RattrOffset) : *(int*)(pCon->Rvalue);
			return lvalue == rvalue;
		}
		else if (pCon->attrType == floats)
		{
			float lvalue = pCon->bLhsIsAttr == 1 ? *(float*)(pData + pCon->LattrOffset) : *(float*)(pCon->Lvalue);
			float rvalue = pCon->bRhsIsAttr == 1 ? *(float*)(pData + pCon->RattrOffset) : *(float*)(pCon->Rvalue);
			return lvalue == rvalue;
		}
		else {
			char* lvalue = pCon->bLhsIsAttr == 1 ? pData + pCon->LattrOffset : (char*)(pCon->Lvalue);
			char* rvalue = pCon->bRhsIsAttr == 1 ? pData + pCon->RattrOffset : (char*)(pCon->Rvalue);
			return strcmp(lvalue, rvalue) == 0;
		}
			break;
	case NEqual:
		if (pCon->attrType == ints)
		{
			int lvalue = pCon->bLhsIsAttr == 1 ? *(int*)(pData + pCon->LattrOffset) : *(int*)(pCon->Lvalue);
			int rvalue = pCon->bRhsIsAttr == 1 ? *(int*)(pData + pCon->RattrOffset) : *(int*)(pCon->Rvalue);
			return lvalue != rvalue;
		}
		else if (pCon->attrType == floats)
		{
			float lvalue = pCon->bLhsIsAttr == 1 ? *(float*)(pData + pCon->LattrOffset) : *(float*)(pCon->Lvalue);
			float rvalue = pCon->bRhsIsAttr == 1 ? *(float*)(pData + pCon->RattrOffset) : *(float*)(pCon->Rvalue);
			return lvalue != rvalue;
		}
		else {
			char* lvalue = pCon->bLhsIsAttr == 1 ? pData + pCon->LattrOffset : (char*)(pCon->Lvalue);
			char* rvalue = pCon->bRhsIsAttr == 1 ? pData + pCon->RattrOffset : (char*)(pCon->Rvalue);
			return strcmp(lvalue, rvalue) != 0;
		}
		break;
	case LessT:
		if (pCon->attrType == ints)
		{
			int lvalue = pCon->bLhsIsAttr == 1 ? *(int*)(pData + pCon->LattrOffset) : *(int*)(pCon->Lvalue);
			int rvalue = pCon->bRhsIsAttr == 1 ? *(int*)(pData + pCon->RattrOffset) : *(int*)(pCon->Rvalue);
			return lvalue < rvalue;
		}
		else if (pCon->attrType == floats)
		{
			float lvalue = pCon->bLhsIsAttr == 1 ? *(float*)(pData + pCon->LattrOffset) : *(float*)(pCon->Lvalue);
			float rvalue = pCon->bRhsIsAttr == 1 ? *(float*)(pData + pCon->RattrOffset) : *(float*)(pCon->Rvalue);
			return lvalue < rvalue;
		}
		else {
			char* lvalue = pCon->bLhsIsAttr == 1 ? pData + pCon->LattrOffset : (char*)(pCon->Lvalue);
			char* rvalue = pCon->bRhsIsAttr == 1 ? pData + pCon->RattrOffset : (char*)(pCon->Rvalue);
			return strcmp(lvalue, rvalue) < 0;
		}
		break;
	case LEqual:
		if (pCon->attrType == ints)
		{
			int lvalue = pCon->bLhsIsAttr == 1 ? *(int*)(pData + pCon->LattrOffset) : *(int*)(pCon->Lvalue);
			int rvalue = pCon->bRhsIsAttr == 1 ? *(int*)(pData + pCon->RattrOffset) : *(int*)(pCon->Rvalue);
			return lvalue <= rvalue;
		}
		else if (pCon->attrType == floats)
		{
			float lvalue = pCon->bLhsIsAttr == 1 ? *(float*)(pData + pCon->LattrOffset) : *(float*)(pCon->Lvalue);
			float rvalue = pCon->bRhsIsAttr == 1 ? *(float*)(pData + pCon->RattrOffset) : *(float*)(pCon->Rvalue);
			return lvalue <= rvalue;
		}
		else {
			char* lvalue = pCon->bLhsIsAttr == 1 ? pData + pCon->LattrOffset : (char*)(pCon->Lvalue);
			char* rvalue = pCon->bRhsIsAttr == 1 ? pData + pCon->RattrOffset : (char*)(pCon->Rvalue);
			return strcmp(lvalue, rvalue) <= 0;
		}
		break;
	case GreatT:
		if (pCon->attrType == ints)
		{
			int lvalue = pCon->bLhsIsAttr == 1 ? *(int*)(pData + pCon->LattrOffset) : *(int*)(pCon->Lvalue);
			int rvalue = pCon->bRhsIsAttr == 1 ? *(int*)(pData + pCon->RattrOffset) : *(int*)(pCon->Rvalue);
			return lvalue > rvalue;
		}
		else if (pCon->attrType == floats)
		{
			float lvalue = pCon->bLhsIsAttr == 1 ? *(float*)(pData + pCon->LattrOffset) : *(float*)(pCon->Lvalue);
			float rvalue = pCon->bRhsIsAttr == 1 ? *(float*)(pData + pCon->RattrOffset) : *(float*)(pCon->Rvalue);
			return lvalue > rvalue;
		}
		else {
			char* lvalue = pCon->bLhsIsAttr == 1 ? pData + pCon->LattrOffset : (char*)(pCon->Lvalue);
			char* rvalue = pCon->bRhsIsAttr == 1 ? pData + pCon->RattrOffset : (char*)(pCon->Rvalue);
			return strcmp(lvalue, rvalue) > 0;
		}
		break;
	case GEqual:
		if (pCon->attrType == ints)
		{
			int lvalue = pCon->bLhsIsAttr == 1 ? *(int*)(pData + pCon->LattrOffset) : *(int*)(pCon->Lvalue);
			int rvalue = pCon->bRhsIsAttr == 1 ? *(int*)(pData + pCon->RattrOffset) : *(int*)(pCon->Rvalue);
			return lvalue >= rvalue;
		}
		else if (pCon->attrType == floats)
		{
			float lvalue = pCon->bLhsIsAttr == 1 ? *(float*)(pData + pCon->LattrOffset) : *(float*)(pCon->Lvalue);
			float rvalue = pCon->bRhsIsAttr == 1 ? *(float*)(pData + pCon->RattrOffset) : *(float*)(pCon->Rvalue);
			return lvalue >= rvalue;
		}
		else {
			char* lvalue = pCon->bLhsIsAttr == 1 ? pData + pCon->LattrOffset : (char*)(pCon->Lvalue);
			char* rvalue = pCon->bRhsIsAttr == 1 ? pData + pCon->RattrOffset : (char*)(pCon->Rvalue);
			return strcmp(lvalue, rvalue) > 0;
		}
		break;
	case NO_OP:
		break;
	default:
		break;
	}
	return false;
}

/*��ȡһ������ɨ�������ļ�¼������÷����ɹ�������ֵrecӦ������¼��������¼��ʶ��
���û�з�������ɨ�������ļ�¼���򷵻�RM_EOF*/
RC GetNextRec(RM_FileScan *rmFileScan,RM_Record *rec)
{
	RC tmp;
	int offset = rmFileScan->pRMFileHandle->fileSubHeader->firstRecordOffset;
	int size = rmFileScan->pRMFileHandle->fileSubHeader->recordSize;
	char* pData = nullptr;
	while ((tmp = nextRec(rmFileScan)) == SUCCESS) //�ҵ�һ����¼����pn,sn)���жϷ���Ҫ��
	{
		//���Ը�ҳ�е�sn��¼�Ƿ������������飬���Ϸ���
		//�����ϣ��޸�rmFileScan�ҵ���һ����¼,ʹ��ѭ���ҵ����ϵ�һ�������򷵻�RM_EOF;
		GetThisPage(rmFileScan->pRMFileHandle->pfileHandle, rmFileScan->pn, &rmFileScan->PageHandle);
		pData = rmFileScan->PageHandle.pFrame->page.pData + offset + rmFileScan->sn * size;//���ܵ����ݵ�ַ
		if (rmFileScan->conNum == 0)
			break;
		int i;
		for (i = 0; i < rmFileScan->conNum; i++)
		{
			Con* pCon = rmFileScan->conditions + i;
			if (!selected(pCon, pData))
			{
				UnpinPage(&rmFileScan->PageHandle);
				break;//Ҫȫ������
			}
		}
		if(i >= rmFileScan->conNum)//��������������ѡ����һ����¼
			break;
	}
	if (tmp == RM_EOF)
		return RM_EOF;
	rec->bValid = true;
	//����ֱ�Ӹ�ֵpData
	//memcpy(rec->pData, pData, size);
	rec->pData = pData;
	UnpinPage(&rmFileScan->PageHandle);
	rec->rid.bValid = true;
	rec->rid.pageNum = rmFileScan->pn;
	rec->rid.slotNum = rmFileScan->sn;
	return SUCCESS;
}

/*�ر�һ���ļ�ɨ�裬�ͷ���Ӧ����Դ*/
RC CloseScan(RM_FileScan* rmFileScan)
{
	rmFileScan->pn = rmFileScan->sn = -1;
	rmFileScan->PageHandle.bOpen = false;
	rmFileScan->PageHandle.pFrame = nullptr;
	rmFileScan->bOpen = false;
	return SUCCESS;
}

RC GetRec (RM_FileHandle *fileHandle,RID *rid, RM_Record *rec) 
{
	RC tmp;
	PF_FileHandle* pf = fileHandle->pfileHandle;//��Ӧҳ���ļ��ľ��
	if (rid->pageNum <=1 || rid->pageNum > pf->pFileSubHeader->pageCount)//������ҳ��
		return RM_INVALIDRID;
	if ((pf->pBitmap[rid->pageNum / 8] & (1 << (rid->pageNum % 8))) == 0)//����ҳ���յģ�����ʧ��
		return RM_INVALIDRID;
	PF_PageHandle pageHandle;
	pageHandle.bOpen = false;
	if ((tmp = GetThisPage(pf, rid->pageNum, &pageHandle)) != SUCCESS)
		return tmp;
	if (((pageHandle.pFrame->page.pData[rid->slotNum / 8] & (1 << (rid->slotNum % 8))) == 0))//���δʹ��
		return RM_INVALIDRID;

	int offset = ((RM_FileSubHeader*)fileHandle->pHdrPage->pData)->firstRecordOffset;
	int size = ((RM_FileSubHeader*)fileHandle->pHdrFrame->page.pData)->recordSize;
	rec->bValid = true;
	rec->rid = *rid;
	rec->rid.bValid = true;
	rec->pData = pageHandle.pFrame->page.pData + offset + (rid->slotNum) * size;//ֻ���ؼ�¼��ַ
	//memcpy(rec->pData, pageHandle.pFrame->page.pData + offset + (rid->slotNum) * size, size);//��������
	UnpinPage(&pageHandle);
	return SUCCESS;
}

RC InsertRec (RM_FileHandle *fileHandle,char *pData, RID *rid)
{
	RC tmp;
	int i, j, k;
	bool isFull = false;//���������¼���ҳ�Ƿ�����
	PF_FileHandle* pf = fileHandle->pfileHandle;
	for (i = 0; i <= pf->pFileSubHeader->pageCount; i++)
	{
		//�ҵ���һ������ҳ
		if (((pf->pBitmap[i / 8] & (1 << (i % 8))) != 0) && ((fileHandle->pBitmap[i / 8] & (1 << (i % 8))) == 0))
			break;
	}
	int offset= ((RM_FileSubHeader*)fileHandle->pHdrPage->pData)->firstRecordOffset;
	int size= ((RM_FileSubHeader*)fileHandle->pHdrFrame->page.pData)->recordSize;
	int recordsPerPage = ((RM_FileSubHeader*)fileHandle->pHdrFrame->page.pData)->recordsPerPage;
	PF_PageHandle pageHandle;//�ֲ�����
	pageHandle.bOpen = false;
	if (i <= pf->pFileSubHeader->pageCount)//��δ����ҳ
	{
		GetThisPage(pf, i, &pageHandle);
		for (j = 0; j < recordsPerPage; j++)
		{
			if ((pageHandle.pFrame->page.pData[j / 8] & (1 << (j % 8))) == 0)
				break;
		}
		for (k = j+1; k < recordsPerPage; k++)
		{
			if ((pageHandle.pFrame->page.pData[k / 8] & (1 << (k % 8))) == 0)
				break;
		}
		if (k >= recordsPerPage)
			isFull = true;
		//����Ӧ�Ĳ�λ��Ϊ1
		pageHandle.pFrame->page.pData[j / 8] |= (1 << (j % 8));
		memcpy(pageHandle.pFrame->page.pData + offset + j * size, pData, size);
		UnpinPage(&pageHandle);
		MarkDirty(&pageHandle);
		rid->bValid = true;
		rid->pageNum = i;
		rid->slotNum = j;
	}
	else{
		AllocatePage(pf, &pageHandle);
		pageHandle.pFrame->page.pData[0] |= 1;
		if (recordsPerPage == 1)
			isFull = true;
		memcpy(pageHandle.pFrame->page.pData + offset, pData, size);
		MarkDirty(&pageHandle);
		UnpinPage(&pageHandle);
		rid->bValid = true;
		rid->pageNum = pageHandle.pFrame->page.pageNum;
		rid->slotNum = 0;
	}
	fileHandle->fileSubHeader->nRecords++;
	fileHandle->pHdrFrame->bDirty = true;
	//��isFull==true,��λʾͼ��Ӧλ��Ϊ1
	if (isFull)
	{
		int pageNum = pageHandle.pFrame->page.pageNum;
		fileHandle->pBitmap[pageNum / 8] |= (1 << (pageNum % 8));
		fileHandle->pfileHandle->pHdrFrame->bDirty = true;
	}
	return SUCCESS;
}

RC DeleteRec (RM_FileHandle *fileHandle,const RID *rid)
{
	RC tmp;
	PF_FileHandle* pf = fileHandle->pfileHandle;//��Ӧҳ���ļ��ľ��
	if (rid->pageNum <= 1 || rid->pageNum > pf->pFileSubHeader->pageCount)//������ҳ��
		return RM_INVALIDRID;
	if ((pf->pBitmap[rid->pageNum / 8] & (1 << (rid->pageNum % 8))) == 0)//����ҳ���յģ�����ʧ��
		return RM_INVALIDRID;
	PF_PageHandle pageHandle;
	pageHandle.bOpen = false;
	if ((tmp = GetThisPage(pf, rid->pageNum, &pageHandle)) != SUCCESS)
		return tmp;
	if (((pageHandle.pFrame->page.pData[rid->slotNum / 8] & (1 << (rid->slotNum % 8))) == 0))//���δʹ��
		return RM_INVALIDRID;
	//��λ��0����Ϊ����ҳ0���ܼ�¼��-1
	pageHandle.pFrame->page.pData[rid->slotNum / 8] &= (~(1 << (rid->slotNum % 8)));
	fileHandle->fileSubHeader->nRecords--;
	fileHandle->pBitmap[rid->pageNum / 8] &= (~(1 << (rid->pageNum % 8)));
	MarkDirty(&pageHandle);
	fileHandle->pHdrFrame->bDirty = true;//û�й���pageHandle,ֱ����Ϊdirty
	UnpinPage(&pageHandle);
	return SUCCESS;
}

RC UpdateRec (RM_FileHandle *fileHandle,const RM_Record *rec)
{
	RC tmp;
	PF_FileHandle* pf = fileHandle->pfileHandle;//��Ӧҳ���ļ��ľ��
	if (rec->rid.pageNum <= 1 || rec->rid.pageNum > pf->pFileSubHeader->pageCount)//������ҳ��
		return RM_INVALIDRID;
	if ((pf->pBitmap[rec->rid.pageNum / 8] & (1 << (rec->rid.pageNum % 8))) == 0)//����ҳ���յģ�����ʧ��
		return RM_INVALIDRID;
	PF_PageHandle pageHandle;
	pageHandle.bOpen = false;
	if ((tmp = GetThisPage(pf, rec->rid.pageNum, &pageHandle)) != SUCCESS)
		return tmp;
	if (((pageHandle.pFrame->page.pData[rec->rid.slotNum / 8] & (1 << (rec->rid.slotNum % 8))) == 0))//���δʹ��
		return RM_INVALIDRID;
	//�޸ĸ�����¼
	int offset = ((RM_FileSubHeader*)fileHandle->pHdrPage->pData)->firstRecordOffset;
	int size = ((RM_FileSubHeader*)fileHandle->pHdrFrame->page.pData)->recordSize;
	memcpy(pageHandle.pFrame->page.pData + offset + rec->rid.slotNum * size, rec->pData, size);
	MarkDirty(&pageHandle);
	UnpinPage(&pageHandle);
	return SUCCESS;
}

//ֱ�ӵ��õĻ������ڽ��в�����ر�
RC RM_CreateFile (char *fileName, int recordSize)
{
	RC tmp;
	char* bitmap;
	int slotsSum = (int)(((PF_PAGE_SIZE - 1) / (recordSize + 1.0 / 8)));
	if (slotsSum <= 0)
		return RM_INVALIDRECSIZE;
	RM_FileSubHeader *fileSubHeader;
	if ((tmp = CreateFile(fileName)) != SUCCESS)
		return tmp;
	//��ҳ���ļ�����
	PF_FileHandle* fileHandle = (PF_FileHandle*)malloc(sizeof(PF_FileHandle));
	fileHandle->bopen = false;
	if ((tmp = openFile(fileName, fileHandle)) != SUCCESS)
		return tmp;
	PF_PageHandle* pageHandle = (PF_PageHandle*)malloc(sizeof(PF_PageHandle));
	pageHandle->bOpen = false;
	if ((tmp = AllocatePage(fileHandle, pageHandle)) != SUCCESS)
		return tmp;

	//int slotsSum = (int)(((PF_PAGE_SIZE - 1) / (recordSize + 1.0 / 8)));
	fileSubHeader = (RM_FileSubHeader*)pageHandle->pFrame->page.pData;
	fileSubHeader->nRecords = 0;
	fileSubHeader->recordSize = recordSize;
	fileSubHeader->recordsPerPage = slotsSum;
	fileSubHeader->firstRecordOffset = (int)ceil(slotsSum / 8.0);//ÿҳ��һ����¼����������ƫ��
	bitmap = pageHandle->pFrame->page.pData + (int)sizeof(RM_FileSubHeader);
	bitmap[0] |= 0x03;//��ʾ0��1ҳ����������װ��¼
	MarkDirty(pageHandle);
	if ((tmp = CloseFile(fileHandle)) != SUCCESS)
		return tmp;
	free(pageHandle);
	free(fileHandle);
	return SUCCESS;
}

RC RM_OpenFile(char *fileName, RM_FileHandle *fileHandle)
{
	RC tmp;
	PF_FileHandle* pf = (PF_FileHandle*)malloc(sizeof(PF_FileHandle));
	if (pf == NULL)
		return FAIL;
	pf->bopen = false;
	if ((tmp = openFile(fileName, pf)) != SUCCESS)
		return tmp;
	PF_PageHandle* pageHandle = (PF_PageHandle*)malloc(sizeof(PF_PageHandle));
	pageHandle->bOpen = false;
	if ((tmp = GetThisPage(pf, 1, pageHandle)) != SUCCESS)
	{
		CloseFile(pf);
		return tmp;
	}
	fileHandle->bOpen = true;
	fileHandle->fileName = fileName;//�ļ�������ָ�룬û�п����ַ���
	fileHandle->pfileHandle = pf;	//�ͷž��ʱ��Ӧ���ͷŸ�ҳ���ļ��ľ���ռ�
	fileHandle->pHdrFrame = pageHandle->pFrame;
	fileHandle->pHdrPage = &(pageHandle->pFrame->page);
	fileHandle->pBitmap = pageHandle->pFrame->page.pData + sizeof(RM_FileSubHeader);
	fileHandle->fileSubHeader = (RM_FileSubHeader *)pageHandle->pFrame->page.pData;
	
	free(pageHandle);//�����ͷ�
	return SUCCESS;
}

RC RM_CloseFile(RM_FileHandle *fileHandle)
{
	RC tmp;
	fileHandle->pHdrFrame->pinCount--;
	if ((tmp = CloseFile(fileHandle->pfileHandle)) != SUCCESS)
	{
		free(fileHandle->pfileHandle);
		return tmp;
	}
	free(fileHandle->pfileHandle);
	fileHandle->bOpen = false;
	return SUCCESS;
}
