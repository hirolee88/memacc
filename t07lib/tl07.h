#include"clsscr.h"
#include<gtk/gtk.h>

struct win_struct
{
	GtkWidget *window;
	GtkWidget *fixed;
	GtkWidget *scroll[2];
	GtkWidget *list[3];
	GtkListStore *store[3];
	GtkWidget *bnt[10];
	GtkWidget *entry[5];
	GtkWidget *radio[2];
	GtkWidget *prog;
	GtkStatusIcon *sicon;
	GtkWidget *menu;
	GtkWidget *menu_restore;
	GtkWidget *menu_set;
	GtkWidget *menu_exit;
	GtkWidget *label;
//下列定义与模块及线程相关	
	int		  thread_lock;//线程锁
	char	  *g_ch;		//通讯缓冲
	char	  *g_addr[2]; //两个结果地址缓冲
	int		  pid;		//进程pid
	int		  sn;		//查找数字
	unsigned int  dseg;
	unsigned int  bseg;//text seg
};
struct win_struct ws;
//定义一个与模块相关的结构。
struct SEG_DESC
{
    unsigned short pbit:15;         //页占15位
    unsigned short sbit:1;          //段占1位，=0表示代码段，=1数据段。
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
    unsigned int maxd;          //上限
    unsigned int mind;          //下限
    struct KVAR_SAD ksa;        //地址结构
    unsigned char vv0[4];       //无用，使结构为16字节
};

struct KVAR_AM
{
    unsigned char sync;                 //命令字段中0字节的同步标志，
    unsigned char cmd;                  //命令字段中1字节的主命令标志
    union{
    int   pid;                          //命令字段中的2-5字节，传入的pid
    unsigned char  pch[4];
    };
    unsigned char proc;                 //命令字段中的6字节，存放进度值1-100
    unsigned char end0;                 //命令字段中的7字节，存放本次查询的结束标志，=1结束
    char vv0[2];                        //命令字段中的8,9字节，暂时未用。再次查询时在副本中用到8字节=3表示有效。
    union{
    unsigned long snum;                 //命令字段中的10字节起的查询关键字。
    unsigned char sch[4];
    };
    char oaddr[4];                      //命令字段中的14字节起的导出地址。
    char vv1[22];                       //18-47字节暂时未,可以考虑传送代码段和数据段的段地址。
    union{
    unsigned int data_seg;
    unsigned char dsg[4];
    };
    union{
    unsigned int text_seg;
    unsigned char tsg[4];
    };
    struct KVAR_ADDR vadd[8];           //8个锁定（修改）数据和地址。 
};
struct SAVE_LOCK
{
	char ch[40];
	struct KVAR_ADDR add;
};
struct KVAR_SAD	 ksa;
struct KVAR_AM	 kam;
int idx;
char doff[40];
struct SAVE_LOCK sl[8];
struct timeval tm;
//{{{ define
#define buf_size			8192
#define adr_buf1			1024*240
#define adr_buf2			4096*10
#define d_begin				192
#define d_len				8000
#define drv_name			"/dev/memacc_8964_dev"

