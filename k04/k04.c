#include<linux/module.h>
#include<linux/init.h>
#include<linux/kernel.h>
#include<linux/slab.h>
#include<linux/vmalloc.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<asm/uaccess.h>
#include<linux/types.h>
#include<linux/moduleparam.h>
#include<linux/pci.h>
#include<asm/unistd.h>
#include<linux/device.h>
#include<linux/sched.h>
#include<linux/pid.h>
#include<linux/mm_types.h>
//get_user_pages
#include<linux/mm.h>
#include<linux/security.h>
#include<linux/pagemap.h>
//kmap,kunmap
#include<linux/highmem.h>
//sleep_on_timeout
#include<linux/wait.h>

//2013-12-31修改了地址记录的结构，因为一页内的偏移不超过4100,所以没必要使用long表示，在压缩下段和页索引，使用4字节表示一个地址记录：
//前两字节表示段序号和页序号，段只搜索代码段和数据段，所以只用一位表示即可，剩下7位表示页索引，每个段的页索引规定最大：30000,也就是
//每个段搜索的最大地址为：120M，全部搜索为240M。应对一般的搜索足够了，如果需要更大的搜索空间可重新定义下列结构：
struct SEG_DESC
{
	unsigned short pbit:15;			//页占15位
	unsigned short sbit:1;			//段占1位，=0表示代码段，=1数据段。
};
struct KVAR_SAD
{
	union{
		struct SEG_DESC s_b;
		unsigned short  spg;
	};
	union{
		unsigned short off;
		unsigned char  ofch[2];
	};
};//现在的结构一次可传送2000地址。
struct KVAR_ADDR
{
	unsigned int maxd;			//上限
	unsigned int mind;			//下限
	struct KVAR_SAD ksa;		//地址结构
	unsigned char vv0[4];		//无用，使结构为16字节
};

struct KVAR_AM
{
	unsigned char sync;					//命令字段中0字节的同步标志，
	unsigned char cmd;					//命令字段中1字节的主命令标志
	union{
	int   pid;							//命令字段中的2-5字节，传入的pid
	unsigned char  pch[4];			
	};
	unsigned char proc;					//命令字段中的6字节，存放进度值1-100
	unsigned char end0;					//命令字段中的7字节，存放本次查询的结束标志，=1结束
	char vv0[2];						//命令字段中的8,9字节，暂时未用。再次查询时在副本中用到8字节=3表示有效。
	union{
	unsigned long snum;					//命令字段中的10字节起的查询关键字。
	unsigned char sch[4];
	};
	char oaddr[4];						//命令字段中的14字节起的导出地址。
	char vv1[22];						//18-47字节暂时未,可以考虑传送代码段和数据段的段地址。
	union{
	unsigned int data_seg;
	unsigned char dsg[4];
	};
	union{
	unsigned int text_seg;
	unsigned char tsg[4];
	};
	struct KVAR_ADDR vadd[8];			//8个锁定（修改）数据和地址。 
};
struct KVAR_TY
{
	int		thread_lock;			//进程运行锁标志。
	unsigned char flag;//=0只允许传送pid,=1只允许首次查询和传送pid。=2全部操作允许。传送pid时该字节置1,首次查询时该字节置2。
	int		 pid;
	union{//带查询的数字
		unsigned char  dest[4];
		int	dnum;
	};
//下面两项保存段起始地址和页序号。
	unsigned long  seg[2];//0:代码段，1：数据段
	unsigned short pin;//-2013-12-31改为缓冲区设置标志 暂时保存，准备往buffer中写入		=3时输入输出缓冲区为tp,!=3时缓冲区的mp
};

MODULE_LICENSE("GPL");
MODULE_AUTHOR("tybitsfox ([email]tybitsfox@126.com[/email])");
MODULE_DESCRIPTION("kernel memory access module.");
//
#define K_BUFFER_SIZE		8192
#define DRV_MAJOR			246
#define DRV_MINOR			0
#define drv_name			"memacc_8964_dev"
#define d_begin				192
#define d_len				8000


//{{{ 
/*定义内核与用户的互交:
  1、不再设置共享内存等，统一使用内核申请的缓冲区同步两者的操作。
  2、缓冲区前192字节始终不会包含数据信息，该区域用于传送命令数据。
  3、后8000字节用于传送数据信息，数据信息的结构为8字节一个单元，每单元
  包含段索引（0字节）内存页索引（2、3字节）和页内偏移（4-7字节）。每次
  传送的数据量为1000个地址。
  4、命令的定义：
  （1）第0字节为用户和内核操作的同步标志。用户界面操作完成后将该字节
  置0,内核操作完成后将该字节置1。因此，当用户查看内核操作是否完成，可
  轮寻该字节是否为1,不为1则进入等待。内核同理，实现两者的操作同步。
  （2）第1字节为主命令字节，
  =0为初次同步，传输待查询进程的pid。第2、3、4、5字节存储pid值。
  =1表示首次查询，此时第10字节开始的连续4字节为待查数据。
  =2表示再次查询，第10字节开始连续4字节为待查数据。并且，数据区域内传入
  前次查询匹配的地址。
  =3表示向用户传送一页的目标内存数据，第15字节开始传入8字节地址，返回该
  地址所在页的整页数据。
  =4表示修改操作，修改操作默认一次最多修改8个数据。传入格式为：上限（4字节）
  下限（4字节）8字节地址格式，一个记录16字节，其实位置：0x30
  =5表示锁定操作，该操作与修改操作基本一致，但是该操作连续监视内存，持续修改。
  （3） 第6字节存储查询进度字，有效值1-100。该字节在进行查询时由内核按
  查询进度写入。并且用户端可不受同步标志字段的限制读取该值进行显示。
  （4） 第7字节存储命令完成标志，该标志对查询命令有效，当首次查询超过1000
  地址时，由于缓冲区的限制要采取分次传送。每传送一次字节0设置并开始传送，
  查询完成后字节0设置并且该字节也设置，表示整个查询结束。同时该字节在首次查
  询时是由内核置位，用户端判断，再次查询时是由客户端置位，内核判断（需要传入
  地址）。
  （5）第30字节开始存储传出的代码段和数据段的地址。
 
 *///}}}
char *mp,*tp;
struct class *mem_class;
unsigned char suc=0;
struct KVAR_TY kv1;
struct KVAR_AM k_am;
//和命令头对应的结构

static int __init areg_init(void);
static void __exit areg_exit(void);
static int mem_open(struct inode *ind,struct file *filp);
static int mem_release(struct inode *ind,struct file *filp);
static ssize_t mem_read(struct file *filp,char __user *buf,size_t size,loff_t *fpos);
static ssize_t mem_write(struct file *filp,const char __user *buf,size_t size,loff_t *fpos);
static void class_create_release(struct class *cls);
//首次查询的线程函数
int first_srh(void *argc);
int next_srh(void *argc);//再次查找的线程函数
int update_srh(void *argc);//修改的线程函数，先用于测试。
int lock_srh(void *argc);//修改的线程函数。
//一个统一的同步设置及阻塞函数。
void s_sync(int t);
//等待函数。
void s_wait_mm(int w);
//


