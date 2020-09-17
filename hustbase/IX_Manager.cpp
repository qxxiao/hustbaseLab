#include "stdafx.h"
#include "IX_Manager.h"
#include<math.h>

//1.IX_IndexHeader�еĽṹ�岻��ָ�룬����rootPageҪд���1ҳ
//2.ÿһҳ��IX_Node�ṹ���е�keys,ridsֻ�Ǳ��浱ʱ�õĵ�ַ���´λ�ı�
//3.�����ƶ��ؼ��ֺ�ָ�룬�����ӽڵ��parentҲҪ�޸�
RC CreateIndex(const char* fileName, AttrType attrType, int attrLength)
{
	RC rc;
	if ((rc = CreateFile(fileName)) != SUCCESS)
		return INDEX_EXIST;
	//��ӵ�һ���Ŀ�����Ϣ
	PF_FileHandle fileHandle;
	fileHandle.bopen = false;
	if ((rc = openFile((char*)fileName, &fileHandle)) != SUCCESS)
		return rc;
	PF_PageHandle pageHandle;
	pageHandle.bOpen = false;
	if ((rc = AllocatePage(&fileHandle, &pageHandle)) != SUCCESS)
		return rc;
	//��������
	//int m = (PF_PAGE_SIZE - sizeof(IX_FileHeader) - sizeof(IX_Node)) / (2 * sizeof(RID) + attrLength);
	//����B+��ʱʹ��
	int m = 5;
	IX_FileHeader* pIX_FileHeader = (IX_FileHeader*)(pageHandle.pFrame->page.pData);
	pIX_FileHeader->attrLength = attrLength;
	pIX_FileHeader->keyLength = attrLength + sizeof(RID);
	pIX_FileHeader->attrType = attrType;
	pIX_FileHeader->rootPage = pageHandle.pFrame->page.pageNum;//1-��̬�仯
	pIX_FileHeader->first_leaf = pageHandle.pFrame->page.pageNum;//��ʼ��Ϊ1
	pIX_FileHeader->order = m;

	IX_Node* pIX_Node = (IX_Node*)(pageHandle.pFrame->page.pData + sizeof(IX_FileHeader));
	pIX_Node->is_leaf = 1;
	pIX_Node->keynum = 0;
	pIX_Node->parent = 0;//�޸��ڵ�
	pIX_Node->brother = 0;//�����ֵܽڵ�
	//ע��ÿ����ʱ���¸�ֵ
	pIX_Node->keys = (char*)(pIX_Node + 1);
	pIX_Node->rids = (RID*)(pIX_Node->keys + pIX_FileHeader->order * pIX_FileHeader->keyLength);
	MarkDirty(&pageHandle);
	if ((rc = CloseFile(&fileHandle)) != SUCCESS)
		return rc;
	return SUCCESS;
}
RC OpenIndex(const char* fileName, IX_IndexHandle* indexHandle)
{
	RC rc;
	PF_FileHandle fileHandle;
	fileHandle.bopen = false;
	if ((rc = openFile((char*)fileName, &fileHandle)) != SUCCESS)
		return rc;
	PF_PageHandle pageHandle;
	pageHandle.bOpen = false;
	//�����һҳ�����п�����Ϣ
	if ((rc = GetThisPage(&fileHandle, 1, &pageHandle)) != SUCCESS)
		return rc;

	indexHandle->bOpen = true;
	indexHandle->fileHandle = fileHandle;
	indexHandle->fileHeader = *(IX_FileHeader*)pageHandle.pFrame->page.pData;
	UnpinPage(&pageHandle);
	return SUCCESS;
}
RC CloseIndex(IX_IndexHandle* indexHandle)
{
	RC rc;
	if ((rc = CloseFile(&indexHandle->fileHandle)) != SUCCESS)
		return rc;
	indexHandle->bOpen = false;
	return SUCCESS;
}

/**
*����һ��������ڵ����������е���С�Ĺؼ���
*�����һ���ȵ�ǰ���ڵ���С�Ļ�С����ȸ��¶�Ӧ·���е���С���ֵ��Ҷ�ӽڵ�֮ǰ�ķ�Ҷ�ӽڵ㣩
*Ȼ���ڲ�����߷���
*��2�����β���
*	pData:	����ֵ
*	rid:	���������Ӧ�ļ�¼id
*/
bool isLess(void* pData1, const RID* rid1, void* pData2, const RID* rid2, AttrType type)
{
	if (type == ints)
	{
		int d1 = *(int*)pData1;
		int d2 = *(int*)pData2;
		if (d1 != d2)
			return d1 < d2;
		else
			return rid1->pageNum == rid2->pageNum ? rid1->slotNum < rid2->slotNum : rid1->pageNum < rid2->pageNum;
	}
	else if (type == floats)
	{
		float d1 = *(float*)pData1;
		float d2 = *(float*)pData2;
		if (d1 != d2)  //����float,ֱ�ӱȽ�
			return d1 < d2;
		else
			return rid1->pageNum == rid2->pageNum ? rid1->slotNum < rid2->slotNum : rid1->pageNum < rid2->pageNum;
	}
	else//chars
	{
		int res = strcmp((char*)pData1, (char*)pData2);
		if (res != 0)
			return res < 0;
		else return rid1->pageNum == rid2->pageNum ? rid1->slotNum < rid2->slotNum : rid1->pageNum < rid2->pageNum;
	}
}
int compare(void* pData1, void* pData2, AttrType type)
{
	if (type == ints)
	{
		int d1 = *(int*)pData1;
		int d2 = *(int*)pData2;
		if (d1 < d2)
			return -1;
		else if (d1 == d2)
			return 0;
		else
			return 1;
	}
	else if (type == floats)
	{
		float d1 = *(float*)pData1;
		float d2 = *(float*)pData2;
		if (d1 < d2)
			return -1;
		else if (d1 == d2)
			return 0;
		else
			return 1;
	}
	else//chars
	{
		int res = strcmp((char*)pData1, (char*)pData2);
		if (res < 0)
			return -1;
		else if (res == 0)
			return 0;
		else
			return 1;
	}
}
bool isEqual(void* pData1, const RID* rid1, void* pData2, const RID* rid2, AttrType type)
{
	if (type == ints)
	{
		int d1 = *(int*)pData1;
		int d2 = *(int*)pData2;
		return d1 == d2 && rid1->pageNum == rid2->pageNum && rid1->slotNum == rid2->slotNum;
	}
	else if (type == floats)
	{
		float d1 = *(float*)pData1;
		float d2 = *(float*)pData2;
		return d1 == d2 && rid1->pageNum == rid2->pageNum && rid1->slotNum == rid2->slotNum;
	}
	else//chars
	{
		int res = strcmp((char*)pData1, (char*)pData2);
		return res == 0 && rid1->pageNum == rid2->pageNum && rid1->slotNum == rid2->slotNum;
	}
}
//��һ��δ���Ľڵ��в���һ��������(�ؼ���+ָ��rid)
bool insert(IX_IndexHandle* indexHandle, IX_Node* pIX_Node, void* pData, const RID* rid, const RID* rid2)
{
	if (pIX_Node->keynum >= indexHandle->fileHeader.order)
		return false;
	int i;
	char* pKey = pIX_Node->keys;//��ʼ��Ϊ��ʼλ�ã����սڵ��ֱ�Ӳ����0��λ��
	for (i = 0; i < pIX_Node->keynum; i++)
	{
		pKey = pIX_Node->keys + i * indexHandle->fileHeader.keyLength;
		if (isLess(pData, rid, pKey, (RID*)(pKey + indexHandle->fileHeader.attrLength), indexHandle->fileHeader.attrType))
			break;
	}
	pKey = pIX_Node->keys + i * indexHandle->fileHeader.keyLength;
	RID* pRid = pIX_Node->rids + i;
	if (pIX_Node->keynum != 0 && i < pIX_Node->keynum)
	{
		memmove(pKey + indexHandle->fileHeader.keyLength, pKey, (pIX_Node->keynum - i) * indexHandle->fileHeader.keyLength);
		memmove(pRid + 1, pRid, (pIX_Node->keynum - i) * sizeof(RID));
	}
	memcpy(pKey, pData, indexHandle->fileHeader.attrLength);
	memcpy(pKey + indexHandle->fileHeader.attrLength, rid, sizeof(RID));
	memcpy(pRid, rid2, sizeof(RID));

	pIX_Node->keynum++;
	return true;
}

