#include "stdafx.h"
#include "EditArea.h"
#include "SYS_Manager.h"
#include "QU_Manager.h"
#include <iostream>
#include"HustBaseDoc.h"
#include<set>
bool isOpen = false;
void ExecuteAndMessage(char* sql, CEditArea* editArea) {//根据执行的语句类型在界面上显示执行结果。此函数需修改
	std::string s_sql = sql;
	RC rc;
	if (s_sql.find("select") == 0)
	{
		SelResult* res = new SelResult;
		Init_Result(res);
		rc = Query(sql, res);
		if (rc == SUCCESS)
		{
			CHustBaseDoc* pDoc = CHustBaseDoc::GetDoc();
			pDoc->selColNum = res->col_num;
			pDoc->selRowNum = 1;
			SelResult* res_t = res;
			while (res_t)
			{
				pDoc->selRowNum += res_t->row_num;
				res_t = res_t->next_res;
			}
			res_t = res;
			for (int i = 0, row = -1; i < pDoc->selRowNum && i < 101; i++, row++)//显示最多100条记录
			{
				if (i != 1 && i % 100 == 1)
				{
					res_t = res_t->next_res;
					row = 0;
				}
				for (int j = 0; j < pDoc->selColNum; j++)
				{
					if (i == 0)//复制表头
						memcpy(pDoc->selResult[i][j], res_t->fields[j], 20);
					else
					{
						CString number;
						if (res_t->type[j] == chars)
							//memcpy(pDoc->selResult[i][j], res_t->res[row][j], res_t->length[j]);
							memcpy(pDoc->selResult[i][j], *(res_t->res[row])+res_t->offset[j], res_t->length[j]);
						else if (res_t->type[j] == ints)
						{
							number.Format(_T("%d"), *(int*)(*(res_t->res[row])+res_t->offset[j]));
							strcpy(pDoc->selResult[i][j], number);
						}
						else
						{
							number.Format(_T("%f"), *(float*)(*(res_t->res[row])+res_t->offset[j]));
							strcpy(pDoc->selResult[i][j], number);
						}
					}
				}
			}
			pDoc->isEdit = 1;
			editArea->showSelResult(pDoc->selRowNum, pDoc->selColNum);
		}
		Destory_Result(res);
		if (rc == SQL_SYNTAX)//只是用来提示语法错误
			execute(sql);
	}
	else rc = execute(sql);
	int row_num = 0;
	char** messages;
	switch (rc) {
	case SUCCESS:
		if (s_sql.find("select") == 0)
			break;
		row_num = 1;
		messages = new char* [row_num];
		messages[0] = "操作成功";
		editArea->ShowMessage(row_num, messages);
		delete[] messages;
		break;
	case SQL_SYNTAX:
		row_num = 1;
		messages = new char* [row_num];
		messages[0] = "有语法错误";
		editArea->ShowMessage(row_num, messages);
		delete[] messages;
		break;
	case TABLE_NOT_EXIST:
		row_num = 1;
		messages = new char* [row_num];
		messages[0] = "表不存在";
		editArea->ShowMessage(row_num, messages);
		delete[] messages;
		break;
	case FIELD_MISSING:
		row_num = 1;
		messages = new char* [row_num];
		messages[0] = "插入的字段太少";
		editArea->ShowMessage(row_num, messages);
		delete[] messages;
		break;
	case FIELD_REDUNDAN:
		row_num = 1;
		messages = new char* [row_num];
		messages[0] = "插入的字段太多";
		editArea->ShowMessage(row_num, messages);
		delete[] messages;
		break;
	case FIELD_TYPE_MISMATCH:
		row_num = 1;
		messages = new char* [row_num];
		messages[0] = "字段类型不匹配";
		editArea->ShowMessage(row_num, messages);
		delete[] messages;
		break;
	case FIElD_NOT_EXIST:
		row_num = 1;
		messages = new char* [row_num];
		messages[0] = "字段不存在";
		editArea->ShowMessage(row_num, messages);
		delete[] messages;
		break;
	case TABLE_CREATE_REPEAT:
		row_num = 1;
		messages = new char* [row_num];
		messages[0] = "表重复创建";
		editArea->ShowMessage(row_num, messages);
		delete[] messages;
		break;
	case SEL_TABLE_NOT_EXIST:
		row_num = 1;
		messages = new char* [row_num];
		messages[0] = "查询的表不存在";
		editArea->ShowMessage(row_num, messages);
		delete[] messages;
		break;
	case SEL_FIELD_TOOMANY:
		row_num = 1;
		messages = new char* [row_num];
		messages[0] = "查询的字段太多（不超过20）";
		editArea->ShowMessage(row_num, messages);
		delete[] messages;
		break;
	case SEL_FIELD1_NOT_EXIST:
		row_num = 1;
		messages = new char* [row_num];
		messages[0] = "查询的字段不存在";
		editArea->ShowMessage(row_num, messages);
		delete[] messages;
		break;
	case SEL_FIELD1_AMBIGUOUS:
		row_num = 1;
		messages = new char* [row_num];
		messages[0] = "查询的字段不明确（有同名字段）";
		editArea->ShowMessage(row_num, messages);
		delete[] messages;
		break;
	case SEL_FIELD2_NOT_EXIST:
		row_num = 1;
		messages = new char* [row_num];
		messages[0] = "条件中的字段不存在";
		editArea->ShowMessage(row_num, messages);
		delete[] messages;
		break;
	case SEL_FIELD2_AMBIGUOUS:
		row_num = 1;
		messages = new char* [row_num];
		messages[0] = "条件中的字段不明确（有同名字段）";
		editArea->ShowMessage(row_num, messages);
		delete[] messages;
		break;
	case SEL_TYPE_MISMATCH:
		row_num = 1;
		messages = new char* [row_num];
		messages[0] = "条件中的字段的类型不匹配（实现中采用类型一致）";
		editArea->ShowMessage(row_num, messages);
		delete[] messages;
		break;
	default:
		row_num = 1;
		messages = new char* [row_num];
		messages[0] = "功能未实现";
		editArea->ShowMessage(row_num, messages);
		delete[] messages;
		break;
	}
}

