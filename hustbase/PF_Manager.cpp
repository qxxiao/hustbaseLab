#include "stdafx.h"
#include "PF_Manager.h"

BF_Manager bf_manager;

const RC AllocateBlock(Frame **buf);
const RC DisposeBlock(Frame *buf);
const RC ForceAllPages(PF_FileHandle *fileHandle);

void inti()		//初始化缓冲区的设置
{
	int i;
	for(i=0;i<PF_BUFFER_SIZE;i++){
		bf_manager.allocated[i]=false;
		bf_manager.frame[i].pinCount=0;
	}
}

const RC CreateFile(const char *fileName)		//创建一个页面文件
{
	int fd;
	char *bitmap;
	PF_FileSubHeader *fileSubHeader;
	fd=_open(fileName,_O_RDWR|_O_CREAT|_O_EXCL|_O_BINARY,_S_IREAD|_S_IWRITE);
	if(fd<0)
		return PF_EXIST;
	_close(fd);
	fd=open(fileName,_O_RDWR);
	Page page;
	memset(&page,0,PF_PAGESIZE);
	bitmap=page.pData+(int)PF_FILESUBHDR_SIZE;		//位图的起始位置
	fileSubHeader=(PF_FileSubHeader *)page.pData;
	fileSubHeader->nAllocatedPages=1;				//已分配的页面
	fileSubHeader->pageCount = 0;					//尾页的页号0
	bitmap[0]|=0x01;
	if(_lseek(fd,0,SEEK_SET)==-1)
		return PF_FILEERR;
	if(_write(fd,(char *)&page,sizeof(Page))!=sizeof(Page)){
		_close(fd);
		return PF_FILEERR;
	}
	if(_close(fd)<0)
		return PF_FILEERR;
	return SUCCESS;
}

//获取一个空的文件句柄，申请空间,未使用//
PF_FileHandle * getPF_FileHandle(void )		
{
	PF_FileHandle *p=(PF_FileHandle *)malloc(sizeof( PF_FileHandle));
	p->bopen=false;
	return p;
}                      

const RC openFile(char *fileName,PF_FileHandle *fileHandle)//打开一个页面文件，返回文件句柄
{
	int fd;
	PF_FileHandle *pfilehandle=fileHandle;
	RC tmp;
	if((fd=_open(fileName,O_RDWR|_O_BINARY))<0)
		return PF_FILEERR;
	pfilehandle->bopen=true;
	pfilehandle->fileName=fileName;
	pfilehandle->fileDesc=fd;
	if((tmp=AllocateBlock(&pfilehandle->pHdrFrame))!=SUCCESS){	//文件的头帧也存储在帧缓冲区
		_close(fd);
		return tmp;
	}
	pfilehandle->pHdrFrame->bDirty=false;
	pfilehandle->pHdrFrame->pinCount=1;
	pfilehandle->pHdrFrame->accTime=clock();
	pfilehandle->pHdrFrame->fileDesc=fd;
	pfilehandle->pHdrFrame->fileName=fileName;
	if(_lseek(fd,0,SEEK_SET)==-1){
		//添加将计数置为0，不然不能丢弃
		pfilehandle->pHdrFrame->pinCount = 0;
		DisposeBlock(pfilehandle->pHdrFrame);
		_close(fd);
		return PF_FILEERR;
	}
	if(_read(fd,&(pfilehandle->pHdrFrame->page),sizeof(Page))!=sizeof(Page)){ //读取硬盘文件的第一页放入该头帧中
		//添加将计数置为0，不然不能丢弃
		pfilehandle->pHdrFrame->pinCount = 0;
		DisposeBlock(pfilehandle->pHdrFrame);
		_close(fd);
		return PF_FILEERR;
	}
	pfilehandle->pHdrPage=&(pfilehandle->pHdrFrame->page);
	pfilehandle->pBitmap=pfilehandle->pHdrPage->pData+PF_FILESUBHDR_SIZE;
	pfilehandle->pFileSubHeader=(PF_FileSubHeader *)pfilehandle->pHdrPage->pData;
	fileHandle=pfilehandle;
	return SUCCESS;
}