struct file_operations mem_fops=
{
	.owner=THIS_MODULE,
	.open=mem_open,
	.release=mem_release,
	.read=mem_read,
	.write=mem_write,
};
//{{{ int __init areg_init(void)
int __init areg_init(void)
{
	int res,retval;
	int devno;
	mem_class=NULL;
	devno=MKDEV(DRV_MAJOR,DRV_MINOR);
	mp=(char*)kmalloc(K_BUFFER_SIZE,GFP_KERNEL);
	if(mp==NULL)
		printk("<1>kmalloc error!\n");
	memset(mp,0,K_BUFFER_SIZE);
	mp[0]=1;//用户进程可以发送请求或命令了
	tp=(char*)vmalloc(K_BUFFER_SIZE);
	if(tp==NULL)
		printk("<1>vmalloc error!\n");
	memset(tp,0,K_BUFFER_SIZE);
	res=register_chrdev(DRV_MAJOR,drv_name,&mem_fops);//注册字符设备
	if(res)
	{
		printk("<1>register char dev error!\n");
		goto reg_err01;
	}
	mem_class=kzalloc(sizeof(*mem_class),GFP_KERNEL);//实体话设备类
	if(IS_ERR(mem_class))
	{
		kfree(mem_class);
		mem_class=NULL;
		printk("<1>kzalloc error!\n");
		goto reg_err02;
	}
	mem_class->name=drv_name;
	mem_class->owner=THIS_MODULE;
	mem_class->class_release=class_create_release; //初始化设备类释放处理函数。
	retval=class_register(mem_class);		//注册设备类
	if(retval)
	{
		kfree(mem_class);
		printk("<1>class_register error!\n");
		goto reg_err02;
	}
	device_create(mem_class,NULL,devno,NULL,drv_name);//注册设备文件系统，并建立节点。
	printk("<1>device create success!!!\n");
	kv1.pin=1;
	return 0;
reg_err02:
	unregister_chrdev(DRV_MAJOR,drv_name);//删除字符设备
reg_err01:
	kfree(mp);
	vfree(tp);
	suc=1;
	return -1;
}//}}}
//{{{ void __exit areg_exit(void)
void __exit areg_exit(void)
{
	if(suc!=0)
		return;
	unregister_chrdev(DRV_MAJOR,drv_name);
	device_destroy(mem_class,MKDEV(DRV_MAJOR,DRV_MINOR));
	if((mem_class!=NULL) && (!IS_ERR(mem_class)))
		class_unregister(mem_class);
	if(mp!=NULL)
		kfree(mp);
	if(tp!=NULL)
		vfree(tp);
	printk("<1>module exit ok!\n");
}//}}}
//{{{ int mem_open(struct inode *ind,struct file *filp)
int mem_open(struct inode *ind,struct file *filp)
{
	try_module_get(THIS_MODULE);	//引用计数增加
	printk("<1>device open success!\n");
	return 0;
}//}}}
//{{{ int mem_release(struct inode *ind,struct file *filp)
int mem_release(struct inode *ind,struct file *filp)
{
	module_put(THIS_MODULE);	//计数器减1
	printk("<1>device release success!\n");
	return 0;
}//}}}
//{{{ ssize_t mem_read(struct file *filp,char *buf,size_t size,loff_t *fpos)
ssize_t mem_read(struct file *filp,char *buf,size_t size,loff_t *fpos)
{
	int res;
	char *tmp;
	if(mp==NULL || tp==NULL)
	{
		printk("<1>kernel buffer error!\n");
		return 0;
	}
	switch(kv1.pin)
	{
	case 0://轮寻时的缓冲
		return 0;
	case 1://首次查询(命令和结果在mp中)和再次查询用户向内核传送地址时(此时的命令字节都在mp中)
		tmp=mp;
		if(size>K_BUFFER_SIZE)
			size=K_BUFFER_SIZE;
		break;
	case 2://再次查询时输出结果
		tmp=tp;
		if(size>K_BUFFER_SIZE)
			size=K_BUFFER_SIZE;
		break;
	default://其他时:首次查询输出结果，再次查询时请求地址。
		return 0;
	};
	res=copy_to_user(buf,tmp,size);
	if(res==0)
	{
//		printk("<1>copy data from k->u success!\n");
		return size;
	}
//	printk("<1>copy date from k->u error!\n");
	return 0;
}//}}}
//{{{ ssize_t mem_write(struct file *filp,char *buf,size_t size,loff_t *fpos)
ssize_t mem_write(struct file *filp,const char *buf,size_t size,loff_t *fpos)
{
	unsigned long i,j;
	int res;
	char *tmp;
	if(mp==NULL ||tp==NULL)
	{
		printk("<1>kernel buffer error!\n");
		return 0;
	}
	switch(kv1.pin)
	{
	case 0://待命状态时默认不读取(不接受用户任何指令)，直接返回
		return 0;
	case 1://使用mp,此时等待接收首次查询时用户的反馈信息，和再次查询时取得地址。
		tmp=mp;
		if(size>K_BUFFER_SIZE)
			size=K_BUFFER_SIZE;
		break;
	case 2://再次查询时传送出查询结果后等待用户的相应信息
		tmp=tp;
		if(size>K_BUFFER_SIZE)
			size=K_BUFFER_SIZE;
		break;
	default:
		return 0;
	};
	res=copy_from_user(tmp,buf,size);
	if(res==0)
	{
		if(tmp[0]!=0 || tmp[7]==1)
			return size;
//		printk("<1>copy data from u->k success!\n");
		if(kv1.thread_lock==0)//命令分析，仅在没有线程运行时才有效。
		{//需要启动内核线程,根据tmp[0]确定启动哪个线程：
			memset((void*)&k_am,0,sizeof(k_am));
			memcpy((void*)&k_am,tmp,sizeof(k_am));//拷贝至结构，平时的操作只在结构中进行。
//为了保险，一些标志再次强制设置：
			k_am.sync=0;k_am.end0=0;
			kv1.pid=k_am.pid;kv1.dnum=k_am.snum;
			if(tmp[1]==1)//首次查找。
			{
				tmp[0]=0;tmp[7]=0;tmp[8]=0;
				kv1.thread_lock=1;s_sync(0);
				kernel_thread(first_srh,NULL,CLONE_KERNEL);//启动首次查找线程.
				return size;
			}
			if(tmp[1]==2)//再次查询
			{//确定采用一个线程完成全部再次查询的操作。因此下面的一步是必须的：
				tmp[0]=0;tmp[7]=0;tmp[8]=0;
				kv1.pin=0;kv1.thread_lock=1;
				kernel_thread(next_srh,NULL,CLONE_KERNEL);//启动再次查询线程
				return size;
			}
			if(tmp[1]==3)//锁定
			{
				j=0;
				if(k_am.vadd[0].ksa.s_b.sbit==0)
					j=kv1.seg[0];
				else
					j=kv1.seg[1];
				i=k_am.vadd[0].ksa.s_b.pbit*4096;
				i+=k_am.vadd[0].ksa.off;
				j+=i;
				printk("<1>addr=0x%lx  num=%d\n",j,k_am.vadd[0].mind);
				kv1.pin=1;kv1.thread_lock=1;//锁定操作最主要的设置就是kv1.pin=1;保证接收缓冲区为mp
				kernel_thread(lock_srh,NULL,CLONE_KERNEL);//启动锁定线程
				return size;
			}
			if(tmp[1]==5)//测试
			{
				kv1.pin=1;kv1.thread_lock=1;
				kernel_thread(update_srh,NULL,CLONE_KERNEL);
				return size;
			}
			//下面还有修改，读取整页等操作。
		}
		else
		{//此时线程已启动，本次操作仅是表示用户已经拷贝完数据（选择每1000地址启动一个线程是这样），可以继续线程的查询操作
		//注意：此时tmp[0]必须是0,该操作应该是由用户设置。另外，在进行再次查找时，每次传送的
		//数据都是有效的，并且如果线程如果启动了，在这应该保存传入的地址。--这实际上是使用一个线程
		//完成全部本次查找还是每查找1000个地址启动一个新线程的选择。每次启动新线程无疑处理起来简单.
		//但是考虑到效率的话，这种单线程方式应该更加有效。	
			s_wait_mm(2);
			return size;
		}
	}
	s_wait_mm(2);
	return size;
	//return 0;
}//}}}
//{{{ int first_srh(void)
int first_srh(void *argc)
{
	char *c,*mc,*v;
	int ret,len,j,k,m,n;
	struct pid *kpid;
	struct task_struct *t_str;
	struct mm_struct *mm_str;
	struct vm_area_struct *vadr;
	struct page **pages;
	unsigned long adr;
	struct KVAR_SAD ksa;
	daemonize("ty_thd1");
	v=NULL;
	memset((void*)&mp[d_begin],0,d_len);
	mc=&mp[d_begin];
	kpid=find_get_pid(kv1.pid);
	if(kpid==NULL)
	{
		printk("<1>find_get_pid error!\n");
		goto ferr_01;
	}
	t_str=pid_task(kpid,PIDTYPE_PID);
	if(t_str==NULL)
	{
		printk("<1>pid_task error!\n");
		goto ferr_01;
	}
	mm_str=get_task_mm(t_str);
	if(mm_str==NULL)
	{
		printk("<1>get_task_mm error!\n");
		goto ferr_01;
	}
	//vadr=mm_str->mmap->vm_next->mmap->vm_next;
	printk("<1>pid: %d data addr is: 0x%lx~~~~0x%lx\n",kv1.pid,mm_str->start_data,mm_str->end_data);
	//printk("<1>stack??:0x%lx~~~~~~~0x%lx\n",vadr->);
	kv1.seg[0]=mm_str->start_brk;//mm_str->start_code; 2014-1-4修改
	kv1.seg[1]=mm_str->start_data;//end_code;
	printk("<1>code seg: 0x%lx data seg: 0x%lx\n",kv1.seg[0],kv1.seg[1]);
	n=0;
	for(m=0;m<2;m++)
	{//段的循环，只查询代码段和数据段。
		if(m==0)//2014-1-4修改为检索堆空间
			//vadr=mm_str->mmap;
			vadr=mm_str->mmap->vm_next->vm_next;
		else
			vadr=mm_str->mmap->vm_next;
		adr=vadr->vm_start;
		len=vma_pages(vadr);//取得页数量
		if(v==NULL)
		{
			v=(char *)vmalloc(sizeof(void*)*(len+1));
			pages=(struct page **)v;//这样做的目的是防止vfree释放时差生误解
			//pages=(struct page **)vmalloc(sizeof(void*)*(len+1));//分配足够数量的空间，保存页地址。
			if(pages==NULL)
			{
				printk("<1>vmalloc error11\n");
				goto ferr_01;
			}
		}
		memset((void*)pages,0,sizeof(void*)*(len+1));
		down_write(&mm_str->mmap_sem);
		ret=get_user_pages(t_str,mm_str,adr,len,0,0,pages,NULL);
		if(ret>0)
		{
			for(k=0;k<ret;k++)
			{
				if(pages[k]==NULL)
				{
					printk("<1>search finished\n");
					break; //查找完成
				}
				if(k>30000)
				{
					printk("<1>max saved pages=30000\n");
					break;
				}
				c=(char*)kmap(pages[k]);
				if(kv1.dnum<256)//按字节查询
				{
					for(j=0;j<4096;j++)
					{
						if(c[j]==kv1.dest[0])
						{
							memset((void*)&ksa,0,sizeof(ksa));
							ksa.s_b.sbit=(unsigned short)m;
							ksa.s_b.pbit=(unsigned short)k;
							ksa.off=(unsigned short)j;
							memcpy((void*)mc,(void*)&ksa,sizeof(ksa));
							mc+=4;//2013-12-31改为4字节长度。
							if(mc-&mp[d_begin]>7996)//写满一页，需要往用户空间传送了
							{
							//	kunmap(pages[k]);
							//	page_cache_release(pages[k]);
							//	up_write(&mm_str->mmap_sem);
							//	printk("<1>one page end\n");
								s_sync(1);//阻塞在此
							//	goto ferr_01;
								//重置指针
								mc=&mp[d_begin];
								memset((void*)&mp[d_begin],0,d_len);//清空数据缓冲
							}
						}
					}
				}
				if(kv1.dnum<0x10000 && kv1.dnum>0xff)
				{
					for(j=0;j<4095;j++)
					{
						if((c[j]==kv1.dest[0]) && (c[j+1]==kv1.dest[1]))
						{
							memset((void*)&ksa,0,sizeof(ksa));
							ksa.s_b.sbit=(unsigned short)m;
							ksa.s_b.pbit=(unsigned short)k;
							ksa.off=(unsigned short)j;
							memcpy((void*)mc,(void*)&ksa,sizeof(ksa));
							mc+=4;//2013-12-31改为4字节长度。
							if(mc-&mp[d_begin]>7996)//写满一页，需要往用户空间传送了
							{
								s_sync(1);//阻塞在此
								//重置指针
								mc=&mp[d_begin];
								memset((void*)&mp[d_begin],0,d_len);//清空数据缓冲
							}
						}
					}
				}
				if(kv1.dnum<0x1000000 && kv1.dnum>0xffff)
				{
					for(j=0;j<4094;j++)
					{
						if((c[j]==kv1.dest[0]) && (c[j+1]==kv1.dest[1]) && (c[j+2]==kv1.dest[2]))
						{
							memset((void*)&ksa,0,sizeof(ksa));
							ksa.s_b.sbit=(unsigned short)m;
							ksa.s_b.pbit=(unsigned short)k;
							ksa.off=(unsigned short)j;
							memcpy((void*)mc,(void*)&ksa,sizeof(ksa));
							mc+=4;//2013-12-31改为4字节长度。
							if(mc-&mp[d_begin]>7996)//写满一页，需要往用户空间传送了
							{
								s_sync(1);//阻塞在此
								//重置指针
								mc=&mp[d_begin];
								memset((void*)&mp[d_begin],0,d_len);//清空数据缓冲
							}
						}
					}
				}
				if(kv1.dnum<0x100000000 && kv1.dnum>0xffffff)
				{
					for(j=0;j<4093;j++)
					{
						if((c[j]==kv1.dest[0]) && (c[j+1]==kv1.dest[1]) && (c[j+2]==kv1.dest[2]) && (c[j+3]==kv1.dest[3]))
						{
							memset((void*)&ksa,0,sizeof(ksa));
							ksa.s_b.sbit=(unsigned short)m;
							ksa.s_b.pbit=(unsigned short)k;
							ksa.off=(unsigned short)j;
							memcpy((void*)mc,(void*)&ksa,sizeof(ksa));
							mc+=4;//2013-12-31改为4字节长度。
							if(mc-&mp[d_begin]>7996)//写满一页，需要往用户空间传送了
							{
								s_sync(1);//阻塞在此
								//重置指针
								mc=&mp[d_begin];
								memset((void*)&mp[d_begin],0,d_len);//清空数据缓冲
							}
						}
					}
				}
				kunmap(pages[k]);
				page_cache_release(pages[k]);
			}
		}//要释放申请的内存
		vfree((void*)v);
		v=NULL;pages=NULL;
		up_write(&mm_str->mmap_sem);
		printk("<1>seg finished\n");
	}//至此全部结束，需要将结束标志置位、取消线程锁。
	s_sync(2);
	kv1.thread_lock=0;//取消线程锁.退出线程
	printk("<1>kernel_thread exit!\n");
	return 0;
ferr_01:
//	printk("<1>ready to exit kernel_thread\n");
	s_sync(2);
	kv1.thread_lock=0;
	if(v!=NULL)
		vfree((void*)v);
	printk("<1>kernel_thread finished!\n");
	return 1;
}//}}}
//{{{ int next_srh(void *argc)
int next_srh(void *argc)
{
	struct KVAR_SAD kr;
	char *c,*mc,*md,*v;
	int ret,len,i,k;
	struct pid *kpid;
	struct task_struct *t_str;
	struct mm_struct *mm_str;
	struct vm_area_struct *vadr;
	struct page **pages;
	unsigned long adr;
	v=NULL;
	daemonize("ty_thd2");
//	memset((void*)&mp[d_begin],0,d_len);
	kpid=find_get_pid(kv1.pid);
	if(kpid==NULL)
	{
		printk("<1>find_get_pid error\n");
		goto nerr_01;
	}
	t_str=pid_task(kpid,PIDTYPE_PID);
	if(t_str==NULL)
	{
		printk("<1>pid_task error\n");
		goto nerr_01;
	}
	mm_str=get_task_mm(t_str);
	if(mm_str==NULL)
	{
		printk("<1>get_task_mm error\n");
		goto nerr_01;
	}
	printk("<1>pid: %d data addr is: 0x%lx~~~~0x%lx\n",kv1.pid,mm_str->start_data,mm_str->end_data);
	kv1.seg[0]=mm_str->start_brk;//mm_str->start_code; 2014-1-4修改
	kv1.seg[1]=mm_str->start_data;
//两个极为关键的设置：c->指向结果缓冲区 mc->指向地址集	
	c=&tp[d_begin];mc=&mp[d_begin];
	memset((void*)c,0,d_len);
//	len=vma_pages(mm_str->mmap->vm_next);
	vadr=mm_str->mmap->vm_next->vm_next;
	//len=vma_pages(mm_str->mmap); //2014-1-4修改
	len=vma_pages(vadr);
	if(v==NULL)
	{
		v=(char*)vmalloc(sizeof(void*)*(len+1));
		pages=(struct page **)v;
		if(pages==NULL)
		{
			printk("<1>vmalloc error12\n");
			goto nerr_01;
		}
	}
	//adr=mm_str->mmap->vm_start; 2014-1-4修改
	adr=vadr->vm_start;
	memset((void*)pages,0,sizeof(void*)*(len+1));
	down_write(&mm_str->mmap_sem);
	ret=get_user_pages(t_str,mm_str,adr,len,0,0,pages,NULL);
	i=-1;md=mc;
	if(ret>0)
	{
		while(1)
		{
			memset((void*)&kr,0,sizeof(kr));//这种处理方式比用户端的指针处理模式差了不少，以后再改吧，不过这里的goto确实让代码逻辑简洁了许多
			memcpy((void*)&kr,mc,sizeof(kr));//获得地址。
			if(kr.s_b.sbit!=0)
			{//此处表示代码段已经搜索完成。
				if(i!=-1)//此时已经运行了kmap,所以在退出前必须进行清理
				{
					kunmap(pages[i]);
					page_cache_release(pages[i]);
				}
				break;//跳出本次循环，进入代码段的查询。
			}
			if((kr.spg==0) && (kr.off==0))
			{
				printk("<1>next searching finished!\n");
				if(i!=-1)//此时已经运行了kmap,所以在退出前必须进行清理
				{
					kunmap(pages[i]);
					page_cache_release(pages[i]);
				}
				//不管i等不等于-1，down_write肯定要释放的。
				up_write(&mm_str->mmap_sem);
				//这里不管是代码段还是数据段的搜索，执行到这里就表示地址集中已经没有数据了，要作为全部搜索完成处理！
				mp[7]=1;
				s_sync(12);
				vfree(v);v=NULL;
				printk("<1>number 4\n");
				goto normal_01;
			}
			if(ret<=kr.s_b.pbit)// || kr.spg==0) //保证页索引没有越界,或者为空
			{
				printk("<1>page index error101\n");
				if(i!=-1)//此时已经运行了kmap,所以在退出前必须进行清理
				{
					kunmap(pages[i]);
					page_cache_release(pages[i]);
				}
				//不管i等不等于-1，down_write肯定要释放的。
				up_write(&mm_str->mmap_sem);
				//这里不管是代码段还是数据段的搜索，执行到这里就表示地址集中已经没有数据了，要作为全部搜索完成处理！
				mp[7]=1;
				s_sync(12);
				vfree(v);v=NULL;
				goto normal_01;
			}
			if(i==-1)
			{
				i=kr.s_b.pbit;
				md=(char*)kmap(pages[kr.s_b.pbit]);
			}
			if(i!=kr.s_b.pbit)
			{
				kunmap(pages[i]);
				page_cache_release(pages[i]);
				md=(char*)kmap(pages[kr.s_b.pbit]);
				i=kr.s_b.pbit;
			}
			k=0;
			if(k_am.snum<256)
			{
				if(k_am.sch[0]==md[kr.off])
				{
					//printk("<1>find it!!!!!!\n");
					k=1;
				}
				goto n_01; 
			}
			if(k_am.snum<0x10000)
			{
				if((k_am.sch[0]==md[kr.off]) &&(k_am.sch[1]==md[kr.off+1]))
					k=1;
				goto n_01;
			}
			if(k_am.snum<0x1000000)
			{
				if((k_am.sch[0]==md[kr.off]) &&(k_am.sch[1]==md[kr.off+1]) &&(k_am.sch[2]==md[kr.off+2]))
					k=1;
				goto n_01;
			}
			if((k_am.sch[0]==md[kr.off]) &&(k_am.sch[1]==md[kr.off+1]) &&(k_am.sch[2]==md[kr.off+2]) && (k_am.sch[3]==md[kr.off+3]))
				k=1;
n_01:			
			if(k==1)//find it
			{
				memcpy(c,(void*)&kr,sizeof(kr));
				c+=4;
				if(c-&tp[d_begin]>7996) //写满一页了
				{
					s_sync(10);
					//清空缓冲区
					memset((void*)&tp[d_begin],0,d_len);
					c=&tp[d_begin];
				}
			}
			mc+=4;//传入的2000个地址查询完成，需要再次读入
			if(mc-&mp[d_begin]>7996)
			{//注意，此时tp中可能已经有了本次的查询结果，所以，应禁止对tp的任何改动
				s_sync(12);
				mc=&mp[d_begin];				
			}
		}
	}
	up_write(&mm_str->mmap_sem);
	printk("<1>code seg end\n");
	if(v!=NULL)
	{	
		vfree(v);
		v=NULL;pages=NULL;
	}	
	if(mm_str->mmap->vm_next==NULL)
	{
		printk("<1>data seg is NULL\n");
		goto nerr_01;
	}
	len=vma_pages(mm_str->mmap->vm_next);
	v=(char*)vmalloc(sizeof(void*)*(len+1));
	pages=(struct page **)v;
	if(pages==NULL)
	{
		printk("<1>vmalloc error123\n");
		goto nerr_01;
	}
	adr=mm_str->mmap->vm_next->vm_start;
	memset((void*)pages,0,sizeof(void*)*(len+1));
	down_write(&mm_str->mmap_sem);
	ret=get_user_pages(t_str,mm_str,adr,len,0,0,pages,NULL);
	i=-1;
	if(ret>0)
	{
		while(1)
		{
			memset((void*)&kr,0,sizeof(kr));
			memcpy((void*)&kr,mc,sizeof(kr));//获得地址。
			if(kr.s_b.sbit!=1)//由于首次查询时是按照代码段，数据段的顺序搜索的，所以，在执行到数据段查询时，不会再出现非数据段的情况。
			{//此处可作为全部查询结束处理
				if(i!=-1)
				{
					kunmap(pages[i]);
					page_cache_release(pages[i]);
				}
				mp[7]=1;
				s_sync(12);
				up_write(&mm_str->mmap_sem);
				vfree(v);v=NULL;
				printk("<1>number 1\n");
				goto normal_01;
				//break;//iiiiiiiiiiiiiiiiiiiiiii
			}
			if(ret<=kr.s_b.pbit)// || kr.spg==0) //保证页索引没有越界
			{
				printk("<1>page index error103\n");
				if(i!=-1)//此时已经运行了kmap,所以在退出前必须进行清理
				{
					kunmap(pages[i]);
					page_cache_release(pages[i]);
				}
				//不管i等不等于-1，down_write肯定要释放的。
				up_write(&mm_str->mmap_sem);
				//这里不管是代码段还是数据段的搜索，执行到这里就表示地址集中已经没有数据了，要作为全部搜索完成处理！
				mp[7]=1;
				s_sync(12);
				vfree(v);v=NULL;
				printk("<1>number 2\n");
				goto normal_01;
			}
			if((kr.spg==0) && (kr.off==0))
			{
				printk("<1>next searching finished!\n");
				if(i!=-1)//此时已经运行了kmap,所以在退出前必须进行清理
				{
					kunmap(pages[i]);
					page_cache_release(pages[i]);
				}
				//不管i等不等于-1，down_write肯定要释放的。
				up_write(&mm_str->mmap_sem);
				//这里不管是代码段还是数据段的搜索，执行到这里就表示地址集中已经没有数据了，要作为全部搜索完成处理！
				mp[7]=1;
				s_sync(12);
				vfree(v);v=NULL;
				printk("<1>number 3\n");
				goto normal_01;
			}
			if(i==-1)
			{
				i=kr.s_b.pbit;
				md=(char*)kmap(pages[kr.s_b.pbit]);
			}
			if(i!=kr.s_b.pbit)
			{
				kunmap(pages[i]);
				page_cache_release(pages[i]);
				md=(char*)kmap(pages[kr.s_b.pbit]);
				i=kr.s_b.pbit;
			}
			k=0;
			if(k_am.snum<256)
			{
				if(k_am.sch[0]==md[kr.off])
				{
					//printk("<1>find it!!!!!!\n");
					k=1;
				}
				goto n_02;
			}
			if(k_am.snum<0x10000)
			{
				if((k_am.sch[0]==md[kr.off]) &&(k_am.sch[1]==md[kr.off+1]))
					k=1;
				goto n_02;
			}
			if(k_am.snum<0x1000000)
			{
				if((k_am.sch[0]==md[kr.off]) &&(k_am.sch[1]==md[kr.off+1]) &&(k_am.sch[2]==md[kr.off+2]))
					k=1;
				goto n_02;
			}
			if((k_am.sch[0]==md[kr.off]) &&(k_am.sch[1]==md[kr.off+1]) &&(k_am.sch[2]==md[kr.off+2]) && (k_am.sch[3]==md[kr.off+3]))
				k=1;
n_02:
			if(k==1)//find it
			{
				memcpy(c,(void*)&kr,sizeof(kr));
				c+=4;
				if(c-&tp[d_begin]>7996) //写满一页了
				{
					s_sync(10);
					memset((void*)&tp[d_begin],0,d_len);
					c=&tp[d_begin];
				}
			}
			mc+=4;//传入的2000个地址查询完成，需要再次读入
			if(mc-&mp[d_begin]>7996)
			{//注意，此时tp中可能已经有了本次的查询结果，所以，应禁止对tp的任何改动
				s_sync(12);
				mc=&mp[d_begin];				
			}
		}
	}
	up_write(&mm_str->mmap_sem);
	printk("<1>data seg end!\n");
	if(v!=NULL)
	{	
		vfree(v);
		v=NULL;
	}//写入最后剩余的地址集	
	//确保为最终退出操作：
	//mp[7]=1;
	//s_sync(10);
normal_01:	
	kv1.thread_lock=0;
	printk("<1>next search thread exit\n");
	return 0;
nerr_01:
	//错误的时候把地址缓冲清空再传送
	memset((void*)tp,0,K_BUFFER_SIZE);
	mp[7]=1;
	s_sync(12);
	kv1.thread_lock=0;
	if(v!=NULL)
		vfree((void*)v);
	printk("<1>next search thread exit111\n");
	return 1;
}//}}}
//{{{ void s_sync(int t)
/*该函数可根据不同的标志字段的设置，确定标志的设置，并进入等待,等待完成后再恢复相关标志。
  0、等待命令状态：
  	 （1）此时内核没有工作，仅是等待用户的指令。设置接受缓冲区为mp即可：kv1.pin=1;该状态不能设置，在系统初始化时或其他状态完成后自动进入该状态。
  1、首次查询：
  	 只有向外传送一种状态，传送时又有三种情况：
	 （1）内核没有完成工作，正常的用户查询同步，此时只需将缓冲区设置标志kv1.pin设为：0即可，此时read函数会根据该标志返回用户的查询字长为0,
	 表示等待中。t=0时设定此状态。次状态没有等待。
	 （2）内核完成一次（2000地址）的搜索，需要将地址结果传送给用户。此时搜索地址保存在mp缓冲区中，先将mp[0]=1表示用户获取的数据有效，mp[7]=0
	 表示后续还有结果。最后将改变缓冲区标志kv1.pin设为：1,然后进入查询等待过程，查询mp[0]==0？等于0表示用户已将传出的地址保存，可以继续。此时
	 修改kv1.pin=0完成退出。t=1时设置此状态。
	 （3）内核完成全部搜索，先将mp[0]=1,mp[7]=1，表示这是最后的查询。用户在收到最后一批地址后就可再次发送其他命令。再将kv1.pin=1。t=2设置此状态。
	 此状态也无需等待，直接退出。
  2、再次查询：
  	 该操作不仅有向外传送的状态，还有等待用户传入地址集的状态。
	 （一）向外传送的状态：
	 （1）内核查询中，正常的用户查询同步，此时将kv1.pin设为：0。该步与首次查询的步骤1完全相同。
	 （2）内核完成一次查询，需要将地址结果传送给用户，此时保存结果的缓冲区为tp，所以将tp[0]=1，表示数据有效，tp[7]=0,表示不是最终的搜索，tp[8]=0,
	 表示本次操作是传送结果。然后将缓冲区标志kv1.pin设为：2,表示此时的输出缓冲区为tp。然后进入等待状态。在等待过程中轮寻tp[0]==0?等于0表示用户操作
	 已经完成，可以退出等待，继续搜索了，此时将kv1.pin=0。t=10设置次状态。
	 （3）内核完成全部查询，此时输出缓冲区仍然为tp，设置tp[0]=1,tp[7]=1,tp[8]=0,kv1.pin=2。注意此时所有工作虽已完成，但不能直接退出，因为此时的默认
	 接受缓冲区为tp而不是默认的等待命令状态的mp缓冲区。所以，此时仍需进入等待，轮寻tp[0]==0?当tp[0]==0时，设置kv1.pin=1。完成退出。t=11设置此状态
	 （二）接受用户输入地址的状态：
	 （1）接受用户输入，此时使用的输入输出缓冲区为mp，所以mp[0]=1，表示用户接受有效的返回数据（命令），mp[7]=0，注意此时的mp[7]只是准备接受用户设置
	 的一个标志，用于标识是否全部地址已都输入？返回0时表示还有地址没有输入，返回1时说明这是最后的地址集。mp[8]=1，表示本次的输出为命令，要求用户输
	 入地址集。最后设置缓冲区切换标志kv1.pin=1，表示mp为输入输出缓冲区。然后进入等待，轮寻mp[0]==0?为真表示已经接受了用户输入的有效地址集。mp[8]=0,
	 kv1.pin=0。完成退出。t=12设置次状态。
注意：

	（1）再次搜索的全部完成标志tp[7]=1的设置依据就是用户设置的地址集的全部输入完成标志：mp[7]=1!!
  	（2）本函数所有涉及到的mp,tp标志的设置，都是依据k_am的状态设置。也就是内核对这些标志的最好不要直接修改，而是修改k_am相关标志，而本函数完成k_am
	到缓冲区标志的设置。 
问题：
	2014-1-3发现的问题，在进行"再次查找"时这个问题可能造成最后一个地址集中的部分地址没有比较就认为"再次查询"已经结束。原因就是当从用户端读取了最后
	一页的地址集时就已经将mp[7]置位，这表示这是最后的地址集了，查询完该集"再次查询"就结束了。而此时如果存储查询结果的缓冲已满，再检查mp[7]标志就会
	认为要进行完全退出操作，而实际上地址集中可能还有未进行比较的地址！当然这种错误不是每次出现的，只有在最后一页的地址集还没全部比较完，结果缓冲区
	就已经满了的情况才会出现。对于这种问题的避免，我认为最简单而又最有效的方法就是在"首次查询"时记录全部查询结果（地址集）的数量。而不能按页计数。
	这样就能保证在取得最后一页地址集进行比较时，如果此时发生了结果缓冲区满的情况，需要先判断下已经比较过的结果数量和全部数量是否有差异，小于全部数
	量则不能做为全部完成的操作。又冒出个更精彩的想法：禁止结果缓冲输出时产生全部完成退出的操作。也即完全退出只能是在获取新的地址集时，或者该地址集
	中出现空指针时。

	------我的牛B之处就在于：我不只是通过程序测试发现某些逻辑性的问题，而是在写代码的时候就能预见和发现很多这些问题！
 */
