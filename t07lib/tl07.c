#include"tl07.h"

//{{{ void crt_window(int a,char **b)
void crt_window(int a,char **b)
{
	GtkWidget *item;
	GtkTreeIter iter;
	int i,j,k;
	char ch[36];
	if(!g_thread_supported())
		g_thread_init(NULL);
	gdk_threads_init();
	gtk_init(&a,&b);
	ws.window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(ws.window),wintitle);
	gtk_widget_set_size_request(ws.window,win_x,win_y);
	gtk_window_set_resizable(GTK_WINDOW(ws.window),FALSE);
	gtk_window_set_position(GTK_WINDOW(ws.window),GTK_WIN_POS_CENTER);
	gtk_window_set_icon(GTK_WINDOW(ws.window),crt_pixbuf(iconfile));
	ws.fixed=gtk_fixed_new();
	gtk_container_add(GTK_CONTAINER(ws.window),ws.fixed);
	crt_treeview1();
	crt_tv1_bnt();
	crt_part2();
	crt_part3();
	memset(ch,0,36);
	//gtk_list_store_append(ws.store[1],&iter);
	//gtk_list_store_set(ws.store[1],&iter,0,"生   命   值",1,"0x884893823",2,155,3,245,-1);
	//gtk_list_store_append(ws.store[1],&iter);
	//gtk_list_store_set(ws.store[1],&iter,0,"武器1",1,"0x884920230",2,16,-1);
	crt_statusicon();
	gtk_widget_show_all(ws.window);
	g_signal_connect(G_OBJECT(ws.window),"delete-event",G_CALLBACK(hide_window),NULL);
//	g_signal_connect_swapped(G_OBJECT(ws.window),"destroy",G_CALLBACK(gtk_main_quit),NULL);
	ws.thread_lock=0;ws.pid=0;ws.sn=0;
	for(i=0;i<8;i++)
		memset((void*)&sl[i],0,sizeof(sl[i]));
	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();
}//}}}
//{{{ GdkPixbuf *crt_pixbuf(gchar *fname)
GdkPixbuf *crt_pixbuf(gchar *fname)
{
	GdkPixbuf *pix;
	GError *error=NULL;
	pix=gdk_pixbuf_new_from_file(fname,&error);
	if(!pix)
	{
		g_print("%s\n",error->message);
		g_error_free(error);
	}
	return pix;
}//}}}
//{{{ void crt_treeview1()
void crt_treeview1()
{
	GtkTreeModel *model;
	GtkTreeViewColumn *column;
	GtkCellRenderer	*render;
//把label1的建立代码移至该函数中，该函数建立的三个控件：label1,treeview,scroll1	
	ws.label=gtk_label_new("");
	//gtk_widget_set_size_request(ws.label,lab1_w,lab1_h);
	gtk_label_set_markup(GTK_LABEL(ws.label),lab1);
	ws.store[0]=gtk_list_store_new(2,G_TYPE_INT,G_TYPE_STRING);
	model=GTK_TREE_MODEL(ws.store[0]);
	ws.list[0]=gtk_tree_view_new_with_model(model);
	render=gtk_cell_renderer_text_new();
	column=gtk_tree_view_column_new_with_attributes("PID",render,"text",0,NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ws.list[0]),column);
	render=gtk_cell_renderer_text_new();
//	g_object_set(G_OBJECT(render),"editable",TRUE);  //可编辑
	column=gtk_tree_view_column_new_with_attributes("进程",render,"text",1,NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ws.list[0]),column);
	gtk_widget_set_size_request(ws.list[0],t1_w,t1_h);
	ws.scroll[0]=gtk_vscrollbar_new(gtk_tree_view_get_vadjustment(GTK_TREE_VIEW(ws.list[0])));
	gtk_widget_set_size_request(ws.scroll[0],srl_w,srl_h);
	gtk_fixed_put(GTK_FIXED(ws.fixed),ws.label,lab1_px,lab1_py);
	gtk_fixed_put(GTK_FIXED(ws.fixed),ws.list[0],t1_px,t1_py);
	gtk_fixed_put(GTK_FIXED(ws.fixed),ws.scroll[0],srl_px,srl_py);
	g_signal_connect(G_OBJECT(ws.list[0]),"row-activated",G_CALLBACK(on_tree1_dblclk),NULL);
}//}}}
//{{{ void crt_tv1_bnt()
void crt_tv1_bnt()
{
	ws.bnt[0]=gtk_button_new_with_label("取得进程");
	gtk_widget_set_size_request(ws.bnt[0],bnt1_w,bnt1_h);
	gtk_fixed_put(GTK_FIXED(ws.fixed),ws.bnt[0],bnt1_px,bnt1_py);
	ws.bnt[1]=gtk_button_new_with_label("重    置");
	gtk_widget_set_size_request(ws.bnt[1],bnt2_w,bnt2_h);
	gtk_fixed_put(GTK_FIXED(ws.fixed),ws.bnt[1],bnt2_px,bnt2_py);
	g_signal_connect(G_OBJECT(ws.bnt[0]),"clicked",G_CALLBACK(on_getproc),NULL);
	g_signal_connect(G_OBJECT(ws.bnt[1]),"clicked",G_CALLBACK(on_reset),NULL);
}//}}}
//{{{ void crt_part2()
void crt_part2()
{
	GtkWidget *label;
	GSList *l;
	label=gtk_label_new("");
	//gtk_widget_set_size_request(label,lab2_w,lab2_h);
	gtk_label_set_markup(GTK_LABEL(label),lab2);
	gtk_fixed_put(GTK_FIXED(ws.fixed),label,lab2_px,lab2_py);
	ws.label=gtk_label_new("");//从现在起，该指针将保留用于输入label的使用
	//gtk_widget_set_size_request(ws.label,labi_w,labi_h);
	//gtk_label_set_markup(GTK_LABEL(ws.label),lab5);
	gtk_fixed_put(GTK_FIXED(ws.fixed),ws.label,labi_px,labi_py);
	label=gtk_label_new("");
	//gtk_widget_set_size_request(label,lab3_w,lab3_h);
	gtk_label_set_markup(GTK_LABEL(label),lab3);
	gtk_fixed_put(GTK_FIXED(ws.fixed),label,lab3_px,lab3_py);
	ws.entry[0]=gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(ws.entry[0]),60);
	gtk_widget_set_size_request(ws.entry[0],entry1_w,entry1_h);
	gtk_fixed_put(GTK_FIXED(ws.fixed),ws.entry[0],entry1_px,entry1_py);
	ws.radio[0]=gtk_radio_button_new(NULL);
	label=gtk_label_new("");
	gtk_label_set_markup(GTK_LABEL(label),rad0);
	gtk_container_add(GTK_CONTAINER(ws.radio[0]),label);
	l=gtk_radio_button_get_group(GTK_RADIO_BUTTON(ws.radio[0]));
	ws.radio[1]=gtk_radio_button_new(l);
	label=gtk_label_new("");
	gtk_label_set_markup(GTK_LABEL(label),rad1);
	gtk_container_add(GTK_CONTAINER(ws.radio[1]),label);
	gtk_fixed_put(GTK_FIXED(ws.fixed),ws.radio[0],rd1_px,rd1_py);
	gtk_fixed_put(GTK_FIXED(ws.fixed),ws.radio[1],rd2_px,rd2_py);
	ws.prog=gtk_progress_bar_new();
	gtk_widget_set_size_request(ws.prog,pro_w,pro_h);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(ws.prog),"进度%");
	gtk_fixed_put(GTK_FIXED(ws.fixed),ws.prog,pro_px,pro_py);
	ws.bnt[2]=gtk_button_new_with_label("首次查找");
	gtk_widget_set_size_request(ws.bnt[2],bnt3_w,bnt3_h);
	gtk_fixed_put(GTK_FIXED(ws.fixed),ws.bnt[2],bnt3_px,bnt3_py);
	ws.bnt[3]=gtk_button_new_with_label("再次查找");
	gtk_widget_set_size_request(ws.bnt[3],bnt4_w,bnt4_h);
	gtk_fixed_put(GTK_FIXED(ws.fixed),ws.bnt[3],bnt4_px,bnt4_py);
	ws.bnt[4]=gtk_button_new_with_label("保存至文件");
	gtk_widget_set_size_request(ws.bnt[4],bnt5_w,bnt5_h);
	gtk_fixed_put(GTK_FIXED(ws.fixed),ws.bnt[4],bnt5_px,bnt5_py);