const RC CloseFile(PF_FileHandle *fileHandle)		//关闭页面文件，同时清除缓冲区的该文件的页面帧
{
	RC tmp;
	fileHandle->pHdrFrame->pinCount--;
	if((tmp=ForceAllPages(fileHandle))!=SUCCESS)	//强制在缓冲区中关闭退出该页面文件的所有页面帧
		return tmp;
	if(_close(fileHandle->fileDesc)<0)
		return PF_FILEERR;
	return SUCCESS;
}

const RC AllocateBlock(Frame **buffer)		//从缓冲区管理中分配一个帧，占用的帧不能被淘汰
{
	int i,min,offset;
	bool flag;
	clock_t mintime;
	for(i=0;i<PF_BUFFER_SIZE;i++)    //返回在缓冲区内空闲的（空闲的或者除去占用后LRU)一帧空间，用*buffer指向
		if(bf_manager.allocated[i]==false){
			bf_manager.allocated[i]=true;
			*buffer=bf_manager.frame+i;
			return SUCCESS;
		}
	flag=false;
	for(i=0;i<PF_BUFFER_SIZE;i++){
		if(bf_manager.frame[i].pinCount!=0)		//占用的页不能被淘汰
			continue;
		if(flag==false){
			flag=true;
			min=i;
			mintime=bf_manager.frame[i].accTime;
		}
		if(bf_manager.frame[i].accTime<mintime){//选取最久未使用的页进行淘汰
			min=i;
			mintime=bf_manager.frame[i].accTime;
		}
	}
	if(flag==false)
		return PF_NOBUF;		                 //缓冲区不足
	if(bf_manager.frame[min].bDirty==true){		 //脏文件--修改过需要写进文件，否则直接丢弃
		offset=(bf_manager.frame[min].page.pageNum)*sizeof(Page);
		if(_lseek(bf_manager.frame[min].fileDesc,offset,SEEK_SET)==offset-1)
			return PF_FILEERR;
		if(_write(bf_manager.frame[min].fileDesc,&(bf_manager.frame[min].page),sizeof(Page))!=sizeof(Page))
			return PF_FILEERR;
	}
	*buffer=bf_manager.frame+min;
	return SUCCESS;
}

//丢弃缓冲区中的该帧，该帧被占用则操作失败；脏页要写回硬盘，只供Openfile使用过//
const RC DisposeBlock(Frame *buf)		
{
	if(buf->pinCount!=0)
		return PF_PAGEPINNED;
	if(buf->bDirty==true){
		if(_lseek(buf->fileDesc,buf->page.pageNum*sizeof(Page),SEEK_SET)<0)
			return PF_FILEERR;
		if(_write(buf->fileDesc,&(buf->page),sizeof(Page))!=sizeof(Page))
			return PF_FILEERR;
	}
	buf->bDirty=false;
	bf_manager.allocated[buf-bf_manager.frame]=false;
	return SUCCESS;
}

//返回一个空的页面句柄，未被使用//
PF_PageHandle* getPF_PageHandle()			
{
	PF_PageHandle *p=(PF_PageHandle *)malloc(sizeof(PF_PageHandle));
	p->bOpen=false;
	return p;
}