RC execute(char* sql) {
	sqlstr* sql_str = NULL;
	RC rc;
	sql_str = get_sqlstr();
	rc = parse(sql, sql_str);//只有两种返回结果SUCCESS和SQL_SYNTAX
	if (rc == SUCCESS)
	{
		int i = 0;
		CHustBaseDoc* pDoc;
		pDoc = CHustBaseDoc::GetDoc();
		switch (sql_str->flag)
		{
		case 1:
			//判断SQL语句为select语句
			break;
		case 2:
			//判断SQL语句为insert语句
				//inserts ins = sql_str->sstr.ins;
			rc = Insert(sql_str->sstr.ins.relName, sql_str->sstr.ins.nValues, sql_str->sstr.ins.values);
			//if (rc == SUCCESS)
			//	pDoc->m_pListView->displayTabInfo(sql_str->sstr.ins.relName);
			break;
		case 3:
			//判断SQL语句为update语句
				//updates upd = sql_str->sstr.upd;
			rc = Update(sql_str->sstr.upd.relName, sql_str->sstr.upd.attrName, &sql_str->sstr.upd.value, sql_str->sstr.upd.nConditions, sql_str->sstr.upd.conditions);
			//if (rc == SUCCESS)
			//	pDoc->m_pListView->displayTabInfo(sql_str->sstr.upd.relName);
			break;

		case 4:
			//判断SQL语句为delete语句
				//deletes del = sql_str->sstr.del;
			rc = Delete(sql_str->sstr.del.relName, sql_str->sstr.del.nConditions, sql_str->sstr.del.conditions);
			//if (rc == SUCCESS)
			//	pDoc->m_pListView->displayTabInfo(sql_str->sstr.del.relName);
			break;
		case 5:
			//判断SQL语句为createTable语句
				//createTable ct = sql_str->sstr.cret;
			rc = CreateTable(sql_str->sstr.cret.relName, sql_str->sstr.cret.attrCount, sql_str->sstr.cret.attributes);
			//if (rc == SUCCESS)
			//	pDoc->m_pTreeView->PopulateTree();
			break;
		case 6:
			//判断SQL语句为dropTable语句
				//dropTable dt = sql_str->sstr.drt;
			rc = DropTable(sql_str->sstr.drt.relName);
			//if (rc == SUCCESS)
			//	pDoc->m_pTreeView->PopulateTree();
			break;
		case 7:
			//判断SQL语句为createIndex语句
				//createIndex ci = sql_str->sstr.crei;
			rc = CreateIndex(sql_str->sstr.crei.indexName, sql_str->sstr.crei.relName, sql_str->sstr.crei.attrName);
			//if (rc == SUCCESS)
			//	pDoc->m_pTreeView->PopulateTree();
			break;
		case 8:
			//判断SQL语句为dropIndex语句
				//dropIndex di = sql_str->sstr.dri;
			rc = DropIndex(sql_str->sstr.dri.indexName);
			//if (rc == SUCCESS)
			//	pDoc->m_pTreeView->PopulateTree();
			break;
		case 9:
			//判断为help语句，可以给出帮助提示
			AfxMessageBox("HustBase");
			break;
		case 10:
			//判断为exit!语句，可以由此进行退出操作
			AfxMessageBox("  欢迎再次使用HustBase\n\tBye......");
			AfxGetMainWnd()->SendMessage(WM_CLOSE);
			break;
		}
	}
	else
	{
		AfxMessageBox(sql_str->sstr.errors);//弹出警告框，sql语句词法解析错误信息
		return rc;
	}
	return rc;
}

