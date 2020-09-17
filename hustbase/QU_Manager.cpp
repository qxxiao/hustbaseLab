#include "StdAfx.h"
#include "QU_Manager.h"

void Init_Result(SelResult* res) {
	res->row_num = 0;
	res->col_num = 0;
	res->next_res = NULL;
}

void Destory_Result(SelResult* res) {
	for (int i = 0; i < res->row_num; i++) {
		for (int j = 0; j < res->col_num; j++) {
			delete[] res->res[i][j];
		}
		delete[] res->res[i];
	}
	if (res->next_res != NULL) {
		Destory_Result(res->next_res);
	}
	//delete res;//res��new�����
}

bool valueCompare(Value* v1, Value* v2, CompOp op)
{
	AttrType type = v1->type;
	switch (op)
	{
	case LessT:
		if (type == ints)
			return *(int*)v1->data < *(int*)v2->data;
		else if (type == floats)
			return *(float*)v1->data < *(float*)v2->data;
		else 
			return strcmp((char*)v1->data, (char*)v2->data)< 0;
		break;
	case LEqual:
		if (type == ints)
			return *(int*)v1->data <= *(int*)v2->data;
		else if (type == floats)
			return *(float*)v1->data <= *(float*)v2->data;
		else
			return strcmp((char*)v1->data, (char*)v2->data) <= 0;
		break;
	case GreatT:
		if (type == ints)
			return *(int*)v1->data > *(int*)v2->data;
		else if (type == floats)
			return *(float*)v1->data > *(float*)v2->data;
		else
			return strcmp((char*)v1->data, (char*)v2->data) > 0;
		break;
	case GEqual:
		if (type == ints)
			return *(int*)v1->data >= *(int*)v2->data;
		else if (type == floats)
			return *(float*)v1->data >= *(float*)v2->data;
		else
			return strcmp((char*)v1->data, (char*)v2->data) >= 0;
		break;
	case EQual:
		if (type == ints)
			return *(int*)v1->data == *(int*)v2->data;
		else if (type == floats)
			return *(float*)v1->data == *(float*)v2->data;
		else
			return strcmp((char*)v1->data, (char*)v2->data) == 0;
		break;
	case NEqual:
		if (type == ints)
			return *(int*)v1->data != *(int*)v2->data;
		else if (type == floats)
			return *(float*)v1->data != *(float*)v2->data;
		else
			return strcmp((char*)v1->data, (char*)v2->data) != 0;
		break;
	default:
		break;
	}
	return false;
}
//�����ǵڼ���������������
int isInTableList(int nRelations, char** relations, char* relName)
{
	for (int i = 0; i < nRelations; i++)
	{
		if (strcmp(relations[i], relName) == 0)
			return nRelations - 1 - i;
	}
	return -1;
}
/*�ȼ��Ϸ���
	1.������
	2.��ѯ�����Դ��ڶ���Ψһ(�����оٱ�����ԣ�
	3.�����еı����оٵı��е�һ������������������(���ϱ����ƻ����ޱ�����)Ψһ(�����оٱ�����ԣ�
	4.�����е�����һ��,�����Ҳ�ѯ���Ϊ�ռ�
	(ѡ��Ϊ*ʱ��nSelAttrs=1,����ΪNULL,�ֶ���Ϊ*)
*/
RC checkFields(int nSelAttrs, RelAttr** selAttrs, int nRelations, char** relations, int nConditions, Condition* conditions, bool& isSelAll,
	int& col_num, AttrType* type, int* offset, int* length, char fields[][20])
{
	RM_FileHandle fh1, fh2;
	RM_OpenFile("SYSTABLES", &fh1);
	RM_OpenFile("SYSCOLUMNS", &fh2);
	RM_FileScan fileScan;
	Con con[2];//��Ҫ�õ���������
	con[0].bLhsIsAttr = 1; con[0].bRhsIsAttr = 0;
	con[1].bLhsIsAttr = 1; con[1].bRhsIsAttr = 0;
	con[0].attrType = con[1].attrType = chars;//����,������
	con[0].LattrOffset = 0;
	con[1].LattrOffset = 21;
	con[0].compOp = con[1].compOp = EQual;
	RM_Record record;
	//����
	int nAttrs = 0;
	isSelAll = false;
	col_num = nSelAttrs;
	for (int i = 0; i < nRelations; i++)
	{
		con[0].Rvalue = relations[i];
		OpenScan(&fileScan, &fh1, 1, con);
		if (GetNextRec(&fileScan, &record) != SUCCESS)
		{
			CloseScan(&fileScan);
			RM_CloseFile(&fh2);
			RM_CloseFile(&fh1);
			return TABLE_NOT_EXIST;
		}
		CloseScan(&fileScan);
		nAttrs += *(int*)(record.pData + 21);//�ǰ�ȫ��
	}
	if (nSelAttrs == 1 && selAttrs[0]->relName == NULL && strcmp(selAttrs[0]->attrName, "*") == 0)
	{
		col_num = nAttrs;
		isSelAll = true;
	}
	if (col_num > 20)
	{
		RM_CloseFile(&fh2);
		RM_CloseFile(&fh1);
		return FIELD_REDUNDAN;
	}
	//tablename(21)	attrcount(int 4)
	//tablename(21)	attrname(21) attrtype(int)	attrlength(int)	attroffset(int)	ix_flag(1)	indexname(21)
	//����ѯ���ֶ�
	if (isSelAll)
	{
		int fieldTh = 0;
		for (int i = 0; i < nRelations; i++)
		{
			con[0].Rvalue = relations[nRelations - 1 - i];//�ӵ�һ����ʼ
			OpenScan(&fileScan, &fh2, 1, con);
			while (GetNextRec(&fileScan, &record) == SUCCESS)
			{
				memcpy(fields[fieldTh], record.pData + 21, 20);
				type[fieldTh] = *(AttrType*)(record.pData + 42);
				length[fieldTh] = *(int*)(record.pData + 42 + sizeof(int));
				offset[fieldTh] = *(int*)(record.pData + 42 + 2 * sizeof(int));
				fieldTh++;
			}
			CloseScan(&fileScan);
		}
	}
	else//�����ڶ���Ψһ��
	{
		for (int i = 0; i < col_num; i++)
		{
			if (selAttrs[col_num - 1 - i]->relName == NULL)//�ӵ�0���ֶο�ʼ
			{
				con[1].Rvalue = selAttrs[col_num - 1 - i]->attrName;
				OpenScan(&fileScan, &fh2, 1, &con[1]);
			}
			else
			{
				con[0].Rvalue = selAttrs[col_num - 1 - i]->relName;
				con[1].Rvalue = selAttrs[col_num - 1 - i]->attrName;
				OpenScan(&fileScan, &fh2, 2, con);
			}
			int nres = 0;
			char data[64 + 3 * sizeof(int)];
			int inTableList = -1;
			while (GetNextRec(&fileScan, &record) == SUCCESS)
			{
				int tmp;
				if ((tmp = isInTableList(nRelations, relations, record.pData)) == -1)
					continue;
				inTableList = nRelations - 1 - tmp;//��ȡ��tmp��������ָ��
				memcpy(data, record.pData, 64 + 3 * sizeof(int));
				nres++;
			}
			if (nres < 1)
			{
				CloseScan(&fileScan);
				RM_CloseFile(&fh2);
				RM_CloseFile(&fh1);
				return FIElD_NOT_EXIST;
			}
			else if (nres > 1)
			{
				CloseScan(&fileScan);
				RM_CloseFile(&fh2);
				RM_CloseFile(&fh1);
				return FIELD_REDUNDAN;
			}
			if (selAttrs[col_num - 1 - i]->relName == NULL)
				selAttrs[col_num - 1 - i]->relName = relations[inTableList];
			memcpy(fields[i], selAttrs[col_num - 1 - i]->attrName, 20);
			type[i] = *(AttrType*)(data + 42);
			length[i] = *(int*)(data + 42 + sizeof(int));
			offset[i] = *(int*)(data + 42 + 2 * sizeof(int));
			CloseScan(&fileScan);
		}
	}
	//��������е��ֶ�
	for (int i = 0; i < nConditions; i++)
	{
		AttrType typeLeft, typeRight;
		if (conditions[i].bLhsIsAttr == 1)
		{
			if (conditions[i].lhsAttr.relName == NULL)
			{
				con[1].Rvalue = conditions[i].lhsAttr.attrName;
				OpenScan(&fileScan, &fh2, 1, &con[1]);
			}
			else
			{
				con[0].Rvalue = conditions[i].lhsAttr.relName;
				con[1].Rvalue = conditions[i].lhsAttr.attrName;
				OpenScan(&fileScan, &fh2, 2, con);
			}
			int nres = 0;
			int inTableList = -1;
			while (GetNextRec(&fileScan, &record) == SUCCESS)
			{
				int tmp;
				if ((tmp = isInTableList(nRelations, relations, record.pData)) == -1)
					continue;
				inTableList = nRelations - 1 - tmp;
				nres++;
			}
			if (nres < 1)
			{
				CloseScan(&fileScan);
				RM_CloseFile(&fh2);
				RM_CloseFile(&fh1);
				return FIElD_NOT_EXIST;
			}
			else if (nres > 1)
			{
				CloseScan(&fileScan);
				RM_CloseFile(&fh2);
				RM_CloseFile(&fh1);
				return FIELD_REDUNDAN;
			}
			typeLeft = *(AttrType*)(record.pData + 42);
			if (conditions[i].lhsAttr.relName == NULL)
				conditions[i].lhsAttr.relName = relations[inTableList];
			CloseScan(&fileScan);
		}
		else
			typeLeft = conditions[i].lhsValue.type;
		if (conditions[i].bRhsIsAttr == 1)
		{
			if (conditions[i].rhsAttr.relName == NULL)
			{
				con[1].Rvalue = conditions[i].rhsAttr.attrName;
				OpenScan(&fileScan, &fh2, 1, &con[1]);
			}
			else
			{
				con[0].Rvalue = conditions[i].rhsAttr.relName;
				con[1].Rvalue = conditions[i].rhsAttr.attrName;
				OpenScan(&fileScan, &fh2, 2, con);
			}
			int nres = 0;
			int inTableList = -1;
			while (GetNextRec(&fileScan, &record) == SUCCESS)
			{
				int tmp;
				if ((tmp = isInTableList(nRelations, relations, record.pData)) == -1)
					continue;
				inTableList = nRelations - 1 - tmp;
				nres++;
			}
			if (nres < 1)
			{
				CloseScan(&fileScan);
				RM_CloseFile(&fh2);
				RM_CloseFile(&fh1);
				return FIElD_NOT_EXIST;
			}
			else if (nres > 1)
			{
				CloseScan(&fileScan);
				RM_CloseFile(&fh2);
				RM_CloseFile(&fh1);
				return FIELD_REDUNDAN;
			}
			typeRight = *(AttrType*)(record.pData + 42);
			if (conditions[i].rhsAttr.relName == NULL)
				conditions[i].rhsAttr.relName = relations[inTableList];
			CloseScan(&fileScan);
		}
		else
			typeRight = conditions[i].rhsValue.type;
		if (typeLeft != typeRight)
		{
			RM_CloseFile(&fh2);
			RM_CloseFile(&fh1);
			return FIELD_TYPE_MISMATCH;
		}
	}
	RM_CloseFile(&fh2);
	RM_CloseFile(&fh1);
	return SUCCESS;
}