RC InsertEntry(IX_IndexHandle* indexHandle, void* pData, const RID* rid)
{
	if (!indexHandle->bOpen)
		return IX_IHCLOSED;
	//�ҵ�Ӧ�ò��������¼��Ҷ�ӽڵ�
	PageNum rootPage = indexHandle->fileHeader.rootPage;
	PF_PageHandle pageHandle;
	GetThisPage(&indexHandle->fileHandle, rootPage, &pageHandle);
	//��ȡ���ڵ��IX_Node�ṹ
	IX_Node* pIX_Node = (IX_Node*)(pageHandle.pFrame->page.pData + sizeof(IX_FileHeader));
	//ע��ÿ����ʱ���¸�ֵ
	pIX_Node->keys = (char*)(pIX_Node + 1);
	pIX_Node->rids = (RID*)(pIX_Node->keys + indexHandle->fileHeader.order * indexHandle->fileHeader.keyLength);
	while (pIX_Node->is_leaf == 0)//��Ҷ�ӽڵ�
	{
		int i;
		for (i = 0; i < pIX_Node->keynum; i++)////��Ҷ�ӽڵ��keynum>0���������ȥ��
		{
			void* pKey = (void*)(pIX_Node->keys + i * indexHandle->fileHeader.keyLength);
			if (isLess(pData, rid, pKey, (RID*)((char*)pKey + indexHandle->fileHeader.attrLength), indexHandle->fileHeader.attrType))
				break;
		}
		//Ӧ�ôӵ�i-1��ָ�����Ѱ�ң��ж�i�Ƿ�Ϊ0
		PageNum nextPage;
		if (i == 0)
		{
			//�滻�м�ڵ����С������
			memcpy(pIX_Node->keys, pData, indexHandle->fileHeader.attrLength);
			memcpy(pIX_Node->keys + indexHandle->fileHeader.attrLength, rid, sizeof(RID));
			MarkDirty(&pageHandle);
			nextPage = pIX_Node->rids->pageNum;//i==0
		}
		else
			nextPage = (pIX_Node->rids + (i - 1))->pageNum;
		UnpinPage(&pageHandle);
		GetThisPage(&indexHandle->fileHandle, nextPage, &pageHandle);
		pIX_Node = (IX_Node*)(pageHandle.pFrame->page.pData + sizeof(IX_FileHeader));
		//ע��ÿ����ʱ���¸�ֵ
		pIX_Node->keys = (char*)(pIX_Node + 1);
		pIX_Node->rids = (RID*)(pIX_Node->keys + indexHandle->fileHeader.order * indexHandle->fileHeader.keyLength);
	}
	//Ҷ�ӽڵ�С��mֱ�Ӳ��뼴��
	if (pIX_Node->keynum < indexHandle->fileHeader.order)
	{
		insert(indexHandle, pIX_Node, pData, rid, rid);
		MarkDirty(&pageHandle);
		UnpinPage(&pageHandle);
	}
	else
	{
		PF_PageHandle rPageHandle, pPageHandle;
		IX_Node* prIX_Node, * ppIX_Node;
		//�жϲ���ĵ�ǰ�ڵ�Ĺؼ��ָ����Ƿ��������m��ֱ��С��m����
		void* data;
		RID* rid1, * rid2;
		if ((data = malloc(indexHandle->fileHeader.attrLength)) == NULL)
			return IX_NOMEM;
		memcpy(data, pData, indexHandle->fileHeader.attrLength);
		if ((rid1 = (RID*)malloc(sizeof(RID))) == NULL)
			return IX_NOMEM;
		memcpy(rid1, rid, sizeof(RID));
		if ((rid2 = (RID*)malloc(sizeof(RID))) == NULL)//��ʼ��ΪҶ�ӽڵ��ָ��
			return IX_NOMEM;
		memcpy(rid2, rid, sizeof(RID));
		while (pIX_Node->keynum == indexHandle->fileHeader.order)
		{
			AllocatePage(&indexHandle->fileHandle, &rPageHandle);
			prIX_Node = (IX_Node*)(rPageHandle.pFrame->page.pData + sizeof(IX_FileHeader));
			prIX_Node->brother = pIX_Node->brother;
			pIX_Node->brother = rPageHandle.pFrame->page.pageNum;
			prIX_Node->is_leaf = pIX_Node->is_leaf;
			prIX_Node->keys = rPageHandle.pFrame->page.pData + sizeof(IX_FileHeader) + sizeof(IX_Node);
			prIX_Node->rids = (RID*)(prIX_Node->keys + indexHandle->fileHeader.order * indexHandle->fileHeader.keyLength);
			///////////////////����Ϊm-1���������Լ�///////////////////////
			int left = (int)ceil(indexHandle->fileHeader.order / 2.0);
			bool isInLeft = false;
			for (int i = 0; i < left; i++)
			{
				void* pKey = pIX_Node->keys + i * indexHandle->fileHeader.keyLength;
				if (isLess(pData, rid, pKey, (RID*)((char*)pKey + indexHandle->fileHeader.attrLength), indexHandle->fileHeader.attrType))
				{
					isInLeft = true;//�ж��²����������¼�ڷ��ѵ����
					i++;
				}
			}
			if (isInLeft)//��߷���left-1,�ұ�m-left+1
			{
				pIX_Node->keynum = left - 1;
				prIX_Node->keynum = indexHandle->fileHeader.order - left + 1;
				memcpy(prIX_Node->keys, pIX_Node->keys + (left - 1) * indexHandle->fileHeader.keyLength, prIX_Node->keynum * indexHandle->fileHeader.keyLength);
				memcpy(prIX_Node->rids, pIX_Node->rids + left - 1, prIX_Node->keynum * sizeof(RID));
				insert(indexHandle, pIX_Node, data, rid1, rid2);
			}
			else
			{
				pIX_Node->keynum = left;
				prIX_Node->keynum = indexHandle->fileHeader.order - left;
				memcpy(prIX_Node->keys, pIX_Node->keys + left * indexHandle->fileHeader.keyLength, prIX_Node->keynum * indexHandle->fileHeader.keyLength);
				memcpy(prIX_Node->rids, pIX_Node->rids + left, prIX_Node->keynum * sizeof(RID));
				insert(indexHandle, prIX_Node, data, rid1, rid2);
			}
			/////////�Է��ѵķ�Ҷ�ӽڵ�ĺ��ӽڵ�����ָ�����ڵ��parent(ֻ�ø��ұߣ�///////////
			if (prIX_Node->is_leaf == 0)
			{
				PF_PageHandle phandle;
				for (int i = 0; i < prIX_Node->keynum; i++)
				{
					GetThisPage(&indexHandle->fileHandle, (prIX_Node->rids + i)->pageNum, &phandle);
					((IX_Node*)(phandle.pFrame->page.pData + sizeof(IX_FileHeader)))->parent = rPageHandle.pFrame->page.pageNum;
					MarkDirty(&phandle);
					UnpinPage(&phandle);
				}
			}
			///////�ж��Ƿ�Ϊ���ڵ㣬�������˸��ڵ㣬���ڵ���Ϣ////////////
			if (pIX_Node->parent == 0)//�Ǹ��ڵ�
			{
				AllocatePage(&indexHandle->fileHandle, &pPageHandle);//�����ǰ�Ǹ��ڵ㣬����Ҫ����2�������Ǹ�ֻ��Ҫһ��
				ppIX_Node = (IX_Node*)(pPageHandle.pFrame->page.pData + sizeof(IX_FileHeader));
				ppIX_Node->brother = 0;
				ppIX_Node->parent = 0;
				ppIX_Node->is_leaf = false;
				ppIX_Node->keynum = 0;//������Ҫ����2��
				ppIX_Node->keys = pPageHandle.pFrame->page.pData + sizeof(IX_FileHeader) + sizeof(IX_Node);
				ppIX_Node->rids = (RID*)(ppIX_Node->keys + indexHandle->fileHeader.order * indexHandle->fileHeader.keyLength);

				pIX_Node->parent = pPageHandle.pFrame->page.pageNum;
				prIX_Node->parent = pPageHandle.pFrame->page.pageNum;
				indexHandle->fileHeader.rootPage = pPageHandle.pFrame->page.pageNum;
				//�����޸ĵ�һҳ��IX_FIleHeader
				PF_PageHandle pageHandle_1;
				GetThisPage(&indexHandle->fileHandle, 1, &pageHandle_1);
				((IX_FileHeader*)(pageHandle_1.pFrame->page.pData))->rootPage= pPageHandle.pFrame->page.pageNum;
				MarkDirty(&pageHandle_1);
				UnpinPage(&pageHandle_1);
				rid2->bValid = true;
				rid2->pageNum = pageHandle.pFrame->page.pageNum;
				rid2->slotNum = 0;
				insert(indexHandle, ppIX_Node, pIX_Node->keys, (RID*)(pIX_Node->keys + indexHandle->fileHeader.attrLength), rid2);
				rid2->pageNum = rPageHandle.pFrame->page.pageNum;
				insert(indexHandle, ppIX_Node, prIX_Node->keys, (RID*)(prIX_Node->rids + indexHandle->fileHeader.attrLength), rid2);
				MarkDirty(&pageHandle);
				MarkDirty(&rPageHandle);
				MarkDirty(&pPageHandle);
				UnpinPage(&pageHandle);
				UnpinPage(&rPageHandle);
				UnpinPage(&pPageHandle);
				free(data); free(rid1); free(rid2);
				return SUCCESS;
			}
			else
				prIX_Node->parent = pIX_Node->parent;
			//data = prIX_Node->keys;
			memcpy(data, prIX_Node->keys, indexHandle->fileHeader.attrLength);
			//rid1 = (RID*)(prIX_Node->keys + indexHandle->fileHeader.attrLength);
			memcpy(rid1, prIX_Node->keys + indexHandle->fileHeader.attrLength, sizeof(RID));
			rid2->bValid = true;
			rid2->pageNum = rPageHandle.pFrame->page.pageNum;
			rid2->slotNum = 0;
			PageNum parent = pIX_Node->parent;
			MarkDirty(&pageHandle);
			MarkDirty(&rPageHandle);
			UnpinPage(&pageHandle);
			UnpinPage(&rPageHandle);
			GetThisPage(&indexHandle->fileHandle, parent, &pageHandle);
			pIX_Node = (IX_Node*)(pageHandle.pFrame->page.pData + sizeof(IX_FileHeader));
			//ע��ÿ����ʱ���¸�ֵ
			pIX_Node->keys = (char*)(pIX_Node + 1);
			pIX_Node->rids = (RID*)(pIX_Node->keys + indexHandle->fileHeader.order * indexHandle->fileHeader.keyLength);
		}
		insert(indexHandle, pIX_Node, data, rid1, rid2);
		MarkDirty(&pageHandle);
		UnpinPage(&pageHandle);
		free(data); free(rid1); free(rid2);
	}
	return SUCCESS;
}

