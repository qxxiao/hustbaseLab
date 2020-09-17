#include "stdafx.h"
#include "RM_Manager.h"
#include "str.h"
#include<math.h>


//辅助函数，获取下一条记录的位置，条件：rmFileScan已经打开
RC nextRec(RM_FileScan* rmFileScan)
{
	/*if (rmFileScan == nullptr || rmFileScan->bOpen == false)
		return FAIL;*/
	if (rmFileScan->pn == -1 && rmFileScan->sn == -1)
		return RM_EOF;//表示没有记录了
	int i, j;
	PF_FileHandle* pf = rmFileScan->pRMFileHandle->pfileHandle;
	PF_PageHandle pageHandle;
	pageHandle.bOpen = false;
	for (i = rmFileScan->pn; i <= pf->pFileSubHeader->pageCount; i++)
	{
		if ((pf->pBitmap[i / 8] & (1 << (i % 8))) == 0)//忽略空闲页
			continue;
		GetThisPage(pf, i, &pageHandle);
		if (i == rmFileScan->pn)    //如果是上次的页，则查找的槽下标+1
			j = rmFileScan->sn + 1;
		else
			j = 0;
		for (; j < rmFileScan->pRMFileHandle->fileSubHeader->recordsPerPage; j++)
		{
			if ((pageHandle.pFrame->page.pData[j / 8] & (1 << (j % 8))) == 0)//空槽
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

		return RM_EOF;//表示没有记录了
	}
	else {
		rmFileScan->PageHandle = pageHandle;
		rmFileScan->pn = i;
		rmFileScan->sn = j;
	}
	return SUCCESS;
}
/*打开一个文件扫描*/
RC OpenScan(RM_FileScan *rmFileScan,RM_FileHandle *fileHandle,int conNum,Con *conditions)//初始化扫描
{
	rmFileScan->bOpen = true;
	rmFileScan->pRMFileHandle = fileHandle;//记录文件的句柄
	rmFileScan->conNum = conNum;
	rmFileScan->conditions = conditions;//没有复制
	//初始化2，-1，表示第一次寻找的位置前一个
	rmFileScan->PageHandle.bOpen = false;
	rmFileScan->PageHandle.pFrame = nullptr;
	rmFileScan->pn = 2;
	rmFileScan->sn = -1;
	return SUCCESS;
}

bool selected(Con* pCon, char* pData)
{
	//假设左右类型是一样的，只有一个AttrType
	//字符串都已正确处理
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

/*获取一个符合扫描条件的记录。如果该方法成功，返回值rec应包含记录副本及记录标识符
如果没有发现满足扫描条件的记录，则返回RM_EOF*/
RC GetNextRec(RM_FileScan *rmFileScan,RM_Record *rec)
{
	RC tmp;
	int offset = rmFileScan->pRMFileHandle->fileSubHeader->firstRecordOffset;
	int size = rmFileScan->pRMFileHandle->fileSubHeader->recordSize;
	char* pData = nullptr;
	while ((tmp = nextRec(rmFileScan)) == SUCCESS) //找到一条记录，（pn,sn)并判断符合要求
	{
		//测试该页中的sn记录是否满足条件数组，符合返回
		//不符合，修改rmFileScan找到下一条记录,使用循环找到符合的一条，否则返回RM_EOF;
		GetThisPage(rmFileScan->pRMFileHandle->pfileHandle, rmFileScan->pn, &rmFileScan->PageHandle);
		pData = rmFileScan->PageHandle.pFrame->page.pData + offset + rmFileScan->sn * size;//可能的数据地址
		if (rmFileScan->conNum == 0)
			break;
		int i;
		for (i = 0; i < rmFileScan->conNum; i++)
		{
			Con* pCon = rmFileScan->conditions + i;
			if (!selected(pCon, pData))
			{
				UnpinPage(&rmFileScan->PageHandle);
				break;//要全部满足
			}
		}
		if(i >= rmFileScan->conNum)//满足才输出，否则选择下一条记录
			break;
	}
	if (tmp == RM_EOF)
		return RM_EOF;
	rec->bValid = true;
	//可以直接赋值pData
	//memcpy(rec->pData, pData, size);
	rec->pData = pData;
	UnpinPage(&rmFileScan->PageHandle);
	rec->rid.bValid = true;
	rec->rid.pageNum = rmFileScan->pn;
	rec->rid.slotNum = rmFileScan->sn;
	return SUCCESS;
}

/*关闭一个文件扫描，释放相应的资源*/
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
	PF_FileHandle* pf = fileHandle->pfileHandle;//对应页面文件的句柄
	if (rid->pageNum <=1 || rid->pageNum > pf->pFileSubHeader->pageCount)//大于总页数
		return RM_INVALIDRID;
	if ((pf->pBitmap[rid->pageNum / 8] & (1 << (rid->pageNum % 8))) == 0)//空闲页（空的）返回失败
		return RM_INVALIDRID;
	PF_PageHandle pageHandle;
	pageHandle.bOpen = false;
	if ((tmp = GetThisPage(pf, rid->pageNum, &pageHandle)) != SUCCESS)
		return tmp;
	if (((pageHandle.pFrame->page.pData[rid->slotNum / 8] & (1 << (rid->slotNum % 8))) == 0))//插槽未使用
		return RM_INVALIDRID;

	int offset = ((RM_FileSubHeader*)fileHandle->pHdrPage->pData)->firstRecordOffset;
	int size = ((RM_FileSubHeader*)fileHandle->pHdrFrame->page.pData)->recordSize;
	rec->bValid = true;
	rec->rid = *rid;
	rec->rid.bValid = true;
	rec->pData = pageHandle.pFrame->page.pData + offset + (rid->slotNum) * size;//只返回记录地址
	//memcpy(rec->pData, pageHandle.pFrame->page.pData + offset + (rid->slotNum) * size, size);//拷贝数据
	UnpinPage(&pageHandle);
	return SUCCESS;
}

RC InsertRec (RM_FileHandle *fileHandle,char *pData, RID *rid)
{
	RC tmp;
	int i, j, k;
	bool isFull = false;//插入该条记录后该页是否已满
	PF_FileHandle* pf = fileHandle->pfileHandle;
	for (i = 0; i <= pf->pFileSubHeader->pageCount; i++)
	{
		//找到第一个非满页
		if (((pf->pBitmap[i / 8] & (1 << (i % 8))) != 0) && ((fileHandle->pBitmap[i / 8] & (1 << (i % 8))) == 0))
			break;
	}
	int offset= ((RM_FileSubHeader*)fileHandle->pHdrPage->pData)->firstRecordOffset;
	int size= ((RM_FileSubHeader*)fileHandle->pHdrFrame->page.pData)->recordSize;
	int recordsPerPage = ((RM_FileSubHeader*)fileHandle->pHdrFrame->page.pData)->recordsPerPage;
	PF_PageHandle pageHandle;//局部变量
	pageHandle.bOpen = false;
	if (i <= pf->pFileSubHeader->pageCount)//有未满的页
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
		//将对应的槽位置为1
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
	//若isFull==true,置位示图相应位置为1
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
	PF_FileHandle* pf = fileHandle->pfileHandle;//对应页面文件的句柄
	if (rid->pageNum <= 1 || rid->pageNum > pf->pFileSubHeader->pageCount)//大于总页数
		return RM_INVALIDRID;
	if ((pf->pBitmap[rid->pageNum / 8] & (1 << (rid->pageNum % 8))) == 0)//空闲页（空的）返回失败
		return RM_INVALIDRID;
	PF_PageHandle pageHandle;
	pageHandle.bOpen = false;
	if ((tmp = GetThisPage(pf, rid->pageNum, &pageHandle)) != SUCCESS)
		return tmp;
	if (((pageHandle.pFrame->page.pData[rid->slotNum / 8] & (1 << (rid->slotNum % 8))) == 0))//插槽未使用
		return RM_INVALIDRID;
	//槽位置0，置为非满页0，总记录数-1
	pageHandle.pFrame->page.pData[rid->slotNum / 8] &= (~(1 << (rid->slotNum % 8)));
	fileHandle->fileSubHeader->nRecords--;
	fileHandle->pBitmap[rid->pageNum / 8] &= (~(1 << (rid->pageNum % 8)));
	MarkDirty(&pageHandle);
	fileHandle->pHdrFrame->bDirty = true;//没有构造pageHandle,直接置为dirty
	UnpinPage(&pageHandle);
	return SUCCESS;
}

RC UpdateRec (RM_FileHandle *fileHandle,const RM_Record *rec)
{
	RC tmp;
	PF_FileHandle* pf = fileHandle->pfileHandle;//对应页面文件的句柄
	if (rec->rid.pageNum <= 1 || rec->rid.pageNum > pf->pFileSubHeader->pageCount)//大于总页数
		return RM_INVALIDRID;
	if ((pf->pBitmap[rec->rid.pageNum / 8] & (1 << (rec->rid.pageNum % 8))) == 0)//空闲页（空的）返回失败
		return RM_INVALIDRID;
	PF_PageHandle pageHandle;
	pageHandle.bOpen = false;
	if ((tmp = GetThisPage(pf, rec->rid.pageNum, &pageHandle)) != SUCCESS)
		return tmp;
	if (((pageHandle.pFrame->page.pData[rec->rid.slotNum / 8] & (1 << (rec->rid.slotNum % 8))) == 0))//插槽未使用
		return RM_INVALIDRID;
	//修改该条记录
	int offset = ((RM_FileSubHeader*)fileHandle->pHdrPage->pData)->firstRecordOffset;
	int size = ((RM_FileSubHeader*)fileHandle->pHdrFrame->page.pData)->recordSize;
	memcpy(pageHandle.pFrame->page.pData + offset + rec->rid.slotNum * size, rec->pData, size);
	MarkDirty(&pageHandle);
	UnpinPage(&pageHandle);
	return SUCCESS;
}

//直接调用的话，打开在进行操作后关闭
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
	//对页面文件操作
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
	fileSubHeader->firstRecordOffset = (int)ceil(slotsSum / 8.0);//每页第一条记录在数据区的偏移
	bitmap = pageHandle->pFrame->page.pData + (int)sizeof(RM_FileSubHeader);
	bitmap[0] |= 0x03;//表示0，1页满，即不能装记录
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
	fileHandle->fileName = fileName;//文件名都是指针，没有拷贝字符串
	fileHandle->pfileHandle = pf;	//释放句柄时，应该释放该页面文件的句柄空间
	fileHandle->pHdrFrame = pageHandle->pFrame;
	fileHandle->pHdrPage = &(pageHandle->pFrame->page);
	fileHandle->pBitmap = pageHandle->pFrame->page.pData + sizeof(RM_FileSubHeader);
	fileHandle->fileSubHeader = (RM_FileSubHeader *)pageHandle->pFrame->page.pData;
	
	free(pageHandle);//可以释放
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
