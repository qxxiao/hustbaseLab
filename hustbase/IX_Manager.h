#ifndef IX_MANAGER_H_H
#define IX_MANAGER_H_H

#include "RM_Manager.h"
#include "PF_Manager.h"

/*索引文件的索引控制信息结构*/
typedef struct{
	int attrLength;
	int keyLength;
	AttrType attrType;
	PageNum rootPage;
	PageNum first_leaf;
	int order;
}IX_FileHeader;

//索引文件的句柄
typedef struct{
	bool bOpen;
	PF_FileHandle fileHandle;
	IX_FileHeader fileHeader;
}IX_IndexHandle;

/*当前节点或页面所存储节点的节点控制信息结构*/
typedef struct{
	int is_leaf;	//该结点是否是叶子节点
	int keynum;	    //该节点实际包含的关键字个数
	PageNum parent; //指向父节点所在的页面号
	PageNum brother;//指向右兄弟节点所在的页面号
	char *keys;		//指向关键字区的指针
	RID *rids;	    //指向指针区的指针
}IX_Node;

typedef struct{
	bool bOpen;		/*扫描是否打开 */
	IX_IndexHandle *pIXIndexHandle;	//指向索引文件操作的指针
	CompOp compOp;  /* 用于比较的操作符*/
	char *value;		 /* 与属性行比较的值 */
    //PF_PageHandle pfPageHandles[PF_BUFFER_SIZE]; // 固定在缓冲区页面所对应的页面操作列表
	//PageNum pnNext; 	//下一个将要被读入的页面号
	PF_PageHandle Pagehandle; //处理中的页面句柄
	PageNum pn;			//扫描即将处理的页面号
	int ridIx;			//扫描即将处理的索引项编号
}IX_IndexScan;

typedef struct Tree_Node{
	int  keyNum;		//节点中包含的关键字（属性值）个数
	char  **keys;		//节点中包含的关键字（属性值）数组
	Tree_Node  *parent;	//父节点
	Tree_Node  *sibling;	//右边的兄弟节点
	Tree_Node  *firstChild;	//最左边的孩子节点
}Tree_Node; //节点数据结构

typedef struct{
	AttrType  attrType;	//B+树对应属性的数据类型
	int  attrLength;	//B+树对应属性值的长度
	int  order;			//B+树的序数
	Tree_Node  *root;	//B+树的根节点
}Tree;

RC CreateIndex(const char * fileName,AttrType attrType,int attrLength);
RC OpenIndex(const char *fileName,IX_IndexHandle *indexHandle);
RC CloseIndex(IX_IndexHandle *indexHandle);

RC InsertEntry(IX_IndexHandle *indexHandle,void *pData,const RID * rid);
RC DeleteEntry(IX_IndexHandle *indexHandle,void *pData,const RID * rid);
RC OpenIndexScan(IX_IndexScan *indexScan,IX_IndexHandle *indexHandle,CompOp compOp,char *value);
RC IX_GetNextEntry(IX_IndexScan *indexScan,RID * rid);
RC CloseIndexScan(IX_IndexScan *indexScan);
RC GetIndexTree(char *fileName, Tree *index);

void deleteTree(Tree_Node* root);
#endif