//����ɾ���˽ڵ��0�����������øú�������B+���������Ĺؼ���
void updateKey(IX_IndexHandle* indexHandle, PF_PageHandle pageHandle, IX_Node* pIX_Node)
{
	int i = 0;
	while (i == 0 && pIX_Node->parent != 0)
	{
		PF_PageHandle pageHandle2;//���ڵ�
		GetThisPage(&indexHandle->fileHandle, pIX_Node->parent, &pageHandle2);
		IX_Node* pIX_Node2 = (IX_Node*)(pageHandle2.pFrame->page.pData + sizeof(IX_FileHeader));
		//ע��ÿ����ʱ���¸�ֵ
		pIX_Node2->keys = (char*)(pIX_Node2 + 1);
		pIX_Node2->rids = (RID*)(pIX_Node2->keys + indexHandle->fileHeader.order * indexHandle->fileHeader.keyLength);
		for (i = 0; i < pIX_Node2->keynum; i++)
		{
			if ((pIX_Node2->rids + i)->pageNum == pageHandle.pFrame->page.pageNum)
				break;
		}
		memcpy(pIX_Node2->keys + i * indexHandle->fileHeader.keyLength, pIX_Node->keys, indexHandle->fileHeader.keyLength);
		MarkDirty(&pageHandle2);
		UnpinPage(&pageHandle);
		pageHandle = pageHandle2;
		pIX_Node = pIX_Node2;
	}
	UnpinPage(&pageHandle);
}
//ɾ��(pData,rid)��������
//1. ����Ҷ�ӽڵ�����>[m/2],ֱ��ɾ��(�����޸��м�ڵ�����������ֵ)
//2. �ݹ��ִ�У������ֵܽڵ�裬���о����ֵܽڵ���кϲ�
RC DeleteEntry(IX_IndexHandle* indexHandle, void* pData, const RID* rid)
{
	if (!indexHandle->bOpen)
		return IX_IHCLOSED;
	PageNum rootPage = indexHandle->fileHeader.rootPage;
	PF_PageHandle pageHandle;//��ǰ�ڵ�
	GetThisPage(&indexHandle->fileHandle, rootPage, &pageHandle);
	IX_Node* pIX_Node = (IX_Node*)(pageHandle.pFrame->page.pData + sizeof(IX_FileHeader));
	//ע��ÿ����ʱ���¸�ֵ
	pIX_Node->keys = (char*)(pIX_Node + 1);
	pIX_Node->rids = (RID*)(pIX_Node->keys + indexHandle->fileHeader.order * indexHandle->fileHeader.keyLength);
	int i;//Ҫɾ������������±�
	int keyLength = indexHandle->fileHeader.keyLength;
	while (pIX_Node->is_leaf == 0)//��Ҷ�ӽڵ�
	{
		for (i = 0; i < pIX_Node->keynum; i++)////��Ҷ�ӽڵ��keynum>0,Ҳ�ҵ�һ���������ֵ
		{
			void* pKey = (void*)(pIX_Node->keys + i * indexHandle->fileHeader.keyLength);
			if (isLess(pData, rid, pKey, (RID*)((char*)pKey + indexHandle->fileHeader.attrLength), indexHandle->fileHeader.attrType))
				break;
		}
		if (i == 0)
		{
			UnpinPage(&pageHandle);
			return IX_INVALIDKEY;//��ʾ������
		}
		PageNum nextPage = (pIX_Node->rids + (i - 1))->pageNum;
		UnpinPage(&pageHandle);
		GetThisPage(&indexHandle->fileHandle, nextPage, &pageHandle);
		pIX_Node = (IX_Node*)(pageHandle.pFrame->page.pData + sizeof(IX_FileHeader));
		//ע��ÿ����ʱ���¸�ֵ
		pIX_Node->keys = (char*)(pIX_Node + 1);
		pIX_Node->rids = (RID*)(pIX_Node->keys + indexHandle->fileHeader.order * indexHandle->fileHeader.keyLength);
	}
	//�ҵ��ü�¼
	for (i = 0; i < pIX_Node->keynum; i++)
	{
		char* pKey = pIX_Node->keys + i * keyLength;
		if (isEqual(pData, rid, pKey, (RID*)(pKey + indexHandle->fileHeader.attrLength), indexHandle->fileHeader.attrType))
			break;
	}
	if (i >= pIX_Node->keynum)
	{
		UnpinPage(&pageHandle);
		return IX_INVALIDKEY;//��ʾ�����������
	}
	////////////1.ֱ��ɾ��������///////////////
	int m_2 = (int)ceil(indexHandle->fileHeader.order / 2.0);
	if (pIX_Node->keynum > m_2 || pIX_Node->parent == 0)//�ڵ�>m/2�����Ǹ��ڵ�ΪҶ�ӣ�ֱ��ɾ������
	{
		if (i < pIX_Node->keynum - 1)
		{
			memmove(pIX_Node->keys + i * keyLength, pIX_Node->keys + (i + 1) * keyLength, (pIX_Node->keynum - i - 1) * keyLength);
			memmove(pIX_Node->rids + i, pIX_Node->rids + i + 1, (pIX_Node->keynum - i - 1) * sizeof(RID));
		}
		pIX_Node->keynum--;
		MarkDirty(&pageHandle);
		if (i == 0 && pIX_Node->parent != 0)//��Ҫ�����м�ڵ�Ĺؼ���
			updateKey(indexHandle, pageHandle, pIX_Node);
		else
			UnpinPage(&pageHandle);
		return SUCCESS;
	}
	else
	{
		while (pIX_Node->keynum == m_2 && pIX_Node->parent != 0)
		{
			//1.�����ֵܽڵ��
			PageNum pageNum = pageHandle.pFrame->page.pageNum;
			PF_PageHandle pPageHandle, lPageHandle, rPageHandle;
			IX_Node* ppIX_Node, * plIX_Node, * prIX_Node;
			GetThisPage(&indexHandle->fileHandle, pIX_Node->parent, &pPageHandle);//��ȡ���ڵ�
			ppIX_Node = (IX_Node*)(pPageHandle.pFrame->page.pData + sizeof(IX_FileHeader));
			//ע��ÿ����ʱ���¸�ֵ
			ppIX_Node->keys = (char*)(ppIX_Node + 1);
			ppIX_Node->rids = (RID*)(ppIX_Node->keys + indexHandle->fileHeader.order * indexHandle->fileHeader.keyLength);
			int j;//��¼���ڵ㵽���ڵ��ָ�����(��ȡ�ֵܽڵ�)
			for (j = 0; j < ppIX_Node->keynum; j++)
			{
				if ((ppIX_Node->rids + j)->pageNum == pageHandle.pFrame->page.pageNum)
					break;
			}
			if (j > 0)//�����ֵܽڵ�
			{
				GetThisPage(&indexHandle->fileHandle, (ppIX_Node->rids + (j - 1))->pageNum, &lPageHandle);
				plIX_Node = (IX_Node*)(lPageHandle.pFrame->page.pData + sizeof(IX_FileHeader));
				//ע��ÿ����ʱ���¸�ֵ
				plIX_Node->keys = (char*)(plIX_Node + 1);
				plIX_Node->rids = (RID*)(plIX_Node->keys + indexHandle->fileHeader.order * indexHandle->fileHeader.keyLength);
				if (plIX_Node->keynum > m_2)//���Դ����ֵܽڵ��
				{
					if (i != 0)
					{
						memmove(pIX_Node->keys + keyLength, pIX_Node->keys, i * keyLength);
						memmove(pIX_Node->rids + 1, pIX_Node->rids, i * sizeof(RID));
					}
					memcpy(pIX_Node->keys, plIX_Node->keys + (plIX_Node->keynum - 1) * keyLength, keyLength);
					memcpy(pIX_Node->rids, plIX_Node->rids + plIX_Node->keynum - 1, sizeof(RID));
					plIX_Node->keynum--;
					MarkDirty(&pageHandle);
					MarkDirty(&lPageHandle);
					if (i == 0)
						updateKey(indexHandle, pageHandle, pIX_Node);
					else
						UnpinPage(&pageHandle);
					UnpinPage(&lPageHandle);
					UnpinPage(&pPageHandle);
					return SUCCESS;
				}
				UnpinPage(&lPageHandle);
			}
			if (j < ppIX_Node->keynum - 1)//�����ֵܽڵ�
			{
				GetThisPage(&indexHandle->fileHandle, (ppIX_Node->rids + j + 1)->pageNum, &rPageHandle);
				prIX_Node = (IX_Node*)(rPageHandle.pFrame->page.pData + sizeof(IX_FileHeader));
				//ע��ÿ����ʱ���¸�ֵ
				prIX_Node->keys = (char*)(prIX_Node + 1);
				prIX_Node->rids = (RID*)(prIX_Node->keys + indexHandle->fileHeader.order * indexHandle->fileHeader.keyLength);
				if (prIX_Node->keynum > m_2)//���Դ����ֵܽڵ��
				{
					if (i != pIX_Node->keynum - 1)//�����ƶ�
					{
						memmove(pIX_Node->keys + i * keyLength, pIX_Node->keys + (i + 1) * keyLength, (pIX_Node->keynum - i - 1) * keyLength);
						memmove(pIX_Node->rids + i, pIX_Node->rids + i + 1, (pIX_Node->keynum - i - 1) * sizeof(RID));
					}
					memcpy(pIX_Node->keys + (pIX_Node->keynum - 1) * keyLength, prIX_Node->keys, keyLength);
					memcpy(pIX_Node->rids + (pIX_Node->keynum - 1), prIX_Node->rids, sizeof(RID));
					prIX_Node->keynum--;
					memmove(prIX_Node->keys, prIX_Node->keys + keyLength, prIX_Node->keynum * keyLength);
					memmove(prIX_Node->rids, prIX_Node->rids + 1, prIX_Node->keynum * sizeof(RID));
					MarkDirty(&pageHandle);
					MarkDirty(&rPageHandle);
					updateKey(indexHandle, rPageHandle, prIX_Node);
					if (i == 0)
						updateKey(indexHandle, pageHandle, pIX_Node);
					UnpinPage(&pPageHandle);
					return SUCCESS;
				}
				UnpinPage(&rPageHandle);
			}
			if (j > 0)//˵�����������ֵܽڵ�ϲ�
			{
				GetThisPage(&indexHandle->fileHandle, (ppIX_Node->rids + (j - 1))->pageNum, &lPageHandle);
				plIX_Node = (IX_Node*)(lPageHandle.pFrame->page.pData + sizeof(IX_FileHeader));
				//ע��ÿ����ʱ���¸�ֵ
				plIX_Node->keys = (char*)(plIX_Node + 1);
				plIX_Node->rids = (RID*)(plIX_Node->keys + indexHandle->fileHeader.order * indexHandle->fileHeader.keyLength);
				if (i == 0)
				{
					memcpy(plIX_Node->keys + plIX_Node->keynum * keyLength, pIX_Node->keys + keyLength, (pIX_Node->keynum - 1) * keyLength);
					memcpy(plIX_Node->rids + plIX_Node->keynum, pIX_Node->rids + 1, (pIX_Node->keynum - 1) * sizeof(RID));
				}
				else
				{
					memcpy(plIX_Node->keys + plIX_Node->keynum * keyLength, pIX_Node->keys, i * keyLength);
					memcpy(plIX_Node->keys + (plIX_Node->keynum + i) * keyLength, pIX_Node->keys + (i + 1) * keyLength, (pIX_Node->keynum - i - 1) * keyLength);
					memcpy(plIX_Node->rids + plIX_Node->keynum, pIX_Node->rids, i * sizeof(RID));
					memcpy(plIX_Node->rids + plIX_Node->keynum + i, pIX_Node->rids + i + 1, (pIX_Node->keynum - i - 1) * sizeof(RID));
				}
				//�޸��ӽڵ�ĸ��ڵ�
				if (pIX_Node->is_leaf == 0)
				{
					PF_PageHandle page;
					for (int k = 0; k < pIX_Node->keynum; k++)
					{
						if (k == i)
							continue;
						GetThisPage(&indexHandle->fileHandle, (pIX_Node->rids + k)->pageNum, &page);
						((IX_Node*)(page.pFrame->page.pData + sizeof(IX_FileHeader)))->parent = lPageHandle.pFrame->page.pageNum;
						MarkDirty(&page);
						UnpinPage(&page);
					}
				}
				plIX_Node->brother = pIX_Node->brother;
				plIX_Node->keynum += (pIX_Node->keynum - 1);
				pIX_Node->keynum = 0;
				MarkDirty(&lPageHandle);
				UnpinPage(&lPageHandle);
				UnpinPage(&pageHandle);
				DisposePage(&indexHandle->fileHandle, pageHandle.pFrame->page.pageNum);
				i = j;//ɾ�����ڵ�ĵ�j��������
			}
			else        //if (j < ppIX_Node->keynum - 1)//˵�����������ֵܽڵ�ϲ�
			{
				GetThisPage(&indexHandle->fileHandle, (ppIX_Node->rids + j + 1)->pageNum, &rPageHandle);
				prIX_Node = (IX_Node*)(rPageHandle.pFrame->page.pData + sizeof(IX_FileHeader));
				//ע��ÿ����ʱ���¸�ֵ
				prIX_Node->keys = (char*)(prIX_Node + 1);
				prIX_Node->rids = (RID*)(prIX_Node->keys + indexHandle->fileHeader.order * indexHandle->fileHeader.keyLength);
				if (i != pIX_Node->keynum - 1)//�����ƶ�
				{
					memmove(pIX_Node->keys + i * keyLength, pIX_Node->keys + (i + 1) * keyLength, (pIX_Node->keynum - i - 1) * keyLength);
					memmove(pIX_Node->rids + i, pIX_Node->rids + i + 1, (pIX_Node->keynum - i - 1) * sizeof(RID));
				}
				//�޸��ӽڵ�ĸ��ڵ�
				if (prIX_Node->is_leaf == 0)
				{
					PF_PageHandle page;
					for (int k = 0; k < prIX_Node->keynum; k++)
					{
						GetThisPage(&indexHandle->fileHandle, (prIX_Node->rids + k)->pageNum, &page);
						((IX_Node*)(page.pFrame->page.pData + sizeof(IX_FileHeader)))->parent = pageHandle.pFrame->page.pageNum;
						MarkDirty(&page);
						UnpinPage(&page);
					}
				}

				pIX_Node->brother = prIX_Node->brother;
				pIX_Node->keynum--;
				memcpy(pIX_Node->keys + pIX_Node->keynum * keyLength, prIX_Node->keys, prIX_Node->keynum * keyLength);
				memcpy(pIX_Node->rids + pIX_Node->keynum, prIX_Node->rids, prIX_Node->keynum * sizeof(RID));
				pIX_Node->keynum += prIX_Node->keynum;
				prIX_Node->keynum = 0;
				MarkDirty(&pageHandle);
				UnpinPage(&pageHandle);
				UnpinPage(&rPageHandle);
				DisposePage(&indexHandle->fileHandle, rPageHandle.pFrame->page.pageNum);
				i = j + 1;//ɾ�����ڵ�ĵ�j+1��������
			}
			pageHandle = pPageHandle;
			pIX_Node = ppIX_Node;
		}
		if (pIX_Node->parent == 0)//��ʱ������Ǹ��ڵ�ɾ�����������ڵ��ʱ��ΪҶ�ӽڵ�
		{
			if (pIX_Node->keynum == 2)//��ʱi==1
			{
				indexHandle->fileHeader.rootPage = pIX_Node->rids->pageNum;
				//�����޸ĵ�һҳ��IX_FIleHeader
				PF_PageHandle pageHandle_1;
				GetThisPage(&indexHandle->fileHandle, 1, &pageHandle_1);
				((IX_FileHeader*)(pageHandle_1.pFrame->page.pData))->rootPage = indexHandle->fileHeader.rootPage;
				MarkDirty(&pageHandle_1);
				UnpinPage(&pageHandle_1);
				PF_PageHandle rootPageHandle;
				GetThisPage(&indexHandle->fileHandle, indexHandle->fileHeader.rootPage, &rootPageHandle);
				int num = pageHandle.pFrame->page.pageNum;
				UnpinPage(&pageHandle);
				DisposePage(&indexHandle->fileHandle, num);
				((IX_Node*)(rootPageHandle.pFrame->page.pData + sizeof(IX_FileHeader)))->parent = 0;//�ֵܽڵ�ϲ�ʱ��
				MarkDirty(&rootPageHandle);
				UnpinPage(&rootPageHandle);
			}
			else//ֱ��ɾ��
			{
				if (i < pIX_Node->keynum - 1)
				{
					memmove(pIX_Node->keys + i * keyLength, pIX_Node->keys + (i + 1) * keyLength, (pIX_Node->keynum - i - 1)* keyLength);
					memmove(pIX_Node->rids + i, pIX_Node->rids + i + 1, (pIX_Node->keynum - i - 1) * sizeof(RID));
				}
				pIX_Node->keynum--;
				MarkDirty(&pageHandle);
				UnpinPage(&pageHandle);
			}
		}
	}
	return SUCCESS;
}