//添加按钮的消息响应函数
	g_signal_connect(G_OBJECT(ws.bnt[2]),"clicked",G_CALLBACK(on_first_srh),NULL);
	g_signal_connect(G_OBJECT(ws.bnt[3]),"clicked",G_CALLBACK(on_next_srh),NULL);
	g_signal_connect(G_OBJECT(ws.bnt[4]),"clicked",G_CALLBACK(on_save),NULL);
}//}}}
//{{{ void crt_part3()
void crt_part3()
{
	GtkTreeModel *model;
	GtkTreeViewColumn *column;
	GtkCellRenderer	*render;
	GtkWidget *label;
	label=gtk_label_new("");
	gtk_label_set_markup(GTK_LABEL(label),lab4);
	gtk_fixed_put(GTK_FIXED(ws.fixed),label,lab4_px,lab4_py);
	label=gtk_label_new("");
	gtk_label_set_markup(GTK_LABEL(label),lab5);
	gtk_fixed_put(GTK_FIXED(ws.fixed),label,lab5_px,lab5_py);
	ws.entry[1]=gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(ws.entry[1]),60);
	gtk_widget_set_size_request(ws.entry[1],entry2_w,entry2_h);
	gtk_fixed_put(GTK_FIXED(ws.fixed),ws.entry[1],entry2_px,entry2_py);
	ws.entry[2]=gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(ws.entry[2]),60);
	gtk_widget_set_size_request(ws.entry[2],entry3_w,entry3_h);
	gtk_fixed_put(GTK_FIXED(ws.fixed),ws.entry[2],entry3_px,entry3_py);
	label=gtk_label_new("");
	gtk_label_set_markup(GTK_LABEL(label),lab6);
	gtk_fixed_put(GTK_FIXED(ws.fixed),label,lab6_px,lab6_py);
	label=gtk_label_new("");
	gtk_label_set_markup(GTK_LABEL(label),lab7);
	gtk_fixed_put(GTK_FIXED(ws.fixed),label,lab7_px,lab7_py);
	ws.entry[3]=gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(ws.entry[3]),60);
	gtk_widget_set_size_request(ws.entry[3],entry4_w,entry4_h);
	gtk_fixed_put(GTK_FIXED(ws.fixed),ws.entry[3],entry4_px,entry4_py);
	ws.entry[4]=gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(ws.entry[4]),60);
	gtk_widget_set_size_request(ws.entry[4],entry5_w,entry5_h);
	gtk_fixed_put(GTK_FIXED(ws.fixed),ws.entry[4],entry5_px,entry5_py);
	ws.bnt[5]=gtk_button_new_with_label("添加锁定");
	gtk_widget_set_size_request(ws.bnt[5],bnt6_w,bnt6_h);
	gtk_fixed_put(GTK_FIXED(ws.fixed),ws.bnt[5],bnt6_px,bnt6_py);
	ws.bnt[6]=gtk_button_new_with_label("删除锁定");
	gtk_widget_set_size_request(ws.bnt[6],bnt7_w,bnt7_h);
	gtk_fixed_put(GTK_FIXED(ws.fixed),ws.bnt[6],bnt7_px,bnt7_py);
	ws.store[1]=gtk_list_store_new(4,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_INT,G_TYPE_INT);
	model=GTK_TREE_MODEL(ws.store[1]);
	ws.list[1]=gtk_tree_view_new_with_model(model);
	render=gtk_cell_renderer_text_new();
	column=gtk_tree_view_column_new_with_attributes("摘  要",render,"text",0,NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ws.list[1]),column);
	render=gtk_cell_renderer_text_new();
	column=gtk_tree_view_column_new_with_attributes("锁定地址",render,"text",1,NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ws.list[1]),column);
	render=gtk_cell_renderer_text_new();
	column=gtk_tree_view_column_new_with_attributes("下限",render,"text",2,NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ws.list[1]),column);
	render=gtk_cell_renderer_text_new();
	column=gtk_tree_view_column_new_with_attributes("上限",render,"text",3,NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ws.list[1]),column);
	gtk_widget_set_size_request(ws.list[1],t2_w,t2_h);
	gtk_fixed_put(GTK_FIXED(ws.fixed),ws.list[1],t2_px,t2_py);
	ws.bnt[7]=gtk_button_new_with_label("保存锁定");
	gtk_widget_set_size_request(ws.bnt[7],bnt8_w,bnt8_h);
	gtk_fixed_put(GTK_FIXED(ws.fixed),ws.bnt[7],bnt8_px,bnt8_py);
	ws.bnt[8]=gtk_button_new_with_label("锁    定");
	gtk_widget_set_size_request(ws.bnt[8],bnt9_w,bnt9_h);
	gtk_fixed_put(GTK_FIXED(ws.fixed),ws.bnt[8],bnt9_px,bnt9_py);
	ws.bnt[9]=gtk_button_new_with_label("装载锁定");
	gtk_widget_set_size_request(ws.bnt[9],bnt10_w,bnt10_h);
	gtk_fixed_put(GTK_FIXED(ws.fixed),ws.bnt[9],bnt10_px,bnt10_py);
	ws.store[2]=gtk_list_store_new(2,G_TYPE_INT,G_TYPE_STRING);
	model=GTK_TREE_MODEL(ws.store[2]);
	ws.list[2]=gtk_tree_view_new_with_model(model);
	render=gtk_cell_renderer_text_new();
	column=gtk_tree_view_column_new_with_attributes("序    号",render,"text",0,NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ws.list[2]),column);
	render=gtk_cell_renderer_text_new();
	column=gtk_tree_view_column_new_with_attributes("扫描地址",render,"text",1,NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ws.list[2]),column);
	gtk_widget_set_size_request(ws.list[2],t3_w,t3_h);
	gtk_fixed_put(GTK_FIXED(ws.fixed),ws.list[2],t3_px,t3_py);
	ws.scroll[1]=gtk_vscrollbar_new(gtk_tree_view_get_vadjustment(GTK_TREE_VIEW(ws.list[2])));
	gtk_widget_set_size_request(ws.scroll[1],srl2_w,srl2_h);
	gtk_fixed_put(GTK_FIXED(ws.fixed),ws.scroll[1],srl2_px,srl2_py);
	//消息响应
	g_signal_connect(G_OBJECT(ws.bnt[5]),"clicked",G_CALLBACK(on_addlock),NULL);
	g_signal_connect(G_OBJECT(ws.bnt[6]),"clicked",G_CALLBACK(on_dellock),NULL);
	g_signal_connect(G_OBJECT(ws.bnt[8]),"clicked",G_CALLBACK(on_lock),NULL);
	g_signal_connect(G_OBJECT(ws.list[2]),"row-activated",G_CALLBACK(on_tree3_dblclk),NULL);


}//}}}
//{{{ void on_getproc(GtkWidget *widget,gpointer gp)
void on_getproc(GtkWidget *widget,gpointer gp)
{
	FILE *file;
	int i,j;
	char ch[128],c[128];
	GtkTreeIter iter;
	//先调用清空函数
	gtk_list_store_clear(ws.store[0]);
	system(ps_cmd);
	file=fopen(proc_file,"r");
	if(file==NULL)
	{
		msgbox("打开文件失败！");
		return;
	}
	memset(ch,0,sizeof(ch));i=0;
	while(fgets(ch,sizeof(ch),file)!=NULL)
	{
		if(i==0)
		{//分割空格都是对齐的，这里确定分割空格的位置：
			for(i=0;i<strlen(ch);i++)
			{
				if(ch[i]=='D')
				{i++;break;}
			}//i保存了空格的位置
			memset(ch,0,sizeof(ch));
			continue;
		}
		memset(c,0,sizeof(c));
		memcpy(c,ch,i);
		j=atoi(c);//得到pid
		gtk_list_store_append(ws.store[0],&iter);
		gtk_list_store_set(ws.store[0],&iter,0,j,1,&ch[i+1],-1);
		memset(ch,0,sizeof(ch));	
	}
	fclose(file);
}//}}}
//{{{ void on_reset(GtkWidget *widget,gpointer pg)
void on_reset(GtkWidget *widget,gpointer pg)
{
	gtk_list_store_clear(ws.store[0]);
}//}}}
//{{{ void msgbox(char *gc)
void msgbox(char *gc)
{
	GtkWidget *dialog;
	GtkMessageType type;
	type=GTK_MESSAGE_INFO;
	dialog=gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,type,GTK_BUTTONS_OK,(gchar *)gc);
	gtk_window_set_position(GTK_WINDOW(dialog),GTK_WIN_POS_CENTER);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}//}}}