RC CreateDB(char* dbpath, char* dbname) {
	CString path = dbpath;
	path += "\\";
	path += dbname;
	CreateDirectory(path, NULL);
	SetCurrentDirectory(path);
	CString tablePath = path + "\\SYSTABLES";
	CString colPath = path + "\\SYSCOLUMNS";
	//创建系统表 创建系统属性表
	//tablename(21)	attrcount(int 4)
	//tablename(21)	attrname(21) attrtype(int)	attrlength(int)	attroffset(int)	ix_flag(1)	indexname(21)
	RM_CreateFile((LPTSTR)(LPCTSTR)tablePath, 21 + sizeof(int));
	RM_CreateFile((LPTSTR)(LPCTSTR)colPath, 64 + 3 * sizeof(int));
	return SUCCESS;
}

RC DropDB(char* dbname) {
	CString path = dbname;
	CFileFind finder;
	bool isNotEmpty = finder.FindFile(path + "\\*.*");
	//if (!isNotEmpty)
	//	return DB_NOT_EXIST;
	while (isNotEmpty)	//默认是标准的数据库
	{
		isNotEmpty = finder.FindNextFile();
		CString fileName = finder.GetFilePath();
		if (!finder.IsDots())
		{
			if (!finder.IsDirectory())
				DeleteFile(fileName);
			else
				DropDB((LPTSTR)(LPCTSTR)fileName);
		}

	}
	finder.Close();
	RemoveDirectory(dbname);
	return SUCCESS;
}

/*打开特定数据库*/
RC OpenDB(char* dbname) {
	CFileFind finder;
	bool isDB = finder.FindFile(CString(dbname) + "\\SYSTABLES") && finder.FindFile(CString(dbname) + "\\SYSCOLUMNS");
	finder.Close();
	if (!isDB)
		return DB_NOT_EXIST;
	isOpen = true;
	//SetCurrentDirectory(dbname);
	return SUCCESS;
}

//关闭当前数据库，关闭所有打开的文件并且清理缓冲区
RC CloseDB() {
	isOpen = false;
	return SUCCESS;
}

bool CanButtonClick() {//需要重新实现
	return isOpen;
}

RC CreateTable(char* relName, int attrCount, AttrInfo* attributes)
{
	//判断列名重复
	RC rc;
	if(strlen(relName)>=21)
		return TABLE_NAME_ILLEGAL;
	int recordSize = 0;
	for (int i = 0; i < attrCount; i++)
		recordSize += (attributes + i)->attrLength;
	if (RM_CreateFile(relName, recordSize) != SUCCESS)
		return TABLE_EXIST;
	std::set<CString> attrNameSet;
	for (int i = 0; i < attrCount; i++)
	{
		if (!attrNameSet.insert(attributes[i].attrName).second)
		{
			DeleteFile(relName);
			return TABLE_COLUMN_ERROR;
		}
	}
	//tablename(21)	attrcount(int 4)
	//tablename(21)	attrname(21) attrtype(int)	attrlength(int)	attroffset(int)	ix_flag(1)	indexname(21)
	RM_FileHandle fh1, fh2;
	RM_OpenFile("SYSTABLES", &fh1);
	RM_OpenFile("SYSCOLUMNS", &fh2);
	char pData[21 + sizeof(int)] = { 0 };
	char pData2[64 + 3 * sizeof(int)] = { 0 };
	RID rid;
	strcpy(pData, relName);
	*(int*)(pData + 21) = attrCount;
	if ((rc = InsertRec(&fh1, pData, &rid)) != SUCCESS)
		return rc;
	int offset = 0;
	for (int i = 0; i < attrCount; i++)//向COLUMNS表中添加属性记录
	{
		strcpy(pData2, relName); strcpy(pData2 + 21, (attributes + i)->attrName);
		*(AttrType*)(pData2 + 42) = (attributes + i)->attrType;
		*(int*)(pData2 + 42 + sizeof(int)) = (attributes + i)->attrLength;
		*(int*)(pData2 + 42 + 2 * sizeof(int)) = offset;
		*(pData2 + 42 + 3 * sizeof(int)) = '0';
		*(pData2 + 43 + 3 * sizeof(int)) = '\0';
		if ((rc = InsertRec(&fh2, pData2, &rid)) != SUCCESS)
			return rc;
		offset += (attributes + i)->attrLength;
	}
	RM_CloseFile(&fh1);
	RM_CloseFile(&fh2);
	return SUCCESS;
}
RC DropTable(char* relName)
{
	RM_FileHandle fh1, fh2;
	RM_OpenFile("SYSTABLES", &fh1);//打开SYSTABLES
	RM_OpenFile("SYSCOLUMNS", &fh2);
	RM_FileScan fileScan;
	Con condition;
	condition.bLhsIsAttr = 1, condition.bRhsIsAttr = 0;
	condition.attrType = chars;
	condition.LattrOffset = 0;
	condition.compOp = EQual;
	condition.Rvalue = relName;
	OpenScan(&fileScan, &fh1, 1, &condition);
	RM_Record record;
	if (GetNextRec(&fileScan, &record) != SUCCESS)//没有该表
	{
		RM_CloseFile(&fh1);
		RM_CloseFile(&fh2);
		CloseScan(&fileScan);
		return TABLE_NOT_EXIST;
	}
	int attcounts = *(int*)(record.pData + 21);
	DeleteRec(&fh1, &(record.rid));//删除SYSTABLES的该表记录
	CloseScan(&fileScan);
	OpenScan(&fileScan, &fh2, 1, &condition);
	for (int i = 0; i < attcounts; i++)
	{
		if (GetNextRec(&fileScan, &record) != SUCCESS)
		{
			RM_CloseFile(&fh1);
			RM_CloseFile(&fh2);
			CloseScan(&fileScan);
			return SUCCESS;
		}
		if (*(record.pData + 42 + 3 * sizeof(int)) == '1')
			DeleteFile(record.pData + 43 + 3 * sizeof(int));
		DeleteRec(&fh2, &(record.rid));
	}
	DeleteFile(relName);
	RM_CloseFile(&fh1);
	RM_CloseFile(&fh2);
	CloseScan(&fileScan);
	return SUCCESS;
}