Con* conditonToCon(int nConditions, Condition* conditions)
{
	if (nConditions <= 0)
		return nullptr;
	RM_FileHandle fh2;
	RM_OpenFile("SYSCOLUMNS", &fh2);
	RM_FileScan fileScan;
	Con condition[2];//��Ҫ�õ���������
	condition[0].bLhsIsAttr = 1; condition[0].bRhsIsAttr = 0;
	condition[1].bLhsIsAttr = 1; condition[1].bRhsIsAttr = 0;
	condition[0].attrType = condition[1].attrType = chars;//����,������
	condition[0].LattrOffset = 0;
	condition[1].LattrOffset = 21;
	condition[0].compOp = condition[1].compOp = EQual;
	RM_Record record;
	//��װn���Ƚ�����
	Con* cons = new Con[nConditions];
	for (int i = 0; i < nConditions; i++)
	{
		AttrType lattr, rattr;
		(cons + i)->compOp = (conditions + nConditions - 1 - i)->op;
		if ((conditions + nConditions - 1 - i)->bLhsIsAttr == 1)//���������
		{
			condition[0].Rvalue = (conditions + nConditions - 1 - i)->lhsAttr.relName;
			condition[1].Rvalue = (conditions + nConditions - 1 - i)->lhsAttr.attrName;
			OpenScan(&fileScan, &fh2, 2, condition);
			if (GetNextRec(&fileScan, &record) != SUCCESS)//���Ҹ����Լ�¼
			{
				RM_CloseFile(&fh2);
				CloseScan(&fileScan);
				free(cons);
				return nullptr;
			}
			lattr = *(AttrType*)(record.pData + 42);
			(cons + i)->bLhsIsAttr = 1;
			(cons + i)->LattrLength = *(int*)(record.pData + 42 + sizeof(int));
			(cons + i)->LattrOffset = *(int*)(record.pData + 42 + 2 * sizeof(int));
			CloseScan(&fileScan);
		}
		else {//�����ֵ
			lattr = (conditions + nConditions - 1 - i)->lhsValue.type;
			(cons + i)->bLhsIsAttr = 0;
			(cons + i)->Lvalue = (conditions + nConditions - 1 - i)->lhsValue.data;
		}

		if ((conditions + nConditions - 1 - i)->bRhsIsAttr == 1)//�ұ�������
		{
			condition[0].Rvalue = (conditions + nConditions - 1 - i)->rhsAttr.relName;
			condition[1].Rvalue = (conditions + nConditions - 1 - i)->rhsAttr.attrName;
			OpenScan(&fileScan, &fh2, 2, condition);
			if (GetNextRec(&fileScan, &record) != SUCCESS)//���Ҹ����Լ�¼
			{
				RM_CloseFile(&fh2);
				CloseScan(&fileScan);
				free(cons);
				return nullptr;
			}
			rattr = *(AttrType*)(record.pData + 42);
			(cons + i)->bRhsIsAttr = 1;
			(cons + i)->RattrLength = *(int*)(record.pData + 42 + sizeof(int));
			(cons + i)->RattrOffset = *(int*)(record.pData + 42 + 2 * sizeof(int));
			CloseScan(&fileScan);
		}
		else {
			rattr = (conditions + nConditions - 1 - i)->rhsValue.type;
			(cons + i)->bRhsIsAttr = 0;
			(cons + i)->Rvalue = (conditions + nConditions - 1 - i)->rhsValue.data;
		}
		if (lattr == rattr)
			(cons + i)->attrType = lattr;
		else {//�������Բ�ͬ
			//if (lattr == chars || rattr == chars)//ֻ��һ����chars
			RM_CloseFile(&fh2);
			delete[]cons;
			return nullptr;
		}
	}
	return cons;
}
//������pData:��ŷ�������������һ��char*��ָ��һ����ѯ�ֶ����ڱ��һ����¼
bool insertSelRes(SelResult* res, char** pData)
{
	if (res->col_num <= 0)
		return false;
	//�ҵ������β�ڵ�
	while (res->next_res)
		res = res->next_res;
	//����100����¼����һ���µĽڵ�
	if (res->row_num == 100)
	{
		res->next_res = new SelResult;
		res->next_res->next_res = nullptr;
		res->next_res->row_num = 0;
		res->next_res->col_num = res->col_num;
		memcpy(res->next_res->type, res->type, sizeof(res->type));
		memcpy(res->next_res->offset, res->offset, sizeof(res->offset));
		memcpy(res->next_res->length, res->length, sizeof(res->length));
		memcpy(res->next_res->fields, res->fields, sizeof(res->fields));
		res = res->next_res;
	}
	//res->res[res->row_num] = new char* [res->col_num];
	//for (int i = 0; i < res->col_num; i++)
	//{
	//	res->res[res->row_num][i] = new char[res->length[i]];
	//	memcpy(res->res[res->row_num][i], pData[i] + res->offset[i], res->length[i]);
	//}
	int recSize=0;
	int distance=0;
	for(int i=0;i<res->col_num;i++)
		recSize+=res->length[i];
	res->res[res->row_num]=new char*;
	*res->res[res->row_num]=new char[recSize];
	for(int i=0;i<res->col_num;i++)
	{
		memcpy(*res->res[res->row_num]+distance,pData[i]+res->offset[i],res->length[i]);
		distance+=res->length[i];
	}
	res->row_num++;
	return true;
}