//{{{ void on_tree1_dblclk(GtkTreeView *treeview,GtkTreePath *path,GtkTreeViewColumn *col,gpointer userdata)
void on_tree1_dblclk(GtkTreeView *treeview,GtkTreePath *path,GtkTreeViewColumn *col,gpointer userdata)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GValue value,va;
	char ch[200];
	memset((void*)&value,0,sizeof(value));
	model=gtk_tree_view_get_model(treeview);
	if(gtk_tree_model_get_iter(model,&iter,path))
	{
		gtk_tree_model_get_value(model,&iter,0,&value);
		ws.pid=g_value_get_int(&value);//get pid
		g_value_unset(&value);
		gtk_tree_model_get_value(model,&iter,1,&value);
		memset(ch,0,sizeof(ch));
		snprintf(ch,sizeof(ch),labi,g_value_get_string(&value));
		gtk_label_set_markup(GTK_LABEL(ws.label),ch);
	}
}//}}}
//{{{ void crt_statusicon()
void crt_statusicon()
{
	ws.sicon=gtk_status_icon_new_from_file(iconfile);
	ws.menu=gtk_menu_new();
	ws.menu_restore=gtk_menu_item_new_with_label("恢复窗口");
	ws.menu_set=gtk_menu_item_new_with_label("关    于");
	ws.menu_exit=gtk_menu_item_new_with_label("退    出");
	gtk_menu_shell_append(GTK_MENU_SHELL(ws.menu),ws.menu_restore);
	gtk_menu_shell_append(GTK_MENU_SHELL(ws.menu),ws.menu_set);
	gtk_menu_shell_append(GTK_MENU_SHELL(ws.menu),ws.menu_exit);
	g_signal_connect(G_OBJECT(ws.menu_restore),"activate",G_CALLBACK(restore_window),NULL);
	g_signal_connect(G_OBJECT(ws.menu_set),"activate",G_CALLBACK(about_window),NULL);
	g_signal_connect(G_OBJECT(ws.menu_exit),"activate",G_CALLBACK(exit_window),NULL);
	gtk_widget_show_all(ws.menu);
	gtk_status_icon_set_tooltip(ws.sicon,tip_statusicon);
	g_signal_connect(GTK_STATUS_ICON(ws.sicon),"activate",GTK_SIGNAL_FUNC(restore_window),NULL);
	g_signal_connect(GTK_STATUS_ICON(ws.sicon),"popup_menu",GTK_SIGNAL_FUNC(show_menu),NULL);
	gtk_status_icon_set_visible(ws.sicon,FALSE);
}//}}}
//{{{ void restore_window(GtkWidget *widget,gpointer gp)
void restore_window(GtkWidget *widget,gpointer gp)
{
	gtk_widget_show(ws.window);
	gtk_window_deiconify(GTK_WINDOW(ws.window));
	gtk_status_icon_set_visible(ws.sicon,FALSE);
}//}}}
//{{{ void about_window(GtkWidget *widget,gpointer gp)
void about_window(GtkWidget *widget,gpointer gp)
{
//	msgbox("hello world");
	char ch[512];
	GtkWidget *dlg,*fixed,*label,*img;
	dlg=gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dlg),"关于本软件");
	gtk_window_set_position(GTK_WINDOW(dlg),GTK_WIN_POS_CENTER);
	gtk_widget_set_size_request(dlg,450,300);
	gtk_window_set_resizable(GTK_WINDOW(dlg),FALSE);
	fixed=gtk_fixed_new();
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox),fixed,FALSE,FALSE,0);
	gtk_container_set_border_width(GTK_CONTAINER(dlg),2);
	img=gtk_image_new_from_file(myphoto);
	gtk_widget_set_size_request(img,99,110);
	gtk_fixed_put(GTK_FIXED(fixed),img,5,5);
	label=gtk_label_new("");
	memset(ch,0,sizeof(ch));
	snprintf(ch,sizeof(ch),"<span style=\"italic\" face=\"YaHei Consolas Hybrid\" font=\"16\" color=\"#ff00ff\">程式设计：tybitsfox</span>");
	gtk_label_set_markup(GTK_LABEL(label),ch);
	gtk_widget_set_size_request(label,300,30);
	gtk_fixed_put(GTK_FIXED(fixed),label,120,40);
	gtk_widget_show_all(dlg);
	gtk_dialog_run(GTK_DIALOG(dlg));
	gtk_widget_destroy(dlg);
}//}}}
//{{{ void exit_window(GtkWidget *widget,gpointer gp)
void exit_window(GtkWidget *widget,gpointer gp)
{
	gtk_main_quit();
}//}}}
//{{{ void show_menu(GtkWidget *widget,guint button,guint32 act_time,gpointer gp)
void show_menu(GtkWidget *widget,guint button,guint32 act_time,gpointer gp)
{
	gtk_menu_popup(GTK_MENU(ws.menu),NULL,NULL,gtk_status_icon_position_menu,widget,button,act_time);

}//}}}
//{{{ gint hide_window(GtkWidget *widget,GdkEvent *event,gpointer gp)
gint hide_window(GtkWidget *widget,GdkEvent *event,gpointer gp)
{
	gtk_widget_hide(ws.window);
	gtk_status_icon_set_visible(ws.sicon,TRUE);
	return TRUE;
}//}}}
//{{{ void on_first_srh(GtkWidget *widget,gpointer gp)
void on_first_srh(GtkWidget *widget,gpointer gp)
{
	char *p;
	if(ws.thread_lock==1)
	{
		msgbox("查询中....请稍后操作");
		return;
	}
	if(ws.pid<1)
	{
		msgbox("请先选择待查询的目标进程");
		return;
	}
	p=(char*)gtk_entry_get_text(GTK_ENTRY(ws.entry[0]));
	if(check_input(p))
	{
		msgbox("输入的查询数据有误");
		return;
	}
//	thd_fst(0);
	g_thread_create(thd_fst,NULL,FALSE,NULL);
//	p=(char*)gtk_label_get_text(GTK_LABEL(ws.label));


}//}}}
//{{{ int check_input(char *c)
int check_input(char *c)
{
	int i,j,k;
	char ch[20];
	j=strlen(c);
	if(j>10)
		return 1;
	memset(ch,0,sizeof(ch));
	if(ws.sn==0)
		k=0;
	else
	{k=ws.sn;ws.sn=0;}
//取得数据的进制类型
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ws.radio[0])))
	{//十进制
		for(i=0;i<j;i++)
		{
			if(c[i]>0x39 ||c[i]<0x30) //no number
			{
				ws.sn=0;
				return 1;
			}
			ws.sn*=10;
			ws.sn+=c[i];ws.sn-=0x30;
		}
	}
	else
	{//十六进制
		for(i=0;i<j;i++)
		{
			ch[i]=toupper(c[i]);
			if(ch[i]>=0x30 && ch[i]<=0x39)
			{
				ws.sn*=16;ws.sn+=ch[i];
				ws.sn-=0x30;
			}
			else
			{
				if(ch[i]>='A' && ch[i]<='F')
				{
					ws.sn*=16;ws.sn+=ch[i];
					ws.sn-=0x37;
				}
				else
				{
					ws.sn=0;return 1;
				}
			}
		}
	}
	if(k==ws.sn)//与上次的查询数据一样？提示下用户
	{
		msgbox("本次输入的查询数据与前次查询相同？？");
	}
	return 0;
}//}}}
//{{{ gpointer thd_fst(gpointer pt)
gpointer thd_fst(gpointer pt)
{
	int i,j,k,l,m,fd;
	char ch[20],*c;
	struct KVAR_AM *k_am;
	struct KVAR_SAD *ksa;
	GtkTreeIter iter;
	GtkListStore *store;
	ws.thread_lock=1;//锁定
//	g_print("aaaaaaaa\n");
	k_am=(struct KVAR_AM *)ws.g_ch;
	ksa=(struct KVAR_SAD*)&(ws.g_ch[d_begin]);
	memset(ws.g_ch,0,buf_size);
	k_am->cmd=1;					//首次查询命令字
	k_am->pid=ws.pid;			//目标pid 
	k_am->snum=ws.sn;			//查询关键字
	fd=open(drv_name,O_RDWR);
	if(fd<0)
	{
		gdk_threads_enter();
		msgbox("目标模块连接失败");
		gdk_threads_leave();
		goto thd_01;
	}
	i=write(fd,ws.g_ch,buf_size);//发送命令
	if(i!=buf_size)
	{
		gdk_threads_enter();
		msgbox("首次查询指令发送失败");
		gdk_threads_leave();
		goto thd_01;
	}
	msleep();
	//开始进入轮寻等待
	//g_timeout_add(300,(GSourceFunc),(gpointer)fd); //1second=1000
	memset(ws.g_ch,0,buf_size);
	c=ws.g_addr[0];k=0;
	memset(c,0,adr_buf1);k=0;
	/*while(1)
	{
		i=read(fd,ws.g_ch,buf_size);
		if(ws.g_ch[0]==1)
		{
			msleep();
			ws.g_ch[0]=0;
			write(fd,ws.g_ch,buf_size);
		}
		else
		{
			g_print("idle....\n");
			write(fd,ws.g_ch,buf_size);
		}
		msleep();
		if(ws.g_ch[7]==1)
			break;
		k++;
		if(k>10)
			break;
	}
	goto thd_01;*/
	gdk_threads_enter();
	gtk_list_store_clear(ws.store[2]);//先清空列表
	gdk_threads_leave();m=0;
	while(1)
	{
//f_002:	
		i=read(fd,ws.g_ch,buf_size);
		if(i==buf_size)//有数据返回
		{
			if(k_am->sync!=1)
			{
				msleep();
//				msgbox("不可能的错误。");
				//break;
//				goto f_002;
				g_print("idle......\n");
				continue;
			}
			//开始读取数据并显示。
			ksa=(struct KVAR_SAD*)&(ws.g_ch[d_begin]);
			for(l=0;l<2000;l++)//最多2000个地址
			{
				if(ksa->spg==0 && ksa->off==0)
					break;
				if(ksa->s_b.sbit==0)//code
					j=(int)k_am->text_seg;
				else
					j=(int)k_am->data_seg;
				i=(int)ksa->s_b.pbit;i*=4096;
				j+=i;j+=(int)ksa->off;
				memset(ch,0,sizeof(ch));
				snprintf(ch,sizeof(ch),"0x%lx",j);
				i=ksa->s_b.pbit;
				gdk_threads_enter();
				store = GTK_LIST_STORE(gtk_tree_view_get_model
						      (GTK_TREE_VIEW(ws.list[2])));
				gtk_list_store_append(store,&iter);
				gtk_list_store_set(store,&iter,0,m,1,ch,-1);
				gdk_threads_leave();
				ksa++;m++;
			}
			memset(ch,0,sizeof(ch));
			snprintf(ch,sizeof(ch),"%d\n",k);
			g_print(ch);
			memcpy((void*)c,(void*)&(ws.g_ch[d_begin]),d_len);
			c+=d_len;k++;
			if(k_am->end0==1)//全部结束
			{
				k_am->sync=0;k_am->end0=1;//确定
				i=write(fd,ws.g_ch,buf_size);
				g_print("first finished!\n");
				//if(i<0)
				//	msgbox("向内核发送指令失败11");
				//else
				//	msgbox("全部查询完成");
				goto thd_01;
			}
			//给内核模块发送完成标志。
			memset((void*)&(ws.g_ch[d_begin]),0,d_len);
			k_am->sync=0;//k_am->end0=1;
			i=write(fd,ws.g_ch,buf_size);
			if(i<0)
			{				
				//msleep();
				//i=write(fd,ws.g_ch,buf_size);
				gdk_threads_enter();
				msgbox("向内核发送指令失败");
				gdk_threads_leave();
				goto thd_01;
			}
			msleep();
			/*if(k>=30)//缓冲区g_addr[0]已满  --这种可能在内核中已经作出了判断，这里可以不用考虑
			{
				msgbox("获取的地址已超过6万～");
				goto thd_01;
			}*/
		}
		msleep();
	}
thd_01:
	close(fd);
	ws.dseg=k_am->data_seg;
	ws.bseg=k_am->text_seg;
	ws.thread_lock=0;
	msleep();
	return 0;	
}//}}}
//{{{ void msleep()
void msleep()
{
  tm.tv_sec=0;
  tm.tv_usec=300000;	//300毫秒
  select(0,NULL,NULL,NULL,&tm);
}//}}}
//{{{ void on_next_srh(GtkWidget *widget,gpointer gp)
void on_next_srh(GtkWidget *widget,gpointer gp)
{
    GtkListStore *store;
    GtkTreeModel *model;
    GtkTreeIter iter;
	char *p;
	if(ws.thread_lock==1)
	{
		msgbox("查询中....请稍后操作");
		return;
	}
	if(ws.pid<1)
	{
		msgbox("请先选择待查询的目标进程");
		return;
	}
	//查看treeview3中是否有记录？没有记录则表示还没有执行首次查找
    store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(ws.list[2])));
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(ws.list[2]));
    if (gtk_tree_model_get_iter_first(model, &iter) == FALSE)
	{
		msgbox("请先进行\"首次查找\"!");
	 	return;
	}
    gtk_list_store_clear(store);//有记录则全部清空。
	p=(char*)gtk_entry_get_text(GTK_ENTRY(ws.entry[0]));
	if(check_input(p))
	{
		msgbox("输入的查询数据有误");
		return;
	}
	g_thread_create(thd_next,NULL,FALSE,NULL);
}//}}}
//{{{ gpointer thd_next(gpointer pt)
gpointer thd_next(gpointer pt)
{
	int i,j,k,l,m,fd,count;
	char ch[20],*c,*p,*dst;
	struct KVAR_AM *k_am;
	struct KVAR_SAD *ksa;
	GtkTreeIter iter;
	GtkListStore *store;
	ws.thread_lock=1;//锁定
	k_am=(struct KVAR_AM *)ws.g_ch;
	ksa=(struct KVAR_SAD*)&(ws.g_ch[d_begin]);
	memset(ws.g_ch,0,buf_size);
	k_am->cmd=2;					//再次查询命令字
	k_am->pid=ws.pid;			//目标pid 
	k_am->snum=ws.sn;			//查询关键字
	c=(char*)&(ws.g_ch[d_begin]);//c指向输入输出缓冲区的起始位置
	p=ws.g_addr[0];//p指向首次查询地址集的起始位置
	dst=ws.g_addr[1];//dst指向本次查询的结果的存储位置。改位置为临时存储。在查询完成后
	//应将本次查询的地址集再拷贝至p指向的位置。
	memset(dst,0,adr_buf2);
	//每次拷贝2000地址至输入输出缓冲区：
	memset(c,0,d_len);
	memcpy(c,p,d_len);
	p+=d_len;count=1;//地址集的数量记录。
	fd=open(drv_name,O_RDWR);
	if(fd<0)
	{
		gdk_threads_enter();
		msgbox("目标模块连接失败");
		gdk_threads_leave();
		goto thd_03;
	}
	i=write(fd,ws.g_ch,buf_size);//发送命令
	if(i!=buf_size)
	{
		gdk_threads_enter();
		msgbox("首次查询指令发送失败");
		gdk_threads_leave();
		goto thd_02;
	}
	msleep();k=0;m=0;
	while(1)
	{
		i=read(fd,ws.g_ch,buf_size);
		if(i==buf_size)//有数据返回
		{
			if(k_am->sync!=1)//内核正在查询中...轮寻等待
			{
				msleep();
				continue;
			}
			//读出有效数据，需要分析是传出的结果地址还是需要向内核传送地址集？
			if(k_am->vv0[0]==0)//传出的结果。此时按照模块的设计，可能产生完全退出。
			{
				g_print("oooooooooooooo\n");
				if(k>4)//超过了备用缓冲区ws.g_addr[1]的大小
				{//此时可能还有数据，但是不能保存了
					k_am->sync=0;
					msleep();
					j=write(fd,ws.g_ch,buf_size);
					if(k_am->end0==1)
						break;//goto tnext_01;
					else
						continue;
				}
				//添加treeview列表的显示代码
				ksa=(struct KVAR_SAD*)&(ws.g_ch[d_begin]);
				for(l=0;l<2000;l++)//最多2000个地址
				{
					if(ksa->spg==0 && ksa->off==0)
					{
						memset(ch,0,sizeof(ch));
						snprintf(ch,sizeof(ch),"%d times\n",l);
						g_print(ch);
						break;
					}
					if(ksa->s_b.sbit==0)//code
						j=(int)k_am->text_seg;
					else
						j=(int)k_am->data_seg;
					i=(int)ksa->s_b.pbit;i*=4096;
					j+=i;j+=(int)ksa->off;
					memset(ch,0,sizeof(ch));
					snprintf(ch,sizeof(ch),"0x%lx",j);
					i=ksa->s_b.pbit;
					gdk_threads_enter();
					store = GTK_LIST_STORE(gtk_tree_view_get_model
							(GTK_TREE_VIEW(ws.list[2])));
					gtk_list_store_append(store,&iter);
					gtk_list_store_set(store,&iter,0,m,1,ch,-1);
					gdk_threads_leave();
					ksa++;m++;
				}
				memcpy((void*)dst,(void*)&(ws.g_ch[d_begin]),d_len);
				dst+=d_len;k++;
				k_am->sync=0;
				msleep();
				j=write(fd,ws.g_ch,buf_size);
				if(k_am->end0==1)//完全退出
					break;//goto tnext_01;
			}
			else//需要向内核传送地址集，对于最后一块地址集，则要设置结束标志
			{
				memset(c,0,d_len);
				memcpy(c,p,d_len);
				p+=d_len;count++;
				if(count>29)//全部地址集都读出了
					k_am->end0=1;//结束标志。
				else
				{
					memcpy((void*)&j,p,sizeof(int));
					if(j==0)//下一页为空了
						k_am->end0=1;
					else
						k_am->end0=0;
				}
				k_am->sync=0;
				write(fd,ws.g_ch,buf_size);
				msleep();
				memset(ch,0,sizeof(ch));
				snprintf(ch,sizeof(ch),"%d %d\n",count,k_am->end0);
				g_print(ch);
			}
		}
		msleep();
	}
	p=ws.g_addr[0];//p指向首次查询地址集的起始位置
	dst=ws.g_addr[1];//dst指向本次查询的结果的存储位置。改位置为临时存储。在查询完成后
	memset(p,0,adr_buf1);
	memcpy(p,dst,adr_buf2);//最终查询结果必须存储在ws.g_addr[0]中。
thd_02:
	close(fd);
	msleep();
thd_03:	
	ws.thread_lock=0;
	g_print("thread exit!\n");
	return 0;
}//}}}
//{{{ void on_save(GtkWidget *widget,gpointer gp)
void on_save(GtkWidget *widget,gpointer gp)
{
	msgbox("测试版本，暂时无法保存锁定地址");
	/*int i,fd;
	struct KVAR_AM *k_am;
	k_am=(struct KVAR_AM *)ws.g_ch;
	memset(ws.g_ch,0,buf_size);
	k_am->cmd=5;					//首次查询命令字
	k_am->pid=ws.pid;			//目标pid 
	k_am->snum=ws.sn;			//查询关键字
	fd=open(drv_name,O_RDWR);
	if(fd<0)
	{
		gdk_threads_enter();
		msgbox("目标模块连接失败");
		gdk_threads_leave();
		return;
	}
	i=write(fd,ws.g_ch,buf_size);//发送命令
	msleep();
	close(fd);
	return;
	//g_thread_create(thd_thr,NULL,FALSE,NULL);
	int fd,i;
	fd=open("addr.txt",O_RDWR|O_CREAT);
	if(fd<1)
	{
		msgbox("crate file error");
		return;
	}
	i=write(fd,ws.g_ch,adr_buf1);
	close(fd);
	msgbox("write success!");*/
}//}}}
//{{{ gpointer thd_thr(gpointer pt)
gpointer thd_thr(gpointer pt)
{
	int fd,i;
	ws.thread_lock=1;

	ws.thread_lock=0;
	return 0;
}//}}}
//{{{ void on_tree3_dblclk(GtkTreeView *treeview,GtkTreePath *path,GtkTreeViewColumn *col,gpointer userdata)
void on_tree3_dblclk(GtkTreeView *treeview,GtkTreePath *path,GtkTreeViewColumn *col,gpointer userdata)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GValue value;
	memset((void*)&value,0,sizeof(value));
	model=gtk_tree_view_get_model(treeview);
	if(gtk_tree_model_get_iter(model,&iter,path))
	{//idx和doff分别为全局变量，保存用户的选择。
		gtk_tree_model_get_value(model,&iter,0,&value);
		idx=g_value_get_int(&value);//get pid
		g_value_unset(&value);
		gtk_tree_model_get_value(model,&iter,1,&value);
		memset(doff,0,sizeof(doff));
		snprintf(doff,sizeof(doff),"%s",g_value_get_string(&value));
		gtk_entry_set_text(GTK_ENTRY(ws.entry[2]),doff);
	}
}//}}}
//{{{ void on_addlock(GtkWidget *widget,gpointer gp)
void on_addlock(GtkWidget *widget,gpointer gp)
{
	check_lock(doff);
	/*GtkTreeModel *model;
	GtkTreeIter	  iter;
	gboolean      bl;
	char ch[100];
	int count;
	count=0;
	model=gtk_tree_view_get_model(GTK_TREE_VIEW(ws.list[1]));
	bl=gtk_tree_model_get_iter_first(model,&iter);
	while(bl)
	{
		count++;
		bl=gtk_tree_model_iter_next(model,&iter);
	}
	memset(ch,0,sizeof(ch));
	snprintf(ch,sizeof(ch),"row counts=%d",count);
	msgbox(ch);*/
}//}}}
//{{{ void on_dellock(GtkWidget *widget,gpointer gp)
void on_dellock(GtkWidget *widget,gpointer gp)
{
	GtkTreeModel *model;
	GtkTreeIter	  iter;
	GtkTreeSelection *sel;
	gchar *str[2];
	gint  gin[2];
	char  ch[100];
	int i,j;
	sel=gtk_tree_view_get_selection(GTK_TREE_VIEW(ws.list[1]));
	if(!gtk_tree_selection_get_selected(sel,&model,&iter))
	{
		msgbox("get error");
		return;
	}
	gtk_tree_model_get(model,&iter,0,&str[0],1,&str[1],2,&gin[0],3,&gin[1],-1);
	memset(ch,0,sizeof(ch));
	snprintf(ch,sizeof(ch),"%s",str[1]);
	i=strlen(ch);
	for(j=0;j<8;j++)
	{
		if(memcmp(ch,sl[j].ch,i)==0)//find it
		{
			memset((void*)&sl[j],0,sizeof(sl[j]));//clear			
			gtk_list_store_remove(GTK_LIST_STORE(model),&iter);
			break;
		}
	}
	//snprintf(ch,sizeof(ch),"%s %s %d %d",str[0],str[1],gin[0],gin[1]);
	g_free(str[0]);g_free(str[1]);
}//}}}
//{{{ int check_lock(char *c)
int check_lock(char *c)
{
	GtkTreeIter iter;
	GtkListStore *store;
	int i,j,k,l,len;
	char c1[20],c2[100];
	char *gc;
	len=strlen(c);
	if(len>40 || len<=0)
	{g_print("impossible!\n");return 1;}
	for(i=0;i<8;i++)
	{
		if(memcmp(c,sl[i].ch,len)==0)
		{
			msgbox("你当前要锁定的地址早已加入到锁定列表中");
			return 1;
		}
	}
	memset(c1,0,20);
	gc=(char*)gtk_entry_get_text(GTK_ENTRY(ws.entry[3]));
	i=strlen(gc);
	if(i>20)
	{g_print("error\n");return 1;}
	memcpy(c1,gc,i);
	j=atoi(c1);
	if(j<0)
	{g_print("error1\n");return 1;}
	memset(c1,0,20);
	gc=(char*)gtk_entry_get_text(GTK_ENTRY(ws.entry[4]));
	i=strlen(gc);
	if(i>20)
	{g_print("error11\n");return 1;}
	memcpy(c1,gc,i);
	k=atoi(c1);
	if(k<0)
	{g_print("error122\n");return 1;}
	memset(c1,0,20);
	gc=(char*)gtk_entry_get_text(GTK_ENTRY(ws.entry[1]));
	i=strlen(gc);
	if(i>20)
	{g_print("error1144\n");return 1;}
	memcpy(c1,gc,i);
	l=0;
	for(i=0;i<8;i++)
	{
		if(sl[i].ch[0]==0)//
		{
			l=1;
			memcpy(sl[i].ch,c,strlen(c));
			sl[i].add.maxd=k;
			sl[i].add.mind=j;
			gc=ws.g_addr[0];
			gc+=idx*sizeof(int);//定位到地址集中的目标地址的偏移
			memcpy((void*)&(sl[i].add.ksa),(void*)gc,sizeof(struct KVAR_SAD));
			break;
		}
	}
	if(l!=1)
	{
		msgbox("锁定的目标地址最多8个");
		return 1;
	}
	store = GTK_LIST_STORE(gtk_tree_view_get_model
			(GTK_TREE_VIEW(ws.list[1])));
	gtk_list_store_append(store,&iter);
	gtk_list_store_set(store,&iter,0,c1,1,c,2,j,3,k,-1);
	return 0;
}//}}}
//{{{ void on_lock(GtkWidget *widget,gpointer gp)
void on_lock(GtkWidget *widget,gpointer gp)
{
	int i,j,k,fd;
	char ch[40],*c,*lb;
	struct KVAR_AM *k_am;
	unsigned int u[2],v,z;
	unsigned short us[2];
	if(ws.dseg>ws.bseg)
	{
		u[0]=ws.bseg;us[0]=0;
		u[1]=ws.dseg;us[1]=1;
	}
	else
	{
		u[0]=ws.dseg;us[0]=1;
		u[1]=ws.bseg;us[1]=0;
	}
	memset(ws.g_ch,0,buf_size);
	k_am=(struct KVAR_AM *)ws.g_ch;
	j=0;
	lb=(char*)gtk_button_get_label(GTK_BUTTON(ws.bnt[8]));
	if(strncmp(lb,"锁    定",strlen(lb))==0)
	{
		gtk_button_set_label(GTK_BUTTON(ws.bnt[8]),"取消锁定");
		for(i=0;i<8;i++)
		{
			v=0;
			if(sl[i].ch[0]!=0)
			{
				if(tonum(sl[i].ch,&v))
					goto err_a01;
				z=sl[i].add.ksa.s_b.pbit*4096;
				z+=sl[i].add.ksa.off;
				if(v>u[1])
					z+=u[1];
				else
					z+=u[0];
				if(z==v)
				{
					memcpy((void*)&(k_am->vadd[j]),(void*)&(sl[i].add),sizeof(struct KVAR_ADDR));
					j++;
				}
				else
				{
					memset(ch,0,sizeof(ch));
					snprintf(ch,sizeof(ch),"org:0x%lx  calc:0x%lx",v,z);
					msgbox(ch);
					break;
				}
			}
		}
		k_am->sync=0;
		k_am->end0=0;
		k_am->cmd=3;
		k_am->pid=ws.pid;
		fd=open(drv_name,O_RDWR);
		if(fd<0)
		{
			msgbox("目标模块连接失败");
			return;
		}
		i=write(fd,ws.g_ch,buf_size);//发送命令
		close(fd);
		return;
	}
	else
	{
		gtk_button_set_label(GTK_BUTTON(ws.bnt[8]),"锁    定");
		k_am->sync=0;
		k_am->end0=1;
		k_am->cmd=3;
		fd=open(drv_name,O_RDWR);
		if(fd<0)
		{
			msgbox("目标模块连接失败");
			return;
		}
		i=write(fd,ws.g_ch,buf_size);//发送命令
		close(fd);
		return;
	}
//锁定操作不用启动线程，直接发送命令即可。
	return;
err_a01:
	msgbox("error");
	return;
}//}}}
//{{{ int tonum(char *c,unsigned int *ui)
int tonum(char *c,unsigned int *ui)
{
	int i,j,k;
	unsigned u;
	j=strlen(c);
	if(j>10)
		return 1;
	if(c[0]!='0' || c[1]!='x')
		return 1;
	u=0;
	for(i=2;i<j;i++)
	{
		k=toupper(c[i]);
		if(k>=0x30 && k<=0x39)
		{
			u*=0x10;
			u+=k;u-=0x30;
		}
		else
		{
			if(k>='A' && k<='F')
			{
				u*=0x10;
				u+=k;u-=0x37;
			}
			else
				return 1;
		}
	}
	*ui=u;
	return 0;
}//}}}