RC CreateIndex(char* indexName, char* relName, char* attrName)
{
	RM_FileHandle fh1, fh2;
	RM_OpenFile("SYSTABLES", &fh1);
	RM_OpenFile("SYSCOLUMNS", &fh2);
	RM_FileScan fileScan;
	Con condition[2];
	condition[0].bLhsIsAttr = 1; condition[0].bRhsIsAttr = 0;
	condition[1].bLhsIsAttr = 1; condition[1].bRhsIsAttr = 0;
	condition[0].attrType = chars;
	condition[1].attrType = chars;
	condition[0].LattrOffset = 0;
	condition[1].LattrOffset = 21;
	condition[0].compOp = EQual;
	condition[1].compOp = EQual;
	condition[0].Rvalue = relName;
	condition[1].Rvalue = attrName;
	RM_Record record;
	OpenScan(&fileScan, &fh1, 1, condition);
	if (GetNextRec(&fileScan, &record) != SUCCESS)
	{
		RM_CloseFile(&fh1);
		RM_CloseFile(&fh2);
		CloseScan(&fileScan);
		return TABLE_NOT_EXIST;
	}
	CloseScan(&fileScan);
	OpenScan(&fileScan, &fh2, 2, condition);
	if (GetNextRec(&fileScan, &record) != SUCCESS)
	{
		RM_CloseFile(&fh1);
		RM_CloseFile(&fh2);
		CloseScan(&fileScan);
		return FIElD_NOT_EXIST;
	}
	CloseScan(&fileScan);
	//tablename(21)	attrname(21) attrtype(int)	attrlength(int)	attroffset(int)	ix_flag(1)	indexname(21)
	if (*(record.pData + 42 + 3 * sizeof(int)) == '1')
	{
		RM_CloseFile(&fh1);
		RM_CloseFile(&fh2);
		return INDEX_EXIST;
	}
	*(record.pData + 42 + 3 * sizeof(int)) = '1';
	memcpy(record.pData + 43 + 3 * sizeof(int), indexName, 21);
	PF_PageHandle pageHandle;
	GetThisPage(fh2.pfileHandle, record.rid.pageNum, &pageHandle);
	MarkDirty(&pageHandle);
	UnpinPage(&pageHandle);
	int offset = *(int*)(record.pData + 42 + 2 * sizeof(int));
	//UpdateRec(&fh2, &record);
	//创建索引文件
	RC rc = CreateIndex(indexName, *(AttrType*)(record.pData + 42), *(int*)(record.pData + 42 + sizeof(int)));
	if (rc != SUCCESS)
	{
		RM_CloseFile(&fh1);
		RM_CloseFile(&fh2);
		return INDEX_EXIST;
	}
	IX_IndexHandle indexHandle;
	OpenIndex(indexName, &indexHandle);
	RM_FileHandle fh;
	RM_OpenFile(relName, &fh);
	OpenScan(&fileScan, &fh, 0, nullptr);
	while (GetNextRec(&fileScan, &record) == SUCCESS)
		InsertEntry(&indexHandle, record.pData + offset, &record.rid);
	CloseIndex(&indexHandle);
	RM_CloseFile(&fh);
	RM_CloseFile(&fh2);
	RM_CloseFile(&fh1);
	return SUCCESS;
}
RC DropIndex(char* indexName)
{
	RM_FileHandle fh2;
	RM_OpenFile("SYSCOLUMNS", &fh2);
	RM_FileScan fileScan;
	//tablename(21)	attrname(21) attrtype(int)	attrlength(int)	attroffset(int)	ix_flag(1)	indexname(21)
	Con condition;
	condition.bLhsIsAttr = 1; condition.bRhsIsAttr = 0;
	condition.attrType = chars;
	condition.LattrOffset = 43 + 3 * sizeof(int);
	condition.compOp = EQual;
	condition.Rvalue = indexName;
	RM_Record record;
	OpenScan(&fileScan, &fh2, 1, &condition);
	if (GetNextRec(&fileScan, &record) != SUCCESS)
	{
		RM_CloseFile(&fh2);
		CloseScan(&fileScan);
		return INDEX_NOT_EXIST;
	}
	CloseScan(&fileScan);
	*(record.pData + 42 + 3 * sizeof(int)) = '0';
	*(record.pData + 43 + 3 * sizeof(int)) = '\0';
	//////////////////////////////
	PF_PageHandle pageHandle;
	GetThisPage(fh2.pfileHandle, record.rid.pageNum, &pageHandle);
	MarkDirty(&pageHandle);
	UnpinPage(&pageHandle);
	DeleteFile(indexName);
	RM_CloseFile(&fh2);
	return SUCCESS;
}