void updateRes(SelResult* res)
{
	while(res)
	{
		for(int i=0;i<res->col_num;i++)
		{
			if(i==0)
				res->offset[i]=0;
			else
				res->offset[i]=res->offset[i-1]+res->length[i-1];
		}
		res=res->next_res;
	}
}

RC multiTablesSelect(int nSelAttrs, RelAttr** selAttrs, bool isSelAll, int nRelations, char** relations, RM_FileHandle* nRelFileHandle, int *consNum, Con** cons, int nConditons, Condition* conditions,
	char** pData, int curRelation, SelResult* res)//�������������ķֱ�ָ��n�����һ����¼������Ƿ���������������¼��
{
	//�����curRelation�������ϲ�ı��������
	RM_FileHandle fh1,fh2;
	RM_FileScan fs1,fs2, fileScan;
	RM_Record record1,record2, record;
	Con con[2];//��Ҫ�õ���������
	con[0].bLhsIsAttr = 1; con[0].bRhsIsAttr = 0;
	con[1].bLhsIsAttr = 1; con[1].bRhsIsAttr = 0;
	con[0].attrType = con[1].attrType = chars;//����,������
	con[0].LattrOffset = 0;
	con[1].LattrOffset = 21;
	con[0].compOp = con[1].compOp = EQual;
	///////////////���һ�������ļ�¼��������///////////
	if (curRelation >= nRelations)//������һ�������ļ�¼����������
	{
		char** pData2 = new char* [res->col_num];
		if (isSelAll)
		{
			RM_OpenFile("SYSTABLES", &fh1);
			int pos = 0;
			for (int i = 0; i < nRelations; i++)
			{
				con[0].Rvalue = relations[nRelations - 1 - i];
				OpenScan(&fs1, &fh1, 1, con);
				GetNextRec(&fs1, &record1);
				int colNum = *(int*)(record1.pData + 21);
				for (int j = 0; j < colNum; j++)
				{
					pData2[pos] = pData[i];
					pos++;
				}
				CloseScan(&fs1);
			}
			RM_CloseFile(&fh1);
		}
		else
		{
			for (int i = 0; i < res->col_num; i++)
				pData2[i] = pData[isInTableList(nRelations, relations, selAttrs[nSelAttrs - 1 - i]->relName)];//nSelAttrs==res.col_num
		}
		insertSelRes(res, pData2);
		delete[]pData2;
		return SUCCESS;
	}
	///////////���ӵ�curRelation��////////////////////////
	OpenScan(&fileScan, &nRelFileHandle[curRelation], consNum[curRelation], cons[curRelation]);
	while (GetNextRec(&fileScan, &record) == SUCCESS)
	{
		memcpy(pData[curRelation], record.pData, nRelFileHandle[curRelation].fileSubHeader->recordSize);
		bool isSel = true;//������¼�Ƿ�����Ŀǰ����
		if (curRelation != 0)//�ж��뱾����ص������Ƿ����
		{
			for (int i = 0; i < nConditons; i++)
			{
				int preTable;
				Value vl, vr;
				if (isInTableList(nRelations, relations, conditions[i].lhsAttr.relName) == curRelation && (preTable = isInTableList(nRelations, relations, conditions[i].rhsAttr.relName)) < curRelation)
				{
					RM_OpenFile("SYSCOLUMNS", &fh2);
					con[0].Rvalue = conditions[i].lhsAttr.relName;
					con[1].Rvalue = conditions[i].lhsAttr.attrName;
					OpenScan(&fs2, &fh2, 2, con);
					GetNextRec(&fs2, &record2);
					vl.data = pData[curRelation] + *(int*)(record2.pData + 42 + 2 * sizeof(int));
					vl.type = *(AttrType*)(record2.pData + 42);
					CloseScan(&fs2);
					con[0].Rvalue = conditions[i].rhsAttr.relName;
					con[1].Rvalue = conditions[i].rhsAttr.attrName;
					OpenScan(&fs2, &fh2, 2, con);
					GetNextRec(&fs2, &record2);
					vr.data = pData[preTable] + *(int*)(record2.pData + 42 + 2 * sizeof(int));
					vr.type= *(AttrType*)(record2.pData + 42);
					CloseScan(&fs2);
					RM_CloseFile(&fh2);
					isSel = valueCompare(&vl, &vr, conditions[i].op);
					if (!isSel)
						break;
				}
				if (isInTableList(nRelations, relations, conditions[i].rhsAttr.relName) == curRelation && (preTable = isInTableList(nRelations, relations, conditions[i].lhsAttr.relName)) < curRelation)
				{
					RM_OpenFile("SYSCOLUMNS", &fh2);
					con[0].Rvalue = conditions[i].rhsAttr.relName;
					con[1].Rvalue = conditions[i].rhsAttr.attrName;
					OpenScan(&fs2, &fh2, 2, con);
					GetNextRec(&fs2, &record2);
					vr.data = pData[curRelation] + *(int*)(record2.pData + 42 + 2 * sizeof(int));
					vr.type = *(AttrType*)(record2.pData + 42);
					CloseScan(&fs2);
					con[0].Rvalue = conditions[i].lhsAttr.relName;
					con[1].Rvalue = conditions[i].lhsAttr.attrName;
					OpenScan(&fs2, &fh2, 2, con);
					GetNextRec(&fs2, &record2);
					vl.data = pData[preTable] + *(int*)(record2.pData + 42 + 2 * sizeof(int));
					vl.type = *(AttrType*)(record2.pData + 42);
					CloseScan(&fs2);
					RM_CloseFile(&fh2);
					isSel = valueCompare(&vl, &vr, conditions[i].op);
					if (!isSel)
						break;
				}
			}
		}
		if (isSel)
			multiTablesSelect(nSelAttrs, selAttrs, isSelAll, nRelations, relations, nRelFileHandle, consNum, cons, nConditons, conditions, pData, curRelation + 1, res);
	}
	CloseScan(&fileScan);
	return SUCCESS;
}