//��ͬ��RM��ʵ�֣���ʼ��ʱpn,ridIx�ǵ�һ�����ܻ�ȡ��������
RC OpenIndexScan(IX_IndexScan* indexScan, IX_IndexHandle* indexHandle, CompOp compOp, char* value)
{
	if (indexScan->bOpen)
		return IX_SCANOPENNED;
	indexScan->bOpen = true;
	indexScan->pIXIndexHandle = indexHandle;
	indexScan->compOp = compOp;
	indexScan->value = value;
	//����ֵ
	PageNum rootPage = indexHandle->fileHeader.rootPage;
	PF_PageHandle pageHandle;//��ǰ�ڵ�
	GetThisPage(&indexHandle->fileHandle, rootPage, &pageHandle);
	IX_Node* pIX_Node = (IX_Node*)(pageHandle.pFrame->page.pData + sizeof(IX_FileHeader));
	//ע��ÿ����ʱ���¸�ֵ
	pIX_Node->keys = (char*)(pIX_Node + 1);
	pIX_Node->rids = (RID*)(pIX_Node->keys + indexHandle->fileHeader.order * indexHandle->fileHeader.keyLength);
	if (pIX_Node->keynum == 0)//����������һ����ʾΪ��
	{
		indexScan->pn = 0;
		indexScan->ridIx = 0;
		return SUCCESS;
	}
	////////////////////////////
	if (compOp == LessT || compOp == LEqual || compOp == NO_OP || compOp == NEqual)//���ǲ�����
	{
		indexScan->pn = indexHandle->fileHeader.first_leaf;
		indexScan->ridIx = 0;
		UnpinPage(&pageHandle);
		return SUCCESS;
	}
	//(compOp == EQual || compOp == GEqual || compOp == GreatT)
	RID rid;
	rid.bValid = true;
	rid.pageNum = 0;
	rid.slotNum = 0;//ֻ�Ǳ�ʾ������������ͬʱ����С������¼�������ڣ�--�ҵ���Ӧ��Ҷ�ӽڵ�
	int i, tmp;
	while (pIX_Node->is_leaf == 0)//��Ҷ�ӽڵ�
	{
		PageNum nextPage;
		for (i = 0; i < pIX_Node->keynum; i++)////��Ҷ�ӽڵ��keynum>0,�ҵ�һ���������ֵ
		{
			void* pKey = (void*)(pIX_Node->keys + i * indexHandle->fileHeader.keyLength);
			if (isLess(value, &rid, pKey, (RID*)((char*)pKey + indexHandle->fileHeader.attrLength), indexHandle->fileHeader.attrType))
				break;
		}
		if (i == 0)
			nextPage = pIX_Node->rids->pageNum;
		nextPage = (pIX_Node->rids + (i - 1))->pageNum;
		UnpinPage(&pageHandle);
		GetThisPage(&indexHandle->fileHandle, nextPage, &pageHandle);
		pIX_Node = (IX_Node*)(pageHandle.pFrame->page.pData + sizeof(IX_FileHeader));
		//ע��ÿ����ʱ���¸�ֵ
		pIX_Node->keys = (char*)(pIX_Node + 1);
		pIX_Node->rids = (RID*)(pIX_Node->keys + indexHandle->fileHeader.order * indexHandle->fileHeader.keyLength);
	}
	if (compOp == EQual)
	{
		for (i = 0; i < pIX_Node->keynum; i++)
		{
			tmp = compare(pIX_Node->keys + i * indexHandle->fileHeader.keyLength, value, indexHandle->fileHeader.attrType);
			if (tmp == -1)//С�ڵ�ȥ��
				continue;
			else
				break;//�ҵ�>����=������
		}
		if (tmp == 1)
		{
			indexScan->pn = 0;
			indexScan->ridIx = 0;
			UnpinPage(&pageHandle);
			return SUCCESS;
		}
		else if (tmp == 0)
		{
			indexScan->pn = pageHandle.pFrame->page.pageNum;
			indexScan->ridIx = i;
			UnpinPage(&pageHandle);
			return SUCCESS;
		}
		else {//��Ҫ������һ���ڵ�
			PageNum brother = pIX_Node->brother;
			if (brother == 0)
			{
				indexScan->pn = 0;
				indexScan->ridIx = 0;
				UnpinPage(&pageHandle);
				return SUCCESS;
			}
			UnpinPage(&pageHandle);
			GetThisPage(&indexHandle->fileHandle, brother, &pageHandle);
			pIX_Node = (IX_Node*)(pageHandle.pFrame->page.pData + sizeof(IX_FileHeader));
			//ע��ÿ����ʱ���¸�ֵ
			pIX_Node->keys = (char*)(pIX_Node + 1);
			pIX_Node->rids = (RID*)(pIX_Node->keys + indexHandle->fileHeader.order * indexHandle->fileHeader.keyLength);
			if (compare(pIX_Node->keys, value, indexHandle->fileHeader.attrType) == 0)
			{
				indexScan->pn = brother;
				indexScan->ridIx = 0;
				UnpinPage(&pageHandle);
				return SUCCESS;
			}
			else {
				indexScan->pn = 0;
				indexScan->ridIx = 0;
				UnpinPage(&pageHandle);
				return SUCCESS;
			}
		}
	}
	if (compOp == GEqual)
	{
		for (i = 0; i < pIX_Node->keynum; i++)
		{
			tmp = compare(pIX_Node->keys + i * indexHandle->fileHeader.keyLength, value, indexHandle->fileHeader.attrType);
			if (tmp == -1)//С�ڵ�ȥ��
				continue;
			else
				break;//comOp=GEʱ�ҵ�>= ; cmpOp=Gʱ�ҵ�>
		}
		if (tmp >= 0)
		{
			indexScan->pn = pageHandle.pFrame->page.pageNum;
			indexScan->ridIx = i;
			UnpinPage(&pageHandle);
			return SUCCESS;
		}
		if (pIX_Node->brother != 0)
		{
			indexScan->pn = pIX_Node->brother;
			indexScan->ridIx = 0;
			UnpinPage(&pageHandle);
			return SUCCESS;
		}
		else {
			indexScan->pn = 0;
			indexScan->ridIx = 0;
			UnpinPage(&pageHandle);
			return SUCCESS;
		}
	}
	if (compOp == GreatT)
	{
		for (i = 0; i < pIX_Node->keynum; i++)
		{
			tmp = compare(pIX_Node->keys + i * indexHandle->fileHeader.keyLength, value, indexHandle->fileHeader.attrType);
			if (tmp <= 0)//<=��ȥ��
				continue;
			else
				break;//comOp=GEʱ�ҵ�>= ; cmpOp=Gʱ�ҵ�>
		}
		if (tmp == 1)
		{
			indexScan->pn = pageHandle.pFrame->page.pageNum;
			indexScan->ridIx = i;
			UnpinPage(&pageHandle);
			return SUCCESS;
		}
		int brother = pIX_Node->brother;
		while (brother != 0)
		{
			UnpinPage(&pageHandle);
			GetThisPage(&indexHandle->fileHandle, brother, &pageHandle);
			pIX_Node = (IX_Node*)(pageHandle.pFrame->page.pData + sizeof(IX_FileHeader));
			//ע��ÿ����ʱ���¸�ֵ
			pIX_Node->keys = (char*)(pIX_Node + 1);
			pIX_Node->rids = (RID*)(pIX_Node->keys + indexHandle->fileHeader.order * indexHandle->fileHeader.keyLength);
			int j;
			for (j = 0; j < pIX_Node->keynum; j++)
			{
				tmp = compare(pIX_Node->keys + i * indexHandle->fileHeader.keyLength, value, indexHandle->fileHeader.attrType);
				if (tmp <= 0)//<=��ȥ��
					continue;
				else
					break;//comOp=GEʱ�ҵ�>= ; cmpOp=Gʱ�ҵ�>
			}
			if (tmp == 1)//�ҵ��˵�һ��>��������
			{
				indexScan->pn = brother;
				indexScan->ridIx = j;
				UnpinPage(&pageHandle);
				return SUCCESS;
			}
			brother = pIX_Node->brother;
		}
		indexScan->pn = 0;
		indexScan->ridIx = 0;
		UnpinPage(&pageHandle);
		return SUCCESS;
	}
	return SUCCESS;
}