void s_sync(int t)
{
	switch(t)
	{
	case 0://内核忙，返回用户的读取字节数为0
		kv1.pin=0;
		return;//没有等待。
	case 1://首次查询，完成一页的地址。
		k_am.sync=1;//mp[0]=1;mp[7]=0;
		k_am.end0=0;//首次查询的结束标志必须由内核决定。
//		memcpy((void*)&mp[40],(void*)&kv1.seg[1],sizeof(int));//save data segment
//		memcpy((void*)&mp[44],(void*)&kv1.seg[0],sizeof(int));//save text segment
		k_am.text_seg=kv1.seg[0];
		k_am.data_seg=kv1.seg[1];
		memcpy((void*)mp,(void*)&k_am,sizeof(k_am));
		kv1.pin=1;		//确定的输入输出缓冲区为mp;
		//call wait
		s_wait_mm(0);
		kv1.pin=0;k_am.sync=mp[0];
		return;
	case 2://首次查询全部完成
		k_am.sync=1;//mp[0]=1;
		k_am.end0=1;//mp[7]=1;
//		memcpy((void*)&mp[40],(void*)&kv1.seg[1],sizeof(int));//save data segment
//		memcpy((void*)&mp[44],(void*)&kv1.seg[0],sizeof(int));//save text segment
		k_am.text_seg=kv1.seg[0];
		k_am.data_seg=kv1.seg[1];
		memcpy((void*)mp,(void*)&k_am,sizeof(k_am));
		kv1.pin=1;
		s_wait_mm(0);
		return;
	case 3://锁定目标数据,注意：与查询操作不同，该操作的退出是被动的，所以不能设置
		//k_am.sync=1，
		kv1.pin=1;
		s_wait_mm(2);//只需休眠1秒，然后由线程代码判断end0是否等于1。确定是否退出.
		return;
	case 10://再次查询，内核完成一次查询,传送结果给用户 再次查询的结果传送中间查询和最终查询合并，统一使用序号10
		k_am.sync=1;//tp[0]=1;
		//tp[7]  此时该标志只能根据mp[7]确定。
//		k_am.end0=mp[7];//tp[7]=mp[7];
		k_am.end0=0;//此处改为在输送结果时永远不能产生全部退出的操作。
		k_am.vv0[0]=0;//传送的结果。tp[8]=0;
		k_am.text_seg=kv1.seg[0];
		k_am.data_seg=kv1.seg[1];
		memcpy((void*)tp,(void*)&k_am,sizeof(k_am));
		kv1.pin=2;	//缓冲区切换为tp
		//call wait
		s_wait_mm(1);
//		if(tp[7]==0)//不是最终搜索
		kv1.pin=0;
//		else
//			kv1.pin=1;
		return;
	case 12://接受用户输入的地址
		k_am.text_seg=kv1.seg[0];
		k_am.data_seg=kv1.seg[1];
		if(mp[7]!=0)
		{//此时应该自动转至最终查询
			k_am.sync=1;//tp[0]=1;
			k_am.end0=1;//tp[7]=1;
			k_am.vv0[0]=0;//tp[8]=0;
			memcpy((void*)tp,(void*)&k_am,sizeof(k_am));
			kv1.pin=2;
			s_wait_mm(1);
			kv1.pin=1;
			memset((void*)&k_am,0,sizeof(k_am));
			return;
		}
		k_am.sync=1;//mp[0]=1;
		k_am.end0=0;//mp[7]=0;
		k_am.vv0[0]=1;//mp[8]=1;
		memcpy((void*)mp,(void*)&k_am,sizeof(k_am));
		kv1.pin=1;
		//call wait
		s_wait_mm(0);
		mp[8]=0;k_am.vv0[0]=0;
		kv1.pin=0;
		return;
	case 13://测试
		k_am.sync=1;k_am.end0=1;
		memcpy((void*)mp,(void*)&k_am,sizeof(k_am));
		s_wait_mm(0);
		kv1.pin=1;
	default:
		printk("<1>error combo\n");
		break;
	};

}//}}}
//{{{ void s_wait_mm()
void s_wait_mm(int w)
{
	wait_queue_head_t	whead;
	wait_queue_t	wdata;
	char *ch;
	init_waitqueue_head(&whead);
	init_waitqueue_entry(&wdata,current);
	add_wait_queue(&whead,&wdata);
	printk("<1>a sleep\n");
	if(w==0)//mp
		ch=mp;
	else
	{
		if(w==1)
			ch=tp;
		else
		{
			sleep_on_timeout(&whead,HZ);
			return;
		}
	}
	while(1)
	{
		if(ch[0]==0)
			break;
		sleep_on_timeout(&whead,200);
	}
}//}}}
//{{{ int update_srh(void *argc)
int update_srh(void *argc)
{
	struct pid *kpid;
	struct task_struct *t_str;
	struct mm_struct *mm_str;
	struct vm_area_struct *vadr1,*vadr2;
	daemonize("ty_thd3");
	kpid=find_get_pid(kv1.pid);
	if(kpid==NULL)
	{
		printk("<1>find_get_pid error\n");
		goto uerr_01;
	}
	t_str=pid_task(kpid,PIDTYPE_PID);
	if(t_str==NULL)
	{
		printk("<1>pid_task error\n");
		goto uerr_01;
	}
	mm_str=get_task_mm(t_str);
	if(mm_str==NULL)
	{
		printk("<1>get_task_mm error\n");
		goto uerr_01;
	}
	printk("<1>pid:%d data addr is: 0x%lx code addr is: 0x%lx stack is: 0x%lx\n",kv1.pid,mm_str->start_data,mm_str->start_code,mm_str->start_stack);
	vadr1=mm_str->mmap->vm_next;vadr2=vadr1->vm_next;
	printk("<1>0x%ld   0x%ld    0x%ld\n",mm_str->mmap->vm_start,vadr1->vm_start,vadr2->vm_start);
uerr_01:
	//s_sync(13);
	kv1.thread_lock=0;
	return 0;
}//}}}
//{{{ int lock_srh()
int lock_srh(void *argc)
{
	struct pid *kpid;
	struct task_struct *t_str;
	struct mm_struct *mm_str;
	struct vm_area_struct *vadr;
	struct page **pages;
	struct KVAR_AM *k_am;
	struct KVAR_SAD *ksa;
	char *v,*c;
	int i,k,ret,len;
	unsigned long adr;
	v=NULL;k=0;
	c=mp;
	daemonize("ty_thd4");
	k_am=(struct KVAR_AM *)mp;
	/*for(i=0;i<8;i++)
	{
		memset((void*)&k1[i],0,sizeof(struct KVAR_SAD));
		memset((void*)&k2[i],0,sizeof(struct KVAR_SAD));
		ksa=(struct KVAR_SAD*)&(k_am->vadd[i].ksa);
		if(ksa->s_b.sbit==0)//堆段在k1
		{
			memcpy((void*)&k1[i],(void*)ksa,sizeof(struct KVAR_SAD));
			if(m1>ksa->s_b.pbit)
				m1=ksa->s_b.pbit;
		}
		else
		{
			memcpy((void*)&k2[i],(void*)ksa,sizeof(struct KVAR_SAD));
			if(m2>ksa->s_b.pbit)
				m2=ksa->s_b.pbit;
		}
	}*/
	kpid=find_get_pid(kv1.pid);
	if(kpid==NULL)
	{
		printk("<1>find_get_pid error\n");
		goto lerr_01;
	}
	t_str=pid_task(kpid,PIDTYPE_PID);
	if(t_str==NULL)
	{
		printk("<1>pid_task error\n");
		goto lerr_01;
	}
	mm_str=get_task_mm(t_str);
	if(mm_str==NULL)
	{
		printk("<1>get_task_mm error\n");
		goto lerr_01;
	}
	if(k_am->vadd[0].ksa.s_b.sbit==0)
		vadr=mm_str->mmap->vm_next->vm_next;
	else
		vadr=mm_str->mmap->vm_next;
	adr=vadr->vm_start;
	len=vma_pages(vadr);
	if(v==NULL)
	{
		v=(char*)vmalloc(sizeof(void*)*(len+1));
	}
	pages=(struct page **)v;
	if(pages==NULL)
	{
		printk("<1>kmalloc error31\n");
		goto lerr_01;
	}
	memset((void*)pages,0,sizeof(void*)*(len+1));
	down_write(&mm_str->mmap_sem);
	ksa=(struct KVAR_SAD *)&(k_am->vadd[0].ksa);
	ret=get_user_pages(current,mm_str,adr,len,0,0,pages,NULL);
	if(ret<=ksa->s_b.pbit)
	{
		printk("<1>page index error\n");
		up_write(&mm_str->mmap_sem);
		goto lerr_01;
	}
	i=ksa->s_b.pbit;
	while(1)
	{
		c=kmap(pages[i]);
		c[ksa->off]=k_am->vadd[0].mind;
		kunmap(pages[i]);
		//page_cache_release(pages[i]);
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(2 * HZ); 
		//s_sync(3);
		if(k_am->end0==1)//退出！
		{
			page_cache_release(pages[i]);
			up_write(&mm_str->mmap_sem);
			goto lerr_01;
		}
		//s_sync(3);
	}
	/*while(1)
	{
		for(k=0;k<8;k++)
		{
			if((k_am->vadd[k].ksa.spg)==0 && (k_am->vadd[k].ksa.off==0))
				break;
		}//k保存了实际要锁定地址的个数
		//j=k_am->vadd[0].ksa.s_b.sbit;//j确定了首个锁定地址所在的段。
		//if(j==1)//锁定地址没有堆的，直接跳转至数据段
		//	goto lnor_01;
		//首先进行堆的查询:(原来的代码段)
		vadr=mm_str->mmap->vm_next->vm_next;
		adr=vadr->vm_start;
		len=vma_pages(vadr);
		if(v==NULL)
		{
			v=(char*)vmalloc(sizeof(void*)*(len+1));
		}
		pages=(struct page **)v;
		if(pages==NULL)
		{
			printk("<1>kmalloc error31\n");
			goto lerr_01;
		}
		memset((void*)pages,0,sizeof(void*)*10);
		down_write(&mm_str->mmap_sem);
		ret=get_user_pages(t_str,mm_str,adr,len,0,0,pages,NULL);
		if(ret>0)
		{
			i=-1;
			for(j=0;j<k;j++)
			{//这里是堆的判断和修改。所以j已经没有必要了
				ksa=&(k_am->vadd[j].ksa);
				if(ksa->s_b.sbit!=0)//地址不在堆中了。
				{
					if(i!=-1)
					{
						kunmap(pages[i]);
						page_cache_release(pages[i]);
						i=-1;
					}
					continue;
					//up_write(&mm_str->mmap_sem);
					//goto lnor_01;
				}
				if(ksa->spg==0 && ksa->off==0)
				{//因为有k的限制，流程不可能到达这里的，还是写上吧。
					printk("<1>it's impossible! you shouldn't be here neo\n");
					//既然到这里了，就认为全部锁定完成。
					if(i!=-1)
					{
						kunmap(pages[i]);
						page_cache_release(pages[i]);
					}
					up_write(&mm_str->mmap_sem);
					goto lerr_01;
				}
				if(ret<=ksa->s_b.pbit || ksa->off>4096)
				{//第三项检查:页索引和页内偏移的检查。这里也是不应到达的
					printk("<1>there is another station00oo!\n");
					if(i!=-1)
					{
						kunmap(pages[i]);
						page_cache_release(pages[i]);
					}
					up_write(&mm_str->mmap_sem);
					goto lerr_01;
				}
				if(i==-1)
				{
					i=ksa->s_b.pbit;
					c=(char*)kmap(pages[i]);
				}
				if(i!=ksa->s_b.pbit)
				{//不在同一页上，需要重新映射。
					kunmap(pages[i]);
					page_cache_release(pages[i]);
					i=ksa->s_b.pbit;
					c=(char*)kmap(pages[i]);
				}
				m=k_am->vadd[j].mind;
				if(m!=0)//数据修改规则：下限不为0则以大于下限为准
				{
					if(m<0x100)//一字节
					{
						if(c[ksa->off]<m)
						{
							c[ksa->off]=(char)m;
						}
						continue;
					}
					if(m<0x10000)//2字节
					{
						n=0;
						memcpy((void*)&n,(void*)&c[ksa->off],2);
						if(n<m)
							memcpy((void*)&c[ksa->off],(void*)&m,2);
						continue;
					}
					if(m<0x1000000)//3字节
					{
						n=0;memcpy((void*)&n,(void*)&c[ksa->off],3);
						if(n<m)
							memcpy((void*)&c[ksa->off],(void*)&m,3);
						continue;
					}
					if(m<0xffffffff)//4字节
					{
						n=0;memcpy((void*)&n,(void*)&c[ksa->off],4);
						if(n<m)
							memcpy((void*)&c[ksa->off],(void*)&m,4);
					}
					continue;
				}
				else//下限为0,则以小于上限为准
				{
					m=k_am->vadd[j].maxd;
					if(m<0x100)//一字节
					{//这里的处理不太合理，因为内存的数据如果已经是2字节的数字，
						//这样处理就不合理了。下同，先记下，以后完善。
						if(c[ksa->off]>m)
						{
							c[ksa->off]=(char)m;
						}
						continue;
					}
					if(m<0x10000)//2字节
					{
						n=0;
						memcpy((void*)&n,(void*)&c[ksa->off],2);
						if(n>m)
							memcpy((void*)&c[ksa->off],(void*)&m,2);
						continue;
					}
					if(m<0x1000000)//3字节
					{
						n=0;memcpy((void*)&n,(void*)&c[ksa->off],3);
						if(n>m)
							memcpy((void*)&c[ksa->off],(void*)&m,3);
						continue;
					}
					if(m<0xffffffff)//4字节
					{
						n=0;memcpy((void*)&n,(void*)&c[ksa->off],4);
						if(n>m)
							memcpy((void*)&c[ksa->off],(void*)&m,4);
					}
					continue;
				}
			}
		}//至此，完成了全部堆中地址的锁定。开始进行数据段地址的锁定。
		up_write(&mm_str->mmap_sem);
//lnor_01:
		if(v!=NULL)
		{	vfree(v);v=NULL;}
		//if(k<=0)//全部地址都已经锁定了，检查是否直接退出。
		//	goto lnor_02;	
		vadr=mm_str->mmap->vm_next;
		adr=vadr->vm_start;
		len=vma_pages(vadr);
		v=(char*)vmalloc(sizeof(void*)*(len+1));
		pages=(struct page **)v;
		if(pages==NULL)
		{
			printk("<1>kmalloc error312\n");
			goto lerr_01;
		}
		memset((void*)pages,0,sizeof(void*)*10);
		down_write(&mm_str->mmap_sem);
		ret=get_user_pages(t_str,mm_str,adr,len,0,0,pages,NULL);
		if(ret>0)
		{
			i=-1;
			for(j=0;j<k;j++)
			{//这里是堆的判断和修改。所以j已经没有必要了
				ksa=&(k_am->vadd[j].ksa);
				if(ksa->s_b.sbit!=1)//地址不在堆中了。
				{
					if(i!=-1)
					{
						kunmap(pages[i]);
						page_cache_release(pages[i]);
						i=-1;
					}
					continue;
					//up_write(&mm_str->mmap_sem);
					//goto lnor_02;
				}
				if(ksa->spg==0 && ksa->off==0)
				{//因为有k的限制，流程不可能到达这里的，还是写上吧。
					printk("<1>it's impossible! you shouldn't be here neo\n");
					//既然到这里了，就认为全部锁定完成。
					if(i!=-1)
					{
						kunmap(pages[i]);
						page_cache_release(pages[i]);
					}
					up_write(&mm_str->mmap_sem);
					goto lerr_01;
				}
				if(ret<=ksa->s_b.pbit || ksa->off>4096)
				{//第三项检查:页索引和页内偏移的检查。这里也是不应到达的
					printk("<1>there is another station!\n");
					if(i!=-1)
					{
						kunmap(pages[i]);
						page_cache_release(pages[i]);
					}
					up_write(&mm_str->mmap_sem);
					goto lerr_01;
				}
				if(i==-1)
				{
					i=ksa->s_b.pbit;
					c=(char*)kmap(pages[i]);
				}
				if(i!=ksa->s_b.pbit)
				{//不在同一页上，需要重新映射。
					kunmap(pages[i]);
					page_cache_release(pages[i]);
					i=ksa->s_b.pbit;
					c=(char*)kmap(pages[i]);
				}
				m=k_am->vadd[j].mind;
				if(m!=0)//数据修改规则：下限不为0则以大于下限为准
				{
					if(m<0x100)//一字节
					{
						if(c[ksa->off]<m)
						{
							c[ksa->off]=(char)m;
						}
						continue;
					}
					if(m<0x10000)//2字节
					{
						n=0;
						memcpy((void*)&n,(void*)&c[ksa->off],2);
						if(n<m)
							memcpy((void*)&c[ksa->off],(void*)&m,2);
						continue;
					}
					if(m<0x1000000)//3字节
					{
						n=0;memcpy((void*)&n,(void*)&c[ksa->off],3);
						if(n<m)
							memcpy((void*)&c[ksa->off],(void*)&m,3);
						continue;
					}
					if(m<0xffffffff)//4字节
					{
						n=0;memcpy((void*)&n,(void*)&c[ksa->off],4);
						if(n<m)
							memcpy((void*)&c[ksa->off],(void*)&m,4);
					}
					continue;
				}
				else//下限为0,则以小于上限为准
				{
					m=k_am->vadd[j].maxd;
					if(m<0x100)//一字节
					{//这里的处理不太合理，因为内存的数据如果已经是2字节的数字，
						//这样处理就不合理了。下同，先记下，以后完善。
						if(c[ksa->off]>m)
						{
							c[ksa->off]=(char)m;
						}
						continue;
					}
					if(m<0x10000)//2字节
					{
						n=0;
						memcpy((void*)&n,(void*)&c[ksa->off],2);
						if(n>m)
							memcpy((void*)&c[ksa->off],(void*)&m,2);
						continue;
					}
					if(m<0x1000000)//3字节
					{
						n=0;memcpy((void*)&n,(void*)&c[ksa->off],3);
						if(n>m)
							memcpy((void*)&c[ksa->off],(void*)&m,3);
						continue;
					}
					if(m<0xffffffff)//4字节
					{
						n=0;memcpy((void*)&n,(void*)&c[ksa->off],4);
						if(n>m)
							memcpy((void*)&c[ksa->off],(void*)&m,4);
					}
					continue;
				}
			}
		}
		up_write(&mm_str->mmap_sem);
//lnor_02:
		//这里进行是否持续锁定的判断。注意：这里和两个查询线程不同的是退出不是主动的
		//而是根据用户的设定确定退出的。
		s_sync(3);
		if(k_am->end0==1)//退出！
			goto lerr_01;
	}*/
lerr_01:
	if(v!=NULL)
		vfree(v);
	kv1.thread_lock=0;	
	return 0;
}//}}}


//{{{ void class_create_release(struct class *cls)
void class_create_release(struct class *cls)
{
	pr_debug("%s called for %s\n",__func__,cls->name);
	kfree(cls);
}//}}}



module_init(areg_init);
module_exit(areg_exit);