//�����ʱ���ֶδ���Ϊ˳�򣨲����е�������
//ͼ����ʾ�Ľ�������ʾ100����¼��������Խ�磬�ַ���<20
RC Select(int nSelAttrs, RelAttr** selAttrs, int nRelations, char** relations, int nConditions, Condition* conditions, SelResult* res)
{
	int col_num;            //ע���ж�Ϊ*ʱҪ�޸�
	AttrType type[20];		//��ѯ�ĸ��ֶε���������
	int offset[20];			//��ѯ�ĸ��ֶ��ڼ�¼�е�ƫ��
	int length[20];			//��ѯ�ĸ��ֶεĳ���
	char fields[20][20];	//��ѯ�ĸ��ֶε��ֶ�����<20
	bool isSelAll = false;
	RC rc;
	//���Ϸ��ԣ����ҽ�ʡ�Եı������
	if ((rc = checkFields(nSelAttrs, selAttrs, nRelations, relations, nConditions, conditions, isSelAll, col_num, type, offset, length, fields)) != SUCCESS)
		return rc;
	res->col_num = col_num;
	res->row_num = 0;
	res->next_res = nullptr;//ȷ��Ϊ��
	//��ʱ���ֶε�˳������ȷ��
	memcpy(res->type, type, sizeof(type));
	memcpy(res->offset, offset, sizeof(offset));
	memcpy(res->length, length, sizeof(length));
	memcpy(res->fields, fields, sizeof(fields));
	///////////�����ѯ//////////////////
	if (nRelations == 1)
	{
		Con* cons = conditonToCon(nConditions, conditions);//0����������nullptr
		RM_FileHandle fileHandle;
		RM_OpenFile(relations[0], &fileHandle);
		RM_FileScan fileScan;
		RM_Record record;
		OpenScan(&fileScan, &fileHandle, nConditions, cons);
		char** pData = new char* [col_num];
		while (GetNextRec(&fileScan, &record) == SUCCESS)
		{
			for (int i = 0; i < col_num; i++)
				pData[i] = record.pData;
			insertSelRes(res, pData);
		}
		updateRes(res);
		delete[]pData;
		if(cons) delete[]cons;
		CloseScan(&fileScan);
		RM_CloseFile(&fileHandle);
		return SUCCESS;
	}
	///////////����ѯ//////////////////
	//��conditons�ֳ�2��
	int* consNum = new int[nRelations];//����ÿ�����м���������
	for(int i=0;i<nRelations;i++)
		consNum[i]=0;
	Con** cons = new Con * [nRelations];//����ÿ����ĵ�������
	for (int i = 0; i < nRelations; i++)
		cons[i] = new Con[nConditions];//���������ռ�

	int nConditions_2 = 0;
	Condition* conditions_2 = new Condition[nConditions];
	for (int i = 0; i < nConditions; i++)
	{
		if (conditions[i].bLhsIsAttr == 0 && conditions[i].bRhsIsAttr == 0)
		{
			if (!valueCompare(&conditions[i].lhsValue, &conditions[i].rhsValue, conditions[i].op))
			{
				delete[]conditions_2;
				for (int j = 0; j < nRelations; j++)
					delete[]cons[i];
				delete[]cons;
				return SUCCESS;
			}
			continue;
		}
		else if (conditions[i].bLhsIsAttr == 1 && conditions[i].bRhsIsAttr == 1 && strcmp(conditions[i].lhsAttr.relName, conditions[i].rhsAttr.relName) != 0)
		{
			conditions_2[nConditions_2] = conditions[i];
			nConditions_2++;
		}
		else
		{
			int relOrder;
			if (conditions[i].bLhsIsAttr == 1)
				relOrder = isInTableList(nRelations, relations, conditions[i].lhsAttr.relName);
			else
				relOrder = isInTableList(nRelations, relations, conditions[i].rhsAttr.relName);
			Con *con_t= conditonToCon(1, &conditions[i]);
			cons[relOrder][consNum[relOrder]] = *con_t;
			delete con_t;
			consNum[relOrder]++;
		}
	}
	//�����������������ӵ��Ȳ�ѯ
	//ֱ�ӽ�ȫ������Լ����ݵ��ڴ棬�ݹ鴦��������
	RM_FileHandle* nRelFileHandle = new RM_FileHandle[nRelations];
	char** pData = new char* [nRelations];//���һ�����Ӻ��ָ��
	for (int i = 0; i < nRelations; i++)
	{
		RM_OpenFile(relations[nRelations - 1 - i], nRelFileHandle + i);
		pData[i] = new char[nRelFileHandle[i].fileSubHeader->recordSize];
	}
	rc = multiTablesSelect(nSelAttrs, selAttrs, isSelAll, nRelations, relations, nRelFileHandle, consNum, cons, nConditions_2, conditions_2, pData, 0, res);
	updateRes(res);
	for (int i = 0; i < nRelations; i++)
	{
		delete[]pData[i];
		RM_CloseFile(nRelFileHandle + i);
	}
	delete[]pData;
	delete[] nRelFileHandle;
	delete[]conditions_2;
	for (int i = 0; i < nRelations; i++)
		delete[]cons[i];
	delete[]cons;
	delete[]consNum;
	return rc;
}

RC Query(char* sql, SelResult* res) {
	sqlstr* sql_str = NULL;
	RC rc;
	sql_str = get_sqlstr();
	rc = parse(sql, sql_str);//ֻ�����ַ��ؽ��SUCCESS��SQL_SYNTAX

	if (rc == SQL_SYNTAX)
		return rc;
	selects sel = sql_str->sstr.sel;
	return Select(sel.nSelAttrs, sel.selAttrs, sel.nRelations, sel.relations, sel.nConditions, sel.conditions, res);
}