RC IX_GetNextEntry(IX_IndexScan* indexScan, RID* rid)
{
	if (!indexScan->bOpen)
		return IX_ISCLOSED;
	if (indexScan->pn == 0 && indexScan->ridIx == 0)
		return IX_EOF;//��ʾû����һ����¼
	GetThisPage(&indexScan->pIXIndexHandle->fileHandle, indexScan->pn, &indexScan->Pagehandle);
	IX_Node* pIX_Node = (IX_Node*)(indexScan->Pagehandle.pFrame->page.pData + sizeof(IX_FileHeader));
	//ע��ÿ����ʱ���¸�ֵ
	pIX_Node->keys = (char*)(pIX_Node + 1);
	pIX_Node->rids = (RID*)(pIX_Node->keys + indexScan->pIXIndexHandle->fileHeader.order * indexScan->pIXIndexHandle->fileHeader.keyLength);
	int keyLength = indexScan->pIXIndexHandle->fileHeader.keyLength;
	//LessT LEqual  NO_OP (NEqual)	 GEqual GreatT
	switch (indexScan->compOp)
	{
	case LessT:
		if (compare(pIX_Node->keys + indexScan->ridIx * keyLength, indexScan->value, indexScan->pIXIndexHandle->fileHeader.attrType) == -1)
			break;
		else
			return IX_EOF;

	case LEqual:
		if (compare(pIX_Node->keys + indexScan->ridIx * keyLength, indexScan->value, indexScan->pIXIndexHandle->fileHeader.attrType) <= 0)
			break;
		else
			return IX_EOF;
	case NO_OP:
		break;
	case GEqual:
		if (compare(pIX_Node->keys + indexScan->ridIx * keyLength, indexScan->value, indexScan->pIXIndexHandle->fileHeader.attrType) >= 0)
			break;
		else
			return IX_EOF;
	case GreatT:
		if (compare(pIX_Node->keys + indexScan->ridIx * keyLength, indexScan->value, indexScan->pIXIndexHandle->fileHeader.attrType) > 0)
			break;
		else
			return IX_EOF;
	case NEqual:
		if (compare(pIX_Node->keys + indexScan->ridIx * keyLength, indexScan->value, indexScan->pIXIndexHandle->fileHeader.attrType) != 0)
			break;
		else
		{
			if (indexScan->ridIx < pIX_Node->keynum - 1)
				indexScan->ridIx++;
			else {
				if (pIX_Node->brother != 0)
				{
					indexScan->pn = pIX_Node->brother;
					indexScan->ridIx = 0;
				}
				else {
					indexScan->pn = 0;
					indexScan->ridIx = 0;
				}
			}
			return IX_GetNextEntry(indexScan, rid);//���ʱ������һ�����ȵ�
		}
	}
	//���Եĸ�����¼��������
	rid->bValid = true;
	rid->pageNum = (pIX_Node->rids + indexScan->ridIx)->pageNum;
	rid->slotNum = (pIX_Node->rids + indexScan->ridIx)->slotNum;
	if (indexScan->ridIx < pIX_Node->keynum - 1)
		indexScan->ridIx++;
	else {
		if (pIX_Node->brother != 0)
		{
			indexScan->pn = pIX_Node->brother;
			indexScan->ridIx = 0;
		}
		else {
			indexScan->pn = 0;
			indexScan->ridIx = 0;
		}
	}
	return SUCCESS;
}