RC Insert(char* relName, int nValues, Value* values)
{
	RM_FileHandle fh1, fh2;
	RM_OpenFile("SYSTABLES", &fh1);
	RM_OpenFile("SYSCOLUMNS", &fh2);
	RM_FileScan fileScan;
	Con condition;
	condition.bLhsIsAttr = 1; condition.bRhsIsAttr = 0;
	condition.attrType = chars;
	condition.LattrOffset = 0;
	condition.compOp = EQual;
	condition.Rvalue = relName;
	RM_Record record, insertRc;
	OpenScan(&fileScan, &fh1, 1, &condition);
	if (GetNextRec(&fileScan, &record) != SUCCESS)
	{
		RM_CloseFile(&fh1);
		RM_CloseFile(&fh2);
		CloseScan(&fileScan);
		return TABLE_NOT_EXIST;
	}
	if (*(int*)(record.pData + 21) > nValues)
	{
		RM_CloseFile(&fh1);
		RM_CloseFile(&fh2);
		CloseScan(&fileScan);
		return FIELD_MISSING;
	}
	if (*(int*)(record.pData + 21) < nValues)
	{
		RM_CloseFile(&fh1);
		RM_CloseFile(&fh2);
		CloseScan(&fileScan);
		return FIELD_REDUNDAN;

	}
	CloseScan(&fileScan);
	OpenScan(&fileScan, &fh2, 1, &condition);
	//顺序查找属性，直接用一个条件（表名）遍历
	//tablename(21)	attrname(21) attrtype(int)	attrlength(int)	attroffset(int)	ix_flag(1)	indexname(21)
	insertRc.pData = (char*)malloc(fh2.fileSubHeader->recordSize);
	if (insertRc.pData == NULL)
		return FAIL;
	int offset = 0;
	for (int i = nValues - 1; i >= 0; i--)
	{
		GetNextRec(&fileScan, &record);
		//按照查询到的属性列进行拼接成插入的记录
		if (*(AttrType*)(record.pData + 42) != (values + i)->type)
		{
			RM_CloseFile(&fh1);
			RM_CloseFile(&fh2);
			CloseScan(&fileScan);
			free(insertRc.pData);
			return FIELD_TYPE_MISMATCH;
		}
		else if ((values + i)->type == chars)
		{
			//超出范围的字符串直接截取
			memcpy(insertRc.pData + offset, (char*)((values + i)->data), *(int*)(record.pData + 42 + sizeof(int)));
			offset += *(int*)(record.pData + 42 + sizeof(int));
		}
		else if ((values + i)->type == ints)
		{
			*(int*)(insertRc.pData + offset) = *(int*)((values + i)->data);
			offset += sizeof(int);
		}
		else
		{
			*(float*)(insertRc.pData + offset) = *(float*)((values + i)->data);
			offset += sizeof(float);
		}
	}
	CloseScan(&fileScan);
	insertRc.bValid = true;
	RM_FileHandle fh;
	RM_OpenFile(relName, &fh);
	InsertRec(&fh, insertRc.pData, &insertRc.rid);//插入整条记录
	RM_CloseFile(&fh);
	//在遍历一遍SYSCOLUMNS表，向每个索引中添加该索引项
	OpenScan(&fileScan, &fh2, 1, &condition);
	for (int i = 0; i < nValues; i++)
	{
		GetNextRec(&fileScan, &record);
		if (*(record.pData + 42 + 3 * sizeof(int)) != '1')
			continue;
		IX_IndexHandle indexHandle;
		OpenIndex(record.pData + 43 + 3 * sizeof(int), &indexHandle);
		InsertEntry(&indexHandle, insertRc.pData + *(int*)(record.pData + 42 + 2 * sizeof(int)), &insertRc.rid);
		CloseIndex(&indexHandle);
	}
	CloseScan(&fileScan);
	RM_CloseFile(&fh1);
	RM_CloseFile(&fh2);
	free(insertRc.pData);
	//Tree tree;
	//if(GetIndexTree("stuSnoIndex", &tree)==SUCCESS)
	//	deleteTree(tree.root);
	return SUCCESS;
}
RC Delete(char* relName, int nConditions, Condition* conditions)
{
	RM_FileHandle fh1, fh2;
	RM_OpenFile("SYSTABLES", &fh1);
	RM_OpenFile("SYSCOLUMNS", &fh2);
	RM_FileScan fileScan;
	Con condition[2];//前面需要用到两个条件
	condition[0].bLhsIsAttr = 1; condition[0].bRhsIsAttr = 0;
	condition[1].bLhsIsAttr = 1; condition[1].bRhsIsAttr = 0;
	condition[0].attrType = condition[1].attrType = chars;//表名,属性名
	condition[0].LattrOffset = 0;
	condition[1].LattrOffset = 21;
	condition[0].compOp = condition[1].compOp = EQual;
	condition[0].Rvalue = relName;
	RM_Record record;
	OpenScan(&fileScan, &fh1, 1, condition);
	if (GetNextRec(&fileScan, &record) != SUCCESS)
	{
		RM_CloseFile(&fh1);
		RM_CloseFile(&fh2);
		CloseScan(&fileScan);
		return TABLE_NOT_EXIST;
	}
	CloseScan(&fileScan);
	//封装n个比较条件
	Con* cons = nullptr;
	if (nConditions != 0)
	{
		cons = (Con*)malloc(nConditions * sizeof(Con));
		if (cons == NULL)
			return FAIL;
	}
	for (int i = 0; i < nConditions; i++)
	{
		AttrType lattr, rattr;
		(cons + i)->compOp = (conditions + nConditions - 1 - i)->op;
		if ((conditions + nConditions - 1 - i)->bLhsIsAttr == 1)//左边是属性
		{
			condition[1].Rvalue = (conditions + nConditions - 1 - i)->lhsAttr.attrName;
			OpenScan(&fileScan, &fh2, 2, condition);
			if (GetNextRec(&fileScan, &record) != SUCCESS)//查找该属性记录
			{
				//tablename(21)	attrname(21) attrtype(int)	attrlength(int)	attroffset(int)	ix_flag(1)	indexname(21)
				RM_CloseFile(&fh1);
				RM_CloseFile(&fh2);
				CloseScan(&fileScan);
				return TABLE_COLUMN_ERROR;//没有该属性名
			}
			lattr = *(AttrType*)(record.pData + 42);
			(cons + i)->bLhsIsAttr = 1;
			(cons + i)->LattrLength = *(int*)(record.pData + 42 + sizeof(int));
			(cons + i)->LattrOffset = *(int*)(record.pData + 42 + 2 * sizeof(int));
			CloseScan(&fileScan);
		}
		else {//左边是值
			lattr = (conditions + nConditions - 1 - i)->lhsValue.type;
			(cons + i)->Lvalue = (conditions + nConditions - 1 - i)->lhsValue.data;
		}

		if ((conditions + nConditions - 1 - i)->bRhsIsAttr == 1)//右边是属性
		{
			condition[1].Rvalue = (conditions + nConditions - 1 - i)->lhsAttr.attrName;
			OpenScan(&fileScan, &fh2, 2, condition);
			if (GetNextRec(&fileScan, &record) != SUCCESS)//查找该属性记录
			{
				//tablename(21)	attrname(21) attrtype(int)	attrlength(int)	attroffset(int)	ix_flag(1)	indexname(21)
				RM_CloseFile(&fh1);
				RM_CloseFile(&fh2);
				CloseScan(&fileScan);
				free(cons);
				return TABLE_COLUMN_ERROR;
			}
			rattr = *(AttrType*)(record.pData + 42);
			(cons + i)->bRhsIsAttr = 1;
			(cons + i)->RattrLength = *(int*)(record.pData + 42 + sizeof(int));
			(cons + i)->RattrOffset = *(int*)(record.pData + 42 + 2 * sizeof(int));
			CloseScan(&fileScan);
		}
		else {
			rattr = (conditions + nConditions - 1 - i)->rhsValue.type;
			(cons + i)->Rvalue = (conditions + nConditions - 1 - i)->rhsValue.data;
		}
		if (lattr == rattr)
			(cons + i)->attrType = lattr;
		else {//左右属性不同
			//if (lattr == chars || rattr == chars)//只有一边是chars
			RM_CloseFile(&fh1);
			RM_CloseFile(&fh2);
			CloseScan(&fileScan);
			if (cons) free(cons);
			return TABLE_COLUMN_ERROR;
		}
	}
	RM_FileHandle fileHandle;
	RM_OpenFile(relName, &fileHandle);
	RM_Record data_record;
	data_record.bValid = false;
	/*data_record.pData = (char*)malloc(fileHandle.fileSubHeader->recordSize);*/
	OpenScan(&fileScan, &fileHandle, nConditions, cons);
	while (GetNextRec(&fileScan, &data_record) == SUCCESS)
	{
		DeleteRec(&fileHandle, &data_record.rid);
		RM_FileScan fileScan2;
		OpenScan(&fileScan2, &fh2, 1, condition);
		//tablename(21)	attrname(21) attrtype(int)	attrlength(int)	attroffset(int)	ix_flag(1)	indexname(21)
		while (GetNextRec(&fileScan2, &record) == SUCCESS)
		{
			if (*(record.pData + 42 + 3 * sizeof(int)) != '1')
				continue;
			IX_IndexHandle indexHandle;
			OpenIndex(record.pData + 43 + 3 * sizeof(int), &indexHandle);
			DeleteEntry(&indexHandle, data_record.pData + *(int*)(record.pData + 42 + 2 * sizeof(int)), &data_record.rid);
			CloseIndex(&indexHandle);
		}
		CloseScan(&fileScan2);
	}
	CloseScan(&fileScan);
	RM_CloseFile(&fileHandle);
	RM_CloseFile(&fh1);
	RM_CloseFile(&fh2);
	if (cons) free(cons);
	/*free(data_record.pData);*/
	///////////////
	/*Tree tree;
	if (GetIndexTree("stuSnoIndex", &tree) == SUCCESS)
		deleteTree(tree.root);*/
	return SUCCESS;
}