#define	wintitle			"修改大师V1.0"
//window's width & height
#define win_x				640
#define win_y				492
//lab1's width & height
#define lab1_w				70
#define lab1_h				25
#define lab1_px				10
#define lab1_py				lab1_px
//treeview1's width & height
#define t1_w				160
#define t1_h				275
//treeview's position
#define t1_px				10
#define t1_py				40
//scrollbar1's width
#define srl_w				16
#define srl_h				t1_h
//scrollbar1's position
#define srl_px				11+t1_w
#define srl_py				t1_py
//bnt1~2's pos and wid
#define bnt1_w				75
#define bnt1_h				22
#define bnt1_px				t1_px
#define bnt1_py				t1_h+t1_py+10
#define bnt2_w				bnt1_w
#define	bnt2_h				bnt1_h
#define bnt2_px				bnt1_px+bnt1_w+srl_w+10
#define bnt2_py				bnt1_py
//part2 label
#define lab2_w				100
#define lab2_h				lab1_h
#define lab2_px				srl_w+srl_px+5
#define lab2_py				lab1_py
//输入项的位置
#define labi_w				140
#define labi_h				lab2_h
#define labi_px				lab2_px
#define labi_py				lab2_py+lab2_h+5
//part2 label2
#define lab3_w				lab2_w
#define lab3_h				lab2_h
#define lab3_px				lab2_px
#define lab3_py				labi_py+55
//part2 entry1
#define entry1_w			140
#define entry1_h			lab3_h-3
#define entry1_px			lab3_px
#define entry1_py			lab3_py+lab3_h+3
//part2 radiobutton
#define rd1_px				entry1_px
#define rd1_py				entry1_py+entry1_h+5
#define rd2_px				rd1_px+70
#define rd2_py				rd1_py
//part2 progressbar
#define pro_w				entry1_w
#define pro_h				entry1_h-6
#define pro_px				entry1_px
#define pro_py				rd1_py+27
//part2 button
#define bnt3_w				120
#define bnt3_h				bnt2_h
#define bnt3_px				pro_px+10
#define bnt3_py				pro_py+pro_h+18
#define bnt4_w				bnt3_w
#define bnt4_h				bnt3_h
#define bnt4_px				bnt3_px
#define bnt4_py				bnt3_py+bnt3_h+18
#define bnt5_w				bnt4_w
#define bnt5_h				bnt4_h
#define bnt5_px				bnt4_px
#define bnt5_py				bnt4_py+bnt4_h+18
//part3 label
#define lab4_px				lab2_px+150
#define lab4_py				lab2_py
#define lab5_px				lab4_px+150
#define lab5_py				lab4_py
//part3 entry
#define entry2_w			140
#define entry2_h			20
#define entry2_px			lab4_px
#define entry2_py			labi_py
#define entry3_w			entry2_w
#define entry3_h			entry2_h
#define entry3_px			lab5_px
#define entry3_py			entry2_py
//part3 label
#define lab6_px				lab4_px
#define lab6_py				entry2_py+entry2_h+5
#define lab7_px				lab5_px
#define lab7_py				lab6_py
//part3 entry
#define entry4_w			entry3_w
#define entry4_h			entry3_h
#define entry4_px			entry2_px
#define entry4_py			lab6_py+27
#define entry5_w			entry4_w
#define entry5_h			entry4_h
#define entry5_px			lab7_px
#define entry5_py			entry4_py
//part3 button
#define bnt6_w				entry4_w
#define bnt6_h				bnt5_h
#define bnt6_px				entry4_px
#define bnt6_py				entry4_py+entry4_h+10
#define bnt7_w				bnt6_w
#define bnt7_h				bnt6_h
#define bnt7_px				entry5_px
#define bnt7_py				bnt6_py
//part3 treeview 185
#define t2_w				290
#define t2_h				160
#define t2_px				bnt6_px
#define t2_py				bnt6_py+bnt6_h+10
//part3 button
#define bnt8_w				bnt1_w
#define bnt8_h				bnt1_h
#define bnt8_px				t2_px+12
#define bnt8_py				t2_h+t2_py+10
#define bnt9_w				bnt8_w
#define bnt9_h				bnt8_h
#define bnt9_px				bnt8_px+bnt8_w+20
#define bnt9_py				bnt8_py
#define bnt10_w				bnt9_w
#define bnt10_h				bnt9_h
#define bnt10_px			bnt9_px+bnt9_w+20
#define bnt10_py			bnt9_py
//part3 treeview
#define t3_w				win_x-srl_w-20
#define t3_h				130
#define t3_px				t1_px
#define t3_py				bnt10_py+bnt10_h+10
//part3 scroll
#define srl2_w				srl_w
#define srl2_h				t3_h
#define srl2_px				t3_px+t3_w
#define srl2_py				t3_py


#define tip_statusicon		"修改大师V1.0\ntybitsfox\n2013-12-25"
#define iconfile			"/root/Desktop/20131113114643935_easyicon_net_48.png"
#define myphoto				"/root/Desktop/08888.jpg"