RC CloseIndexScan(IX_IndexScan* indexScan)
{
	indexScan->bOpen = false;
	indexScan->pn = 0;
	indexScan->ridIx = 0;
	return SUCCESS;
}

Tree_Node* getTree(IX_IndexHandle* indexHandle, IX_Node* pIX_Node, Tree_Node* parent, int childs)//ÿ���ڵ�ֻ�ñ�������ߵĺ��ӽڵ�����ֵܽڵ�
{
	//����һ���ڵ㣬���ڸ��ڵ���ʣ����ӽڵ����-1
	childs--;
	PF_PageHandle pageHandle;
	Tree_Node* pNode = (Tree_Node*)malloc(sizeof(Tree_Node));
	if (pNode == NULL)
		return nullptr;
	//////////////����Ҫ���ĸ�ֵ����///////////////////
	pNode->keyNum = pIX_Node->keynum; //����Ϊ0--����Ϊ��
	if (pNode->keyNum != 0) 
	{
		pNode->keys = (char**)malloc(pNode->keyNum * sizeof(char*));
		if (pNode->keys == NULL)
			return nullptr;
	}
	else
		pNode->keys = nullptr;
	for (int i = 0; i < pNode->keyNum; i++)
	{
		pNode->keys[i] = (char*)malloc(indexHandle->fileHeader.attrLength);
		if (pNode->keys[i] == NULL)
			return nullptr;
		memcpy(pNode->keys[i], pIX_Node->keys + i * indexHandle->fileHeader.keyLength, indexHandle->fileHeader.attrLength);
	}
	pNode->parent = parent;
	//��ǰ�ڵ�Ϊ��Ҷ�ӽڵ�-�м�ڵ�
	if (pIX_Node->is_leaf == 0)
	{
		PageNum first = pIX_Node->rids->pageNum;
		int keyNum = pIX_Node->keynum;
		GetThisPage(&indexHandle->fileHandle, first, &pageHandle);
		IX_Node* pIX = (IX_Node*)(pageHandle.pFrame->page.pData + sizeof(IX_FileHeader));
		pIX->keys = (char*)(pIX + 1);
		pIX->rids = (RID*)(pIX->keys + indexHandle->fileHeader.order * indexHandle->fileHeader.keyLength);
		pNode->firstChild = getTree(indexHandle, pIX, pNode, keyNum);
		UnpinPage(&pageHandle);
	}
	else//��ǰ�ڵ�ΪҶ�ӽڵ�
		pNode->firstChild = nullptr;
	if (childs == 0)
		pNode->sibling = nullptr;
	else
	{
		GetThisPage(&indexHandle->fileHandle, pIX_Node->brother, &pageHandle);
		IX_Node* pIX = (IX_Node*)(pageHandle.pFrame->page.pData + sizeof(IX_FileHeader));
		pIX->keys = (char*)(pIX + 1);
		pIX->rids = (RID*)(pIX->keys + indexHandle->fileHeader.order * indexHandle->fileHeader.keyLength);
		pNode->sibling = getTree(indexHandle, pIX, parent, childs);
		UnpinPage(&pageHandle);
	}
	return pNode;
}

