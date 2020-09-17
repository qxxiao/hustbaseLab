#include "stdafx.h"
#include "PF_Manager.h"

BF_Manager bf_manager;

const RC AllocateBlock(Frame **buf);
const RC DisposeBlock(Frame *buf);
const RC ForceAllPages(PF_FileHandle *fileHandle);

void inti()		//��ʼ��������������
{
	int i;
	for(i=0;i<PF_BUFFER_SIZE;i++){
		bf_manager.allocated[i]=false;
		bf_manager.frame[i].pinCount=0;
	}
}

const RC CreateFile(const char *fileName)		//����һ��ҳ���ļ�
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
	bitmap=page.pData+(int)PF_FILESUBHDR_SIZE;		//λͼ����ʼλ��
	fileSubHeader=(PF_FileSubHeader *)page.pData;
	fileSubHeader->nAllocatedPages=1;				//�ѷ����ҳ��
	fileSubHeader->pageCount = 0;					//βҳ��ҳ��0
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

//��ȡһ���յ��ļ����������ռ�,δʹ��//
PF_FileHandle * getPF_FileHandle(void )		
{
	PF_FileHandle *p=(PF_FileHandle *)malloc(sizeof( PF_FileHandle));
	p->bopen=false;
	return p;
}                      

const RC openFile(char *fileName,PF_FileHandle *fileHandle)//��һ��ҳ���ļ��������ļ����
{
	int fd;
	PF_FileHandle *pfilehandle=fileHandle;
	RC tmp;
	if((fd=_open(fileName,O_RDWR|_O_BINARY))<0)
		return PF_FILEERR;
	pfilehandle->bopen=true;
	pfilehandle->fileName=fileName;
	pfilehandle->fileDesc=fd;
	if((tmp=AllocateBlock(&pfilehandle->pHdrFrame))!=SUCCESS){	//�ļ���ͷ֡Ҳ�洢��֡������
		_close(fd);
		return tmp;
	}
	pfilehandle->pHdrFrame->bDirty=false;
	pfilehandle->pHdrFrame->pinCount=1;
	pfilehandle->pHdrFrame->accTime=clock();
	pfilehandle->pHdrFrame->fileDesc=fd;
	pfilehandle->pHdrFrame->fileName=fileName;
	if(_lseek(fd,0,SEEK_SET)==-1){
		//��ӽ�������Ϊ0����Ȼ���ܶ���
		pfilehandle->pHdrFrame->pinCount = 0;
		DisposeBlock(pfilehandle->pHdrFrame);
		_close(fd);
		return PF_FILEERR;
	}
	if(_read(fd,&(pfilehandle->pHdrFrame->page),sizeof(Page))!=sizeof(Page)){ //��ȡӲ���ļ��ĵ�һҳ�����ͷ֡��
		//��ӽ�������Ϊ0����Ȼ���ܶ���
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

const RC CloseFile(PF_FileHandle *fileHandle)		//�ر�ҳ���ļ���ͬʱ����������ĸ��ļ���ҳ��֡
{
	RC tmp;
	fileHandle->pHdrFrame->pinCount--;
	if((tmp=ForceAllPages(fileHandle))!=SUCCESS)	//ǿ���ڻ������йر��˳���ҳ���ļ�������ҳ��֡
		return tmp;
	if(_close(fileHandle->fileDesc)<0)
		return PF_FILEERR;
	return SUCCESS;
}

const RC AllocateBlock(Frame **buffer)		//�ӻ����������з���һ��֡��ռ�õ�֡���ܱ���̭
{
	int i,min,offset;
	bool flag;
	clock_t mintime;
	for(i=0;i<PF_BUFFER_SIZE;i++)    //�����ڻ������ڿ��еģ����еĻ��߳�ȥռ�ú�LRU)һ֡�ռ䣬��*bufferָ��
		if(bf_manager.allocated[i]==false){
			bf_manager.allocated[i]=true;
			*buffer=bf_manager.frame+i;
			return SUCCESS;
		}
	flag=false;
	for(i=0;i<PF_BUFFER_SIZE;i++){
		if(bf_manager.frame[i].pinCount!=0)		//ռ�õ�ҳ���ܱ���̭
			continue;
		if(flag==false){
			flag=true;
			min=i;
			mintime=bf_manager.frame[i].accTime;
		}
		if(bf_manager.frame[i].accTime<mintime){//ѡȡ���δʹ�õ�ҳ������̭
			min=i;
			mintime=bf_manager.frame[i].accTime;
		}
	}
	if(flag==false)
		return PF_NOBUF;		                 //����������
	if(bf_manager.frame[min].bDirty==true){		 //���ļ�--�޸Ĺ���Ҫд���ļ�������ֱ�Ӷ���
		offset=(bf_manager.frame[min].page.pageNum)*sizeof(Page);
		if(_lseek(bf_manager.frame[min].fileDesc,offset,SEEK_SET)==offset-1)
			return PF_FILEERR;
		if(_write(bf_manager.frame[min].fileDesc,&(bf_manager.frame[min].page),sizeof(Page))!=sizeof(Page))
			return PF_FILEERR;
	}
	*buffer=bf_manager.frame+min;
	return SUCCESS;
}

//�����������еĸ�֡����֡��ռ�������ʧ�ܣ���ҳҪд��Ӳ�̣�ֻ��Openfileʹ�ù�//
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

//����һ���յ�ҳ������δ��ʹ��//
PF_PageHandle* getPF_PageHandle()			
{
	PF_PageHandle *p=(PF_PageHandle *)malloc(sizeof(PF_PageHandle));
	p->bOpen=false;
	return p;
}

const RC GetThisPage(PF_FileHandle *fileHandle,PageNum pageNum,PF_PageHandle *pageHandle)//��ȡ���ļ������ָ��ҳ��ҳ���ҳ����
{
	int i,nread,offset;
	RC tmp;
	PF_PageHandle *pPageHandle=pageHandle;
	if(pageNum>fileHandle->pFileSubHeader->pageCount)//������ҳ��
		return PF_INVALIDPAGENUM;
	if((fileHandle->pBitmap[pageNum/8]&(1<<(pageNum%8)))==0)//����ҳ���յģ�����ʧ��
		return PF_INVALIDPAGENUM;
	pPageHandle->bOpen=true;							    //��ҳ�����ѱ���---���ڷ��ʸ�ҳ��
	for(i=0;i<PF_BUFFER_SIZE;i++){
		if(bf_manager.allocated[i]==false)
			continue;
		if(strcmp(bf_manager.frame[i].fileName,fileHandle->fileName)!=0)//�ȱȽ��ļ����ڱȽ�ҳ���
			continue;
		if(bf_manager.frame[i].page.pageNum==pageNum){      //�������Ѿ����ڸ�ҳ��
			pPageHandle->pFrame=bf_manager.frame+i;
			pPageHandle->pFrame->pinCount++;				//��ȡ��ҳ��---ʹ�ü���+1
			pPageHandle->pFrame->accTime=clock();
			return SUCCESS;
		}
	}														//���ҵ�ҳ���ڻ������������µ�һҳ�����������ҷ���
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

//�����ļ������Ӧ��ҳ���ļ�����һ���µ�ҳ�棨���л�����������ռ䣩�������뻺����
const RC AllocatePage(PF_FileHandle *fileHandle,PF_PageHandle *pageHandle)	
{
	PF_PageHandle *pPageHandle=pageHandle;
	RC tmp;
	int i,byte,bit;
	fileHandle->pHdrFrame->bDirty=true;   //����ҳ����Ҫ�޸�ͷ֡��ͷҳ��---�򿪵��ļ���ͷ֡���ڻ�������
	if((fileHandle->pFileSubHeader->nAllocatedPages)<=(fileHandle->pFileSubHeader->pageCount)){//������δ����Ŀ���ҳ��δʹ�ã�
		for(i=0;i<=fileHandle->pFileSubHeader->pageCount;i++){
			byte=i/8;
			bit=i%8;
			if(((fileHandle->pBitmap[byte])&(1<<bit))==0){	//���ҵ�һ��δ���伴����ҳ
				(fileHandle->pFileSubHeader->nAllocatedPages)++;
				fileHandle->pBitmap[byte]|=(1<<bit);
				break;
			}
		}
		if(i<=fileHandle->pFileSubHeader->pageCount)
			return GetThisPage(fileHandle,i,pageHandle);//����λʾͼ�����ֱ�ӷ��ظ�ҳ���֡
		
	}
	//ҳ���ļ�û�п���ҳ����Ҫ������ҳ
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

//��ȡ�Ѿ��򿪵�ҳ���ҳ��
const RC GetPageNum(PF_PageHandle *pageHandle,PageNum *pageNum)
{
	if(pageHandle->bOpen==false)
		return PF_PHCLOSED;
	*pageNum=pageHandle->pFrame->page.pageNum;
	return SUCCESS;
}

const RC GetData(PF_PageHandle *pageHandle,char **pData) //��ȡ�Ѿ��򿪵�ҳ������������ָ��
{
	if(pageHandle->bOpen==false)
		return PF_PHCLOSED;
	*pData=pageHandle->pFrame->page.pData;
	return SUCCESS;
}

//�����ļ��б��ΪpageNum��ҳ�����ݣ������Ϊ����ҳ��δ����״̬��
//�ڶ�������ҳ��ʧ��
//���һ���̭�ڻ������еĸ�ҳ����ҳ��ռ���򲻻���̭��
const RC DisposePage(PF_FileHandle *fileHandle,PageNum pageNum)
{
	int i;
	char tmp;
	if(pageNum>fileHandle->pFileSubHeader->pageCount)
		return PF_INVALIDPAGENUM;
	if(((fileHandle->pBitmap[pageNum/8])&(1<<(pageNum%8)))==0)
		return PF_INVALIDPAGENUM;
	fileHandle->pHdrFrame->bDirty=true;    //�޸�ҳ���ļ���ͷ֡��λʾͼ
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

//���򿪵�ҳ�棨�ڻ���������֡���λ��ҳ---�޸Ĺ�������д��Ӳ��
const RC MarkDirty(PF_PageHandle *pageHandle)    
{
	pageHandle->pFrame->bDirty=true;
	return SUCCESS;
}

//����Ե�ǰҳ��ռ�ã�ռ�ü���-1������GetThisPage��AllocatePage�����û���Ϊ0��ҳ
const RC UnpinPage(PF_PageHandle *pageHandle)    
{
	pageHandle->pFrame->pinCount--;
	return SUCCESS;
}

//����˳��ڻ�������pageNumҳ���ҳ��ҳ��ռ����ʧ��PF_PAGEINNED--û��ʹ�ù�
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

//ǿ���˳���������������и�ҳ���ļ���ҳ��֡---û���ж�ҳ��ռ��
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