RC Update(char* relName, char* attrName, Value* value, int nConditions, Condition* conditions)
{
	RM_FileHandle fh1, fh2;
	RM_OpenFile("SYSTABLES", &fh1);
	RM_OpenFile("SYSCOLUMNS", &fh2);
	RM_FileScan fileScan;
	Con condition[2];//前面需要用到两个条件
	condition[0].bLhsIsAttr = 1; condition[0].bRhsIsAttr = 0;
	condition[1].bLhsIsAttr = 1; condition[1].bRhsIsAttr = 0;
	condition[0].attrType = condition[1].attrType = chars;//表名,属性名
	condition[0].LattrOffset = 0;
	condition[1].LattrOffset = 21;
	condition[0].compOp = condition[1].compOp = EQual;
	condition[0].Rvalue = relName;
	condition[1].Rvalue = attrName;
	RM_Record record;
	/*char data[64 + 3 * sizeof(int)] = { 0 };
	record.pData = data;*/
	OpenScan(&fileScan, &fh1, 1, condition);
	if (GetNextRec(&fileScan, &record) != SUCCESS)
	{
		RM_CloseFile(&fh1);
		RM_CloseFile(&fh2);
		CloseScan(&fileScan);
		return TABLE_NOT_EXIST;
	}
	CloseScan(&fileScan);
	//tablename(21)	attrname(21) attrtype(int)	attrlength(int)	attroffset(int)	ix_flag(1)	indexname(21)
	OpenScan(&fileScan, &fh2, 2, condition);
	if (GetNextRec(&fileScan, &record) != SUCCESS)
	{
		RM_CloseFile(&fh1);
		RM_CloseFile(&fh2);
		CloseScan(&fileScan);
		return TABLE_COLUMN_ERROR;
	}
	CloseScan(&fileScan);
	if (value->type != *(AttrType*)(record.pData + 42))
	{
		RM_CloseFile(&fh1);
		RM_CloseFile(&fh2);
		return FIELD_TYPE_MISMATCH;
	}
	//保存要更改的属性的信息
	AttrType type = *(AttrType*)(record.pData + 42);
	int length = *(int*)(record.pData + 42 + sizeof(int));
	int offset = *(int*)(record.pData + 42 + 2 * sizeof(int));
	char ix_flag = *(record.pData + 42 + 3 * sizeof(int));
	char indexName[21];
	if (ix_flag == '1')
		memcpy(indexName, record.pData + 43 + 3 * sizeof(int), 21);
	//封装n个比较条件
	Con* cons = nullptr;
	if (nConditions != 0)
	{
		cons = (Con*)malloc(nConditions * sizeof(Con));
		if (cons == NULL)
			return FAIL;
	}
	for (int i = 0; i < nConditions; i++)
	{
		AttrType lattr, rattr;
		(cons + i)->compOp = (conditions + nConditions - 1 - i)->op;
		if ((conditions + nConditions - 1 - i)->bLhsIsAttr == 1)//左边是属性
		{
			condition[1].Rvalue = (conditions + nConditions - 1 - i)->lhsAttr.attrName;
			OpenScan(&fileScan, &fh2, 2, condition);
			if (GetNextRec(&fileScan, &record) != SUCCESS)//查找该属性记录
			{
				//tablename(21)	attrname(21) attrtype(int)	attrlength(int)	attroffset(int)	ix_flag(1)	indexname(21)
				RM_CloseFile(&fh1);
				RM_CloseFile(&fh2);
				CloseScan(&fileScan);
				return FIElD_NOT_EXIST;//没有该属性名
			}
			lattr = *(AttrType*)(record.pData + 42);
			(cons + i)->bLhsIsAttr = 1;
			(cons + i)->LattrLength = *(int*)(record.pData + 42 + sizeof(int));
			(cons + i)->LattrOffset = *(int*)(record.pData + 42 + 2 * sizeof(int));
			CloseScan(&fileScan);
		}
		else {//左边是值
			lattr = (conditions + nConditions - 1 - i)->lhsValue.type;
			(cons + i)->bLhsIsAttr = 0;
			(cons + i)->Lvalue = (conditions + nConditions - 1 - i)->lhsValue.data;
		}

		if ((conditions + nConditions - 1 - i)->bRhsIsAttr == 1)//右边是属性
		{
			condition[1].Rvalue = (conditions + nConditions - 1 - i)->rhsAttr.attrName;
			OpenScan(&fileScan, &fh2, 2, condition);
			if (GetNextRec(&fileScan, &record) != SUCCESS)//查找该属性记录
			{
				//tablename(21)	attrname(21) attrtype(int)	attrlength(int)	attroffset(int)	ix_flag(1)	indexname(21)
				RM_CloseFile(&fh1);
				RM_CloseFile(&fh2);
				CloseScan(&fileScan);
				free(cons);
				return FIElD_NOT_EXIST;
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
		else {//左右属性不同
			//if (lattr == chars || rattr == chars)//只有一边是chars
			RM_CloseFile(&fh1);
			RM_CloseFile(&fh2);
			free(cons);
			return FIELD_TYPE_MISMATCH;
		}
	}

	RM_FileHandle fileHandle;
	RM_OpenFile(relName, &fileHandle);
	RM_Record data_record;
	data_record.bValid = false;
	//data_record.pData = (char*)malloc(fileHandle.fileSubHeader->recordSize);
	OpenScan(&fileScan, &fileHandle, nConditions, cons);
	while (GetNextRec(&fileScan, &data_record) == SUCCESS)
	{
		//字符串长的截取
		if (ix_flag != '1')
			memcpy(data_record.pData + offset, value->data, length);
		else
		{
			IX_IndexHandle indexHandle;
			OpenIndex(indexName, &indexHandle);
			DeleteEntry(&indexHandle, data_record.pData + offset, &data_record.rid);
			memcpy(data_record.pData + offset, value->data, length);
			InsertEntry(&indexHandle, data_record.pData + offset, &data_record.rid);
			CloseIndex(&indexHandle);
		}
		PF_PageHandle pageHandle;
		GetThisPage(fileHandle.pfileHandle, data_record.rid.pageNum, &pageHandle);
		MarkDirty(&pageHandle);
		UnpinPage(&pageHandle);
		//UpdateRec(&fileHandle, &data_record);//rid不用变
	}
	CloseScan(&fileScan);
	RM_CloseFile(&fileHandle);
	RM_CloseFile(&fh1);
	RM_CloseFile(&fh2);
	
	//free(data_record.pData);
	return SUCCESS;
}