RC GetIndexTree(char* fileName, Tree* index)
{
	IX_IndexHandle indexHandle;
	if (OpenIndex(fileName, &indexHandle) != SUCCESS)
		return INDEX_NOT_EXIST;
	index->attrType = indexHandle.fileHeader.attrType;
	index->attrLength = indexHandle.fileHeader.attrLength;
	index->order = indexHandle.fileHeader.order;
	PF_PageHandle pageHandle;
	GetThisPage(&indexHandle.fileHandle, indexHandle.fileHeader.rootPage, &pageHandle);
	IX_Node* pIX_Node = (IX_Node*)(pageHandle.pFrame->page.pData + sizeof(IX_FileHeader));
	pIX_Node->keys = (char*)(pIX_Node + 1);
	pIX_Node->rids = (RID*)(pIX_Node->keys + indexHandle.fileHeader.order * indexHandle.fileHeader.keyLength);
	index->root = getTree(&indexHandle, pIX_Node, nullptr, 1);
	UnpinPage(&pageHandle);
	CloseIndex(&indexHandle);
	return SUCCESS;
}

void deleteTree(Tree_Node *root)
{
	if (root->firstChild)
		deleteTree(root->firstChild);
	if (root->sibling)
		deleteTree(root->sibling);
	for (int i = 0; i < root->keyNum; i++)
		free(root->keys[i]);
	free(root->keys);
	free(root);
}