#define ps_cmd				"ps -Ao pid,fname >/tmp/uuuu_mod_tybitsfox.txt"
#define proc_file			"/tmp/uuuu_mod_tybitsfox.txt"

#define labi				"<span style=\"oblique\" color=\"red\" font=\"10\">%s</span>"
#define lab1				"<span style=\"oblique\" color=\"blue\" font=\"10\">获取的进程：</span>"
#define lab2				"<span style=\"oblique\" color=\"blue\" font=\"10\">当前指定的进程：</span>"
#define lab3				"<span style=\"oblique\" color=\"blue\" font=\"10\">待查询数据：</span>"
#define lab4				"<span style=\"oblique\" color=\"blue\" font=\"10\">锁定摘要：</span>"
#define lab5				"<span style=\"oblique\" color=\"blue\" font=\"10\">锁定地址：</span>"
#define lab6				"<span style=\"oblique\" color=\"blue\" font=\"10\">锁定下限：</span>"
#define lab7				"<span style=\"oblique\" color=\"blue\" font=\"10\">锁定上限：</span>"
#define rad0				"<span color=\"#ff00ff\" font=\"10\">十进制</span>"
#define rad1				"<span color=\"#ff00ff\" font=\"10\">十六进制</span>"
//}}}
//{{{
//主窗口的建立函数
void crt_window(int a,char **b);
//pixbuf的生成函数
GdkPixbuf *crt_pixbuf(gchar *fname);
//尝试建立treeview
void crt_treeview1();
//建立操作treeview1的两个按钮
void crt_tv1_bnt();
//建立第二列的控件
void crt_part2();
//建立第三列的控件
void crt_part3();
//按钮1 "取得进程"的响应函数
void on_getproc(GtkWidget *widget,gpointer gp);
//按钮2 "重置"的响应函数
void on_reset(GtkWidget *widget,gpointer pg);
//按钮3 "首次查找"的响应函数
void on_first_srh(GtkWidget *widget,gpointer gp);
//消息提示框
void msgbox(char *gc);
//树形列表1的双击响应函数
void on_tree1_dblclk(GtkTreeView *treeview,GtkTreePath *path,GtkTreeViewColumn *col,gpointer userdata);
//建立托盘
void crt_statusicon();
//托盘菜单中：恢复窗口的响应函数。
void restore_window(GtkWidget *widget,gpointer gp);
//托盘菜单中：关于窗口的响应函数
void about_window(GtkWidget *widget,gpointer gp);
//托盘菜单中：退出的响应函数
void exit_window(GtkWidget *widget,gpointer gp);
//托盘图标的右键菜单弹出响应函数
void show_menu(GtkWidget *widget,guint button,guint32 act_time,gpointer gp);
//主窗口隐藏，切换至托盘形式
gint hide_window(GtkWidget *widget,GdkEvent *event,gpointer gp);
//首次查找的按钮响应函数
void on_first_srh(GtkWidget *widget,gpointer gp);
//检查输入的查询关键字函数
int check_input(char *c);
//首次查找的线程函数
gpointer thd_fst(gpointer pt);
//一个毫秒级的定时器
void msleep();
//再次查询按钮的响应函数。
void on_next_srh(GtkWidget *widget,gpointer gp);
//再次查找的线程函数
gpointer thd_next(gpointer pt);
//treeview3的双击响应函数
void on_tree3_dblclk(GtkTreeView *treeview,GtkTreePath *path,GtkTreeViewColumn *col,gpointer userdata);
//测试用，保存首次查询的地址集：
void on_save(GtkWidget *widget,gpointer gp);
//锁定指定地址数据的线程。
gpointer thd_thr(gpointer pt);
//按钮"添加锁定"的响应函数，测试取得treeview的记录条数
void on_addlock(GtkWidget *widget,gpointer gp);
//按钮"删除锁定"的响应函数，测试取得记录的内容
void on_dellock(GtkWidget *widget,gpointer gp);
//锁定信息的输入检查函数。
int check_lock(char *c);
//开始锁定
void on_lock(GtkWidget *widget,gpointer gp);
//地址转换
int tonum(char *c,unsigned int *ui);


//}}}


