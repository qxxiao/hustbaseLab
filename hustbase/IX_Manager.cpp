#include "stdafx.h"
#include "IX_Manager.h"
#include<math.h>

//1.IX_IndexHeader中的结构体不是指针，更改rootPage要写会第1页
//2.每一页的IX_Node结构体中的keys,rids只是保存当时用的地址，下次会改变
//3.分裂移动关键字和指针，对于子节点的parent也要修改
RC CreateIndex(const char* fileName, AttrType attrType, int attrLength)
{
	RC rc;
	if ((rc = CreateFile(fileName)) != SUCCESS)
		return INDEX_EXIST;
	//添加第一个的控制信息
	PF_FileHandle fileHandle;
	fileHandle.bopen = false;
	if ((rc = openFile((char*)fileName, &fileHandle)) != SUCCESS)
		return rc;
	PF_PageHandle pageHandle;
	pageHandle.bOpen = false;
	if ((rc = AllocatePage(&fileHandle, &pageHandle)) != SUCCESS)
		return rc;
	//计算序数
	//int m = (PF_PAGE_SIZE - sizeof(IX_FileHeader) - sizeof(IX_Node)) / (2 * sizeof(RID) + attrLength);
	//测试B+树时使用
	int m = 5;
	IX_FileHeader* pIX_FileHeader = (IX_FileHeader*)(pageHandle.pFrame->page.pData);
	pIX_FileHeader->attrLength = attrLength;
	pIX_FileHeader->keyLength = attrLength + sizeof(RID);
	pIX_FileHeader->attrType = attrType;
	pIX_FileHeader->rootPage = pageHandle.pFrame->page.pageNum;//1-动态变化
	pIX_FileHeader->first_leaf = pageHandle.pFrame->page.pageNum;//初始化为1
	pIX_FileHeader->order = m;

	IX_Node* pIX_Node = (IX_Node*)(pageHandle.pFrame->page.pData + sizeof(IX_FileHeader));
	pIX_Node->is_leaf = 1;
	pIX_Node->keynum = 0;
	pIX_Node->parent = 0;//无父节点
	pIX_Node->brother = 0;//无右兄弟节点
	//注意每次用时重新赋值
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
	//载入第一页，含有控制信息
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
*插入一个索引项，节点中是子树中的最小的关键字
*当添加一个比当前根节点最小的还小的项，先更新对应路径中的最小项的值（叶子节点之前的非叶子节点）
*然后在插入或者分裂
*分2种情形插入
*	pData:	属性值
*	rid:	该索引项对应的记录id
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
		if (d1 != d2)  //都是float,直接比较
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
//向一个未满的节点中插入一条索引项(关键字+指针rid)
bool insert(IX_IndexHandle* indexHandle, IX_Node* pIX_Node, void* pData, const RID* rid, const RID* rid2)
{
	if (pIX_Node->keynum >= indexHandle->fileHeader.order)
		return false;
	int i;
	char* pKey = pIX_Node->keys;//初始化为起始位置，若空节点就直接插入第0个位置
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
	//找到应该插入该条记录的叶子节点
	PageNum rootPage = indexHandle->fileHeader.rootPage;
	PF_PageHandle pageHandle;
	GetThisPage(&indexHandle->fileHandle, rootPage, &pageHandle);
	//获取根节点的IX_Node结构
	IX_Node* pIX_Node = (IX_Node*)(pageHandle.pFrame->page.pData + sizeof(IX_FileHeader));
	//注意每次用时重新赋值
	pIX_Node->keys = (char*)(pIX_Node + 1);
	pIX_Node->rids = (RID*)(pIX_Node->keys + indexHandle->fileHeader.order * indexHandle->fileHeader.keyLength);
	while (pIX_Node->is_leaf == 0)//非叶子节点
	{
		int i;
		for (i = 0; i < pIX_Node->keynum; i++)////非叶子节点的keynum>0，分裂提出去的
		{
			void* pKey = (void*)(pIX_Node->keys + i * indexHandle->fileHeader.keyLength);
			if (isLess(pData, rid, pKey, (RID*)((char*)pKey + indexHandle->fileHeader.attrLength), indexHandle->fileHeader.attrType))
				break;
		}
		//应该从第i-1个指针继续寻找，判断i是否为0
		PageNum nextPage;
		if (i == 0)
		{
			//替换中间节点的最小索引项
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
		//注意每次用时重新赋值
		pIX_Node->keys = (char*)(pIX_Node + 1);
		pIX_Node->rids = (RID*)(pIX_Node->keys + indexHandle->fileHeader.order * indexHandle->fileHeader.keyLength);
	}
	//叶子节点小于m直接插入即可
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
		//判断插入的当前节点的关键字个数是否等于序数m，直到小于m插入
		void* data;
		RID* rid1, * rid2;
		if ((data = malloc(indexHandle->fileHeader.attrLength)) == NULL)
			return IX_NOMEM;
		memcpy(data, pData, indexHandle->fileHeader.attrLength);
		if ((rid1 = (RID*)malloc(sizeof(RID))) == NULL)
			return IX_NOMEM;
		memcpy(rid1, rid, sizeof(RID));
		if ((rid2 = (RID*)malloc(sizeof(RID))) == NULL)//初始化为叶子节点的指针
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
			///////////////////若是为m-1的序数可以简化///////////////////////
			int left = (int)ceil(indexHandle->fileHeader.order / 2.0);
			bool isInLeft = false;
			for (int i = 0; i < left; i++)
			{
				void* pKey = pIX_Node->keys + i * indexHandle->fileHeader.keyLength;
				if (isLess(pData, rid, pKey, (RID*)((char*)pKey + indexHandle->fileHeader.attrLength), indexHandle->fileHeader.attrType))
				{
					isInLeft = true;//判定新插入的索引记录在分裂的左边
					i++;
				}
			}
			if (isInLeft)//左边分配left-1,右边m-left+1
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
			/////////对分裂的非叶子节点的孩子节点重新指定父节点的parent(只用改右边）///////////
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
			///////判断是否为根节点，迭代到了根节点，填充节点信息////////////
			if (pIX_Node->parent == 0)//是根节点
			{
				AllocatePage(&indexHandle->fileHandle, &pPageHandle);//如果当前是根节点，则需要插入2条；不是根只需要一条
				ppIX_Node = (IX_Node*)(pPageHandle.pFrame->page.pData + sizeof(IX_FileHeader));
				ppIX_Node->brother = 0;
				ppIX_Node->parent = 0;
				ppIX_Node->is_leaf = false;
				ppIX_Node->keynum = 0;//后面需要插入2条
				ppIX_Node->keys = pPageHandle.pFrame->page.pData + sizeof(IX_FileHeader) + sizeof(IX_Node);
				ppIX_Node->rids = (RID*)(ppIX_Node->keys + indexHandle->fileHeader.order * indexHandle->fileHeader.keyLength);

				pIX_Node->parent = pPageHandle.pFrame->page.pageNum;
				prIX_Node->parent = pPageHandle.pFrame->page.pageNum;
				indexHandle->fileHeader.rootPage = pPageHandle.pFrame->page.pageNum;
				//必须修改第一页的IX_FIleHeader
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
			//注意每次用时重新赋值
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

//若是删除了节点的0号索引，调用该函数更新B+树的索引的关键字
void updateKey(IX_IndexHandle* indexHandle, PF_PageHandle pageHandle, IX_Node* pIX_Node)
{
	int i = 0;
	while (i == 0 && pIX_Node->parent != 0)
	{
		PF_PageHandle pageHandle2;//父节点
		GetThisPage(&indexHandle->fileHandle, pIX_Node->parent, &pageHandle2);
		IX_Node* pIX_Node2 = (IX_Node*)(pageHandle2.pFrame->page.pData + sizeof(IX_FileHeader));
		//注意每次用时重新赋值
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
//删除(pData,rid)的索引项
//1. 若是叶子节点数量>[m/2],直接删除(可能修改中间节点的索引项的最值)
//2. 递归的执行：先向兄弟节点借，不行就与兄弟节点进行合并
RC DeleteEntry(IX_IndexHandle* indexHandle, void* pData, const RID* rid)
{
	if (!indexHandle->bOpen)
		return IX_IHCLOSED;
	PageNum rootPage = indexHandle->fileHeader.rootPage;
	PF_PageHandle pageHandle;//当前节点
	GetThisPage(&indexHandle->fileHandle, rootPage, &pageHandle);
	IX_Node* pIX_Node = (IX_Node*)(pageHandle.pFrame->page.pData + sizeof(IX_FileHeader));
	//注意每次用时重新赋值
	pIX_Node->keys = (char*)(pIX_Node + 1);
	pIX_Node->rids = (RID*)(pIX_Node->keys + indexHandle->fileHeader.order * indexHandle->fileHeader.keyLength);
	int i;//要删除的索引项的下标
	int keyLength = indexHandle->fileHeader.keyLength;
	while (pIX_Node->is_leaf == 0)//非叶子节点
	{
		for (i = 0; i < pIX_Node->keynum; i++)////非叶子节点的keynum>0,也找第一个比它大的值
		{
			void* pKey = (void*)(pIX_Node->keys + i * indexHandle->fileHeader.keyLength);
			if (isLess(pData, rid, pKey, (RID*)((char*)pKey + indexHandle->fileHeader.attrLength), indexHandle->fileHeader.attrType))
				break;
		}
		if (i == 0)
		{
			UnpinPage(&pageHandle);
			return IX_INVALIDKEY;//表示不存在
		}
		PageNum nextPage = (pIX_Node->rids + (i - 1))->pageNum;
		UnpinPage(&pageHandle);
		GetThisPage(&indexHandle->fileHandle, nextPage, &pageHandle);
		pIX_Node = (IX_Node*)(pageHandle.pFrame->page.pData + sizeof(IX_FileHeader));
		//注意每次用时重新赋值
		pIX_Node->keys = (char*)(pIX_Node + 1);
		pIX_Node->rids = (RID*)(pIX_Node->keys + indexHandle->fileHeader.order * indexHandle->fileHeader.keyLength);
	}
	//找到该记录
	for (i = 0; i < pIX_Node->keynum; i++)
	{
		char* pKey = pIX_Node->keys + i * keyLength;
		if (isEqual(pData, rid, pKey, (RID*)(pKey + indexHandle->fileHeader.attrLength), indexHandle->fileHeader.attrType))
			break;
	}
	if (i >= pIX_Node->keynum)
	{
		UnpinPage(&pageHandle);
		return IX_INVALIDKEY;//表示该索引项不存在
	}
	////////////1.直接删除索引项///////////////
	int m_2 = (int)ceil(indexHandle->fileHeader.order / 2.0);
	if (pIX_Node->keynum > m_2 || pIX_Node->parent == 0)//节点>m/2或者是根节点为叶子，直接删除即可
	{
		if (i < pIX_Node->keynum - 1)
		{
			memmove(pIX_Node->keys + i * keyLength, pIX_Node->keys + (i + 1) * keyLength, (pIX_Node->keynum - i - 1) * keyLength);
			memmove(pIX_Node->rids + i, pIX_Node->rids + i + 1, (pIX_Node->keynum - i - 1) * sizeof(RID));
		}
		pIX_Node->keynum--;
		MarkDirty(&pageHandle);
		if (i == 0 && pIX_Node->parent != 0)//需要更新中间节点的关键字
			updateKey(indexHandle, pageHandle, pIX_Node);
		else
			UnpinPage(&pageHandle);
		return SUCCESS;
	}
	else
	{
		while (pIX_Node->keynum == m_2 && pIX_Node->parent != 0)
		{
			//1.先向兄弟节点借
			PageNum pageNum = pageHandle.pFrame->page.pageNum;
			PF_PageHandle pPageHandle, lPageHandle, rPageHandle;
			IX_Node* ppIX_Node, * plIX_Node, * prIX_Node;
			GetThisPage(&indexHandle->fileHandle, pIX_Node->parent, &pPageHandle);//获取父节点
			ppIX_Node = (IX_Node*)(pPageHandle.pFrame->page.pData + sizeof(IX_FileHeader));
			//注意每次用时重新赋值
			ppIX_Node->keys = (char*)(ppIX_Node + 1);
			ppIX_Node->rids = (RID*)(ppIX_Node->keys + indexHandle->fileHeader.order * indexHandle->fileHeader.keyLength);
			int j;//记录父节点到本节点的指针序号(获取兄弟节点)
			for (j = 0; j < ppIX_Node->keynum; j++)
			{
				if ((ppIX_Node->rids + j)->pageNum == pageHandle.pFrame->page.pageNum)
					break;
			}
			if (j > 0)//有左兄弟节点
			{
				GetThisPage(&indexHandle->fileHandle, (ppIX_Node->rids + (j - 1))->pageNum, &lPageHandle);
				plIX_Node = (IX_Node*)(lPageHandle.pFrame->page.pData + sizeof(IX_FileHeader));
				//注意每次用时重新赋值
				plIX_Node->keys = (char*)(plIX_Node + 1);
				plIX_Node->rids = (RID*)(plIX_Node->keys + indexHandle->fileHeader.order * indexHandle->fileHeader.keyLength);
				if (plIX_Node->keynum > m_2)//可以从左兄弟节点借
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
			if (j < ppIX_Node->keynum - 1)//有右兄弟节点
			{
				GetThisPage(&indexHandle->fileHandle, (ppIX_Node->rids + j + 1)->pageNum, &rPageHandle);
				prIX_Node = (IX_Node*)(rPageHandle.pFrame->page.pData + sizeof(IX_FileHeader));
				//注意每次用时重新赋值
				prIX_Node->keys = (char*)(prIX_Node + 1);
				prIX_Node->rids = (RID*)(prIX_Node->keys + indexHandle->fileHeader.order * indexHandle->fileHeader.keyLength);
				if (prIX_Node->keynum > m_2)//可以从右兄弟节点借
				{
					if (i != pIX_Node->keynum - 1)//向左移动
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
			if (j > 0)//说明可以与左兄弟节点合并
			{
				GetThisPage(&indexHandle->fileHandle, (ppIX_Node->rids + (j - 1))->pageNum, &lPageHandle);
				plIX_Node = (IX_Node*)(lPageHandle.pFrame->page.pData + sizeof(IX_FileHeader));
				//注意每次用时重新赋值
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
				//修改子节点的父节点
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
				i = j;//删除父节点的第j个索引项
			}
			else        //if (j < ppIX_Node->keynum - 1)//说明可以与右兄弟节点合并
			{
				GetThisPage(&indexHandle->fileHandle, (ppIX_Node->rids + j + 1)->pageNum, &rPageHandle);
				prIX_Node = (IX_Node*)(rPageHandle.pFrame->page.pData + sizeof(IX_FileHeader));
				//注意每次用时重新赋值
				prIX_Node->keys = (char*)(prIX_Node + 1);
				prIX_Node->rids = (RID*)(prIX_Node->keys + indexHandle->fileHeader.order * indexHandle->fileHeader.keyLength);
				if (i != pIX_Node->keynum - 1)//向左移动
				{
					memmove(pIX_Node->keys + i * keyLength, pIX_Node->keys + (i + 1) * keyLength, (pIX_Node->keynum - i - 1) * keyLength);
					memmove(pIX_Node->rids + i, pIX_Node->rids + i + 1, (pIX_Node->keynum - i - 1) * sizeof(RID));
				}
				//修改子节点的父节点
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
				i = j + 1;//删除父节点的第j+1个索引项
			}
			pageHandle = pPageHandle;
			pIX_Node = ppIX_Node;
		}
		if (pIX_Node->parent == 0)//此时处理的是根节点删除索引，根节点此时不为叶子节点
		{
			if (pIX_Node->keynum == 2)//此时i==1
			{
				indexHandle->fileHeader.rootPage = pIX_Node->rids->pageNum;
				//必须修改第一页的IX_FIleHeader
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
				((IX_Node*)(rootPageHandle.pFrame->page.pData + sizeof(IX_FileHeader)))->parent = 0;//兄弟节点合并时改
				MarkDirty(&rootPageHandle);
				UnpinPage(&rootPageHandle);
			}
			else//直接删除
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


//不同于RM的实现，初始化时pn,ridIx是第一个可能获取的索引项
RC OpenIndexScan(IX_IndexScan* indexScan, IX_IndexHandle* indexHandle, CompOp compOp, char* value)
{
	if (indexScan->bOpen)
		return IX_SCANOPENNED;
	indexScan->bOpen = true;
	indexScan->pIXIndexHandle = indexHandle;
	indexScan->compOp = compOp;
	indexScan->value = value;
	//赋初值
	PageNum rootPage = indexHandle->fileHeader.rootPage;
	PF_PageHandle pageHandle;//当前节点
	GetThisPage(&indexHandle->fileHandle, rootPage, &pageHandle);
	IX_Node* pIX_Node = (IX_Node*)(pageHandle.pFrame->page.pData + sizeof(IX_FileHeader));
	//注意每次用时重新赋值
	pIX_Node->keys = (char*)(pIX_Node + 1);
	pIX_Node->rids = (RID*)(pIX_Node->keys + indexHandle->fileHeader.order * indexHandle->fileHeader.keyLength);
	if (pIX_Node->keynum == 0)//空索引，下一条表示为空
	{
		indexScan->pn = 0;
		indexScan->ridIx = 0;
		return SUCCESS;
	}
	////////////////////////////
	if (compOp == LessT || compOp == LEqual || compOp == NO_OP || compOp == NEqual)//考虑不等吗
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
	rid.slotNum = 0;//只是表示该索引属性相同时的最小索引记录（不存在）--找到对应的叶子节点
	int i, tmp;
	while (pIX_Node->is_leaf == 0)//非叶子节点
	{
		PageNum nextPage;
		for (i = 0; i < pIX_Node->keynum; i++)////非叶子节点的keynum>0,找第一个比它大的值
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
		//注意每次用时重新赋值
		pIX_Node->keys = (char*)(pIX_Node + 1);
		pIX_Node->rids = (RID*)(pIX_Node->keys + indexHandle->fileHeader.order * indexHandle->fileHeader.keyLength);
	}
	if (compOp == EQual)
	{
		for (i = 0; i < pIX_Node->keynum; i++)
		{
			tmp = compare(pIX_Node->keys + i * indexHandle->fileHeader.keyLength, value, indexHandle->fileHeader.attrType);
			if (tmp == -1)//小于的去掉
				continue;
			else
				break;//找到>或者=的索引
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
		else {//需要查找下一个节点
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
			//注意每次用时重新赋值
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
			if (tmp == -1)//小于的去掉
				continue;
			else
				break;//comOp=GE时找到>= ; cmpOp=G时找到>
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
			if (tmp <= 0)//<=的去掉
				continue;
			else
				break;//comOp=GE时找到>= ; cmpOp=G时找到>
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
			//注意每次用时重新赋值
			pIX_Node->keys = (char*)(pIX_Node + 1);
			pIX_Node->rids = (RID*)(pIX_Node->keys + indexHandle->fileHeader.order * indexHandle->fileHeader.keyLength);
			int j;
			for (j = 0; j < pIX_Node->keynum; j++)
			{
				tmp = compare(pIX_Node->keys + i * indexHandle->fileHeader.keyLength, value, indexHandle->fileHeader.attrType);
				if (tmp <= 0)//<=的去掉
					continue;
				else
					break;//comOp=GE时找到>= ; cmpOp=G时找到>
			}
			if (tmp == 1)//找到了第一个>的索引项
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
		return IX_EOF;//表示没有下一条记录
	GetThisPage(&indexScan->pIXIndexHandle->fileHandle, indexScan->pn, &indexScan->Pagehandle);
	IX_Node* pIX_Node = (IX_Node*)(indexScan->Pagehandle.pFrame->page.pData + sizeof(IX_FileHeader));
	//注意每次用时重新赋值
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
			return IX_GetNextEntry(indexScan, rid);//相等时查找下一个不等的
		}
	}
	//测试的该条记录符合条件
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

Tree_Node* getTree(IX_IndexHandle* indexHandle, IX_Node* pIX_Node, Tree_Node* parent, int childs)//每个节点只用保存最左边的孩子节点和右兄弟节点
{
	//创建一个节点，对于父节点则剩余的子节点个数-1
	childs--;
	PF_PageHandle pageHandle;
	Tree_Node* pNode = (Tree_Node*)malloc(sizeof(Tree_Node));
	if (pNode == NULL)
		return nullptr;
	//////////////都需要做的赋值操作///////////////////
	pNode->keyNum = pIX_Node->keynum; //可能为0--索引为空
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
	//当前节点为非叶子节点-中间节点
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
	else//当前节点为叶子节点
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