const RC GetThisPage(PF_FileHandle *fileHandle,PageNum pageNum,PF_PageHandle *pageHandle)//获取该文件句柄下指定页号页面的页面句柄
{
	int i,nread,offset;
	RC tmp;
	PF_PageHandle *pPageHandle=pageHandle;
	if(pageNum>fileHandle->pFileSubHeader->pageCount)//大于总页数
		return PF_INVALIDPAGENUM;
	if((fileHandle->pBitmap[pageNum/8]&(1<<(pageNum%8)))==0)//空闲页（空的）返回失败
		return PF_INVALIDPAGENUM;
	pPageHandle->bOpen=true;							    //该页面句柄已被打开---正在访问该页面
	for(i=0;i<PF_BUFFER_SIZE;i++){
		if(bf_manager.allocated[i]==false)
			continue;
		if(strcmp(bf_manager.frame[i].fileName,fileHandle->fileName)!=0)//先比较文件名在比较页面号
			continue;
		if(bf_manager.frame[i].page.pageNum==pageNum){      //缓冲区已经存在该页面
			pPageHandle->pFrame=bf_manager.frame+i;
			pPageHandle->pFrame->pinCount++;				//获取该页面---使用计数+1
			pPageHandle->pFrame->accTime=clock();
			return SUCCESS;
		}
	}														//查找的页不在缓冲区，读入新的一页到缓冲区并且返回
	if((tmp=AllocateBlock(&(pPageHandle->pFrame)))!=SUCCESS){
		return tmp;
	}
	pPageHandle->pFrame->bDirty=false;
	pPageHandle->pFrame->fileDesc=fileHandle->fileDesc;
	pPageHandle->pFrame->fileName=fileHandle->fileName;
	pPageHandle->pFrame->pinCount=1;
	pPageHandle->pFrame->accTime=clock();
	offset=pageNum*sizeof(Page);
	if(_lseek(fileHandle->fileDesc,offset,SEEK_SET)==offset-1){
		bf_manager.allocated[pPageHandle->pFrame-bf_manager.frame]=false;
		return PF_FILEERR;
	}
	if((nread=read(fileHandle->fileDesc,&(pPageHandle->pFrame->page),sizeof(Page)))!=sizeof(Page)){
		bf_manager.allocated[pPageHandle->pFrame-bf_manager.frame]=false;
		return PF_FILEERR;
	}
	return SUCCESS;
}

//给该文件句柄对应的页面文件申请一个新的页面（空闲或者重新申请空间），会载入缓冲区
const RC AllocatePage(PF_FileHandle *fileHandle,PF_PageHandle *pageHandle)	
{
	PF_PageHandle *pPageHandle=pageHandle;
	RC tmp;
	int i,byte,bit;
	fileHandle->pHdrFrame->bDirty=true;   //分配页面需要修改头帧（头页）---打开的文件的头帧会在缓冲区中
	if((fileHandle->pFileSubHeader->nAllocatedPages)<=(fileHandle->pFileSubHeader->pageCount)){//即还有未分配的空闲页（未使用）
		for(i=0;i<=fileHandle->pFileSubHeader->pageCount;i++){
			byte=i/8;
			bit=i%8;
			if(((fileHandle->pBitmap[byte])&(1<<bit))==0){	//查找第一个未分配即空闲页
				(fileHandle->pFileSubHeader->nAllocatedPages)++;
				fileHandle->pBitmap[byte]|=(1<<bit);
				break;
			}
		}
		if(i<=fileHandle->pFileSubHeader->pageCount)
			return GetThisPage(fileHandle,i,pageHandle);//更改位示图后可以直接返回该页面的帧
		
	}
	//页面文件没有空闲页，需要分配新页
	fileHandle->pFileSubHeader->nAllocatedPages++;
	fileHandle->pFileSubHeader->pageCount++;
	byte=fileHandle->pFileSubHeader->pageCount/8;
	bit=fileHandle->pFileSubHeader->pageCount%8;
	fileHandle->pBitmap[byte]|=(1<<bit);
	if((tmp=AllocateBlock(&(pPageHandle->pFrame)))!=SUCCESS){
		return tmp;
	}
	pPageHandle->pFrame->bDirty=false;
	pPageHandle->pFrame->fileDesc=fileHandle->fileDesc;
	pPageHandle->pFrame->fileName=fileHandle->fileName;
	pPageHandle->pFrame->pinCount=1;
	pPageHandle->pFrame->accTime=clock();
	memset(&(pPageHandle->pFrame->page),0,sizeof(Page));
	pPageHandle->pFrame->page.pageNum=fileHandle->pFileSubHeader->pageCount;
	if(_lseek(fileHandle->fileDesc,0,SEEK_END)==-1){
		bf_manager.allocated[pPageHandle->pFrame-bf_manager.frame]=false;
		return PF_FILEERR;
	}
	if(_write(fileHandle->fileDesc,&(pPageHandle->pFrame->page),sizeof(Page))!=sizeof(Page)){
		bf_manager.allocated[pPageHandle->pFrame-bf_manager.frame]=false;
		return PF_FILEERR;
	}
	
	return SUCCESS;
}

//获取已经打开的页面的页号
const RC GetPageNum(PF_PageHandle *pageHandle,PageNum *pageNum)
{
	if(pageHandle->bOpen==false)
		return PF_PHCLOSED;
	*pageNum=pageHandle->pFrame->page.pageNum;
	return SUCCESS;
}