//{{{
/*
 ws.scroll[0]=gtk_scrolled_window_new(NULL,NULL);
	gtk_widget_set_size_request(ws.scroll[0],164,304);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ws.scroll[0]),GTK_POLICY_AUTOMATIC,GTK_POLICY_ALWAYS);
	gtk_fixed_put(GTK_FIXED(ws.fixed),ws.scroll[0],10,40);
	ws.list[0]=gtk_list_new();
	gtk_list_set_selection_mode(GTK_LIST(ws.list[0]),GTK_SELECTION_SINGLE);
	gtk_list_scroll_vertical(GTK_LIST(ws.list[0]),GTK_SCROLL_STEP_FORWARD,0);
	gtk_widget_set_size_request(ws.list[0],160,300);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(ws.scroll[0]),ws.list[0]);
	memset(ch,0,6);
	for(j=0;j<5;j++)
	{
		ch[j]='a';
	}
	for(i=0;i<20;i++)
	{
		item=gtk_list_item_new_with_label(ch);
		for(j=0;j<5;j++)
		{
			ch[j]++;
		}
		gtk_container_add(GTK_CONTAINER(ws.list[0]),item);
	}
//下列代码是treeview的双击事件响应，待测试！
g_signal_connect(view, "row-activated", (GCallback) view_onRowActivated, NULL);
void view_onRowActivated (GtkTreeView *treeview,
                      GtkTreePath *path,
                      GtkTreeViewColumn *col,
                      gpointer userdata)
{
    GtkTreeModel *model;
    GtkTreeIter iter;

    g_print ("A row has been double-clicked!\n");

    model = gtk_tree_view_get_model(treeview);

    if (gtk_tree_model_get_iter(model, &iter, path))
    {
         gchar *name;

         gtk_tree_model_get(model, &iter, COLUMN, &name, -1);

         g_print ("Double-clicked row contains name %s\n", name);

         g_free(name);
    }
}
//下列代码是测试列表框是否还有记录
static void remove_item(GtkWidget * widget, gpointer selection)
{
    GtkListStore *store;
    GtkTreeModel *model;
    GtkTreeIter iter;
    store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(list)));
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(list));
    if (gtk_tree_model_get_iter_first(model, &iter) == FALSE)
        return;
    if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(selection), &model, &iter)) {
        gtk_list_store_remove(store, &iter);
    }
}

static void remove_all(GtkWidget * widget, gpointer selection)
{
    GtkListStore *store;
    GtkTreeModel *model;
    GtkTreeIter iter;
    store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(list)));
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(list));
    if (gtk_tree_model_get_iter_first(model, &iter) == FALSE)
        return;
    gtk_list_store_clear(store);
}

 *///}}}