const RC GetData(PF_PageHandle *pageHandle,char **pData) //获取已经打开的页面的数据区域的指针
{
	if(pageHandle->bOpen==false)
		return PF_PHCLOSED;
	*pData=pageHandle->pFrame->page.pData;
	return SUCCESS;
}

//丢弃文件中编号为pageNum的页面内容，将其变为空闲页（未分配状态）
//在丢弃空闲页会失败
//并且会淘汰在缓冲区中的该页（该页面占用则不会淘汰）
const RC DisposePage(PF_FileHandle *fileHandle,PageNum pageNum)
{
	int i;
	char tmp;
	if(pageNum>fileHandle->pFileSubHeader->pageCount)
		return PF_INVALIDPAGENUM;
	if(((fileHandle->pBitmap[pageNum/8])&(1<<(pageNum%8)))==0)
		return PF_INVALIDPAGENUM;
	fileHandle->pHdrFrame->bDirty=true;    //修改页面文件的头帧的位示图
	fileHandle->pFileSubHeader->nAllocatedPages--;
	tmp=1<<(pageNum%8);
	fileHandle->pBitmap[pageNum/8]&=~tmp;
	for(i=0;i<PF_BUFFER_SIZE;i++){
		if(bf_manager.allocated[i]==false)
			continue;
		if(strcmp(bf_manager.frame[i].fileName,fileHandle->fileName)!=0)
			continue;
		if(bf_manager.frame[i].page.pageNum==pageNum){
			if(bf_manager.frame[i].pinCount!=0)
				return PF_PAGEPINNED;
			bf_manager.allocated[i]=false;
			return SUCCESS;
		}
	}
	return SUCCESS;
}

//将打开的页面（在缓冲区）的帧标记位脏页---修改过，方便写入硬盘
const RC MarkDirty(PF_PageHandle *pageHandle)    
{
	pageHandle->pFrame->bDirty=true;
	return SUCCESS;
}

//解除对当前页的占用，占用计数-1用于在GetThisPage或AllocatePage可以置换出为0的页
const RC UnpinPage(PF_PageHandle *pageHandle)    
{
	pageHandle->pFrame->pinCount--;
	return SUCCESS;
}

//清除退出在缓冲区的pageNum页码的页，页被占用则失败PF_PAGEINNED--没有使用过
const RC ForcePage(PF_FileHandle *fileHandle,PageNum pageNum)
{
	int i;
	for(i=0;i<PF_BUFFER_SIZE;i++){
		if(bf_manager.allocated[i]==false)
			continue;
		if(strcmp(bf_manager.frame[i].fileName,fileHandle->fileName)!=0)
			continue;
		if(bf_manager.frame[i].page.pageNum==pageNum){
			if(bf_manager.frame[i].pinCount!=0)
				return PF_PAGEPINNED;
			if(bf_manager.frame[i].bDirty==true){
				if(_lseek(fileHandle->fileDesc,pageNum*PF_PAGESIZE,SEEK_SET)<0)
					return PF_FILEERR;
				if(_write(fileHandle->fileDesc,&(bf_manager.frame[i].page),PF_PAGESIZE)<0)
					return PF_FILEERR;
			}
			bf_manager.allocated[i]=false;
		}
	}
	return SUCCESS;
}

//强制退出并清除缓冲区所有该页面文件的页面帧---没有判断页面占用
const RC ForceAllPages(PF_FileHandle *fileHandle) 
{
	int i,offset;
	for(i=0;i<PF_BUFFER_SIZE;i++){
		if(bf_manager.allocated[i]==false)
			continue;
		if(strcmp(bf_manager.frame[i].fileName,fileHandle->fileName)!=0)
			continue;

		if(bf_manager.frame[i].bDirty==true){
			offset=(bf_manager.frame[i].page.pageNum)*sizeof(Page);
			if(_lseek(fileHandle->fileDesc,offset,SEEK_SET)==offset-1)
				return PF_FILEERR;
			if(_write(fileHandle->fileDesc,&(bf_manager.frame[i].page),sizeof(Page))!=sizeof(Page))
				return PF_FILEERR;
		}
		bf_manager.allocated[i]=false;
	}
	return SUCCESS;
}