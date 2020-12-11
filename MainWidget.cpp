#include "MainWidget.h"
#include "ui_mainWidget.h"
#include <QPainter>
#include "myQss.h"
#include <QFileDialog>
#include <QStandardPaths>
#include <QScrollBar>
#include "Music.h"
#include <QMouseEvent>
#include <QtSql>
#include <QMessageBox>
#include <QInputDialog>
#include <QDateTime>
#include "MusicListDialog.h"
#include "myslider.h"
MainWidget::MainWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);

    //UI初始化（.ui文件中无法完成的设置，这里补上）
    init_UI();

    //初始化生成播放器模块及相关组件
    init_play();

    //右键菜单初始化
    init_actions();

    //数据库初始化
    init_sqlite();

    //歌单初始化
    init_musicList();

    //配置初始化
    init_settings();

    //系统托盘初始化
    init_systemTrayIcon();
}

MainWidget::~MainWidget()
{
    delete ui;
}
void MainWidget::paintEvent(QPaintEvent *event)
{
    //需要添加以下代码，才能正常在主窗口Widget中显示背景图片（https://blog.csdn.net/xiejie0226/article/details/81165379）
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    QWidget::paintEvent(event);
}

void MainWidget::init_UI()
{
    //窗口设置圆角后，会出现留白，需要添加以下代码
    setAttribute(Qt::WA_TranslucentBackground, true);
    //去除标题栏
    setWindowFlags(Qt::FramelessWindowHint);

    //UI初始化（.ui文件中无法完成的设置，这里补上）
    ui->volumeSlider->hide();
    ui->playListWidget->verticalScrollBar()->setStyleSheet(ListWidgetStyle());
    ui->localMusicWidget->verticalScrollBar()->setStyleSheet(ListWidgetStyle());
    ui->favorMusicWidget->verticalScrollBar()->setStyleSheet(ListWidgetStyle());
    ui->nameListWidget->verticalScrollBar()->setStyleSheet(ListWidgetStyle());
    ui->musicListWidget->verticalScrollBar()->setStyleSheet(ListWidgetStyle());
    ui->playListWidget->setIcon(QIcon(":/image/image/image/music.png"));
    ui->localMusicWidget->setIcon(QIcon(":/image/image/image/music-file.png"));
    ui->favorMusicWidget->setIcon(QIcon(":/image/image/image/like.png"));
    ui->musicListWidget->setIcon(QIcon(":/image/image/image/MusicListItem.png"));
}

void MainWidget::init_play()
{
    //播放器初始化
    player= new QMediaPlayer(this);
    playlist=new QMediaPlaylist;
    playlist->setPlaybackMode(QMediaPlaylist::Loop);//初始时设置列表顺序循环
    player->setPlaylist(playlist);//把初始化的播放器和播放表关联起来

    ui->positionSlider->setTracking(false);
    //连接槽与信号
    connect(ui->positionSlider, &QAbstractSlider::valueChanged, this, &MainWidget::setPosition);//人为拉动进度条则改变播放进度进度
    connect(player, &QMediaPlayer::positionChanged, this, &MainWidget::updatePosition);//进度条，时间显示，歌词显示跟随播放进度变化
    connect(player, &QMediaPlayer::durationChanged, this, &MainWidget::updateDuration);//每次播放新的歌曲根据当前歌曲的时长设置进度条最大值，或者没有歌曲播放时设置相关界面元素的显示
    connect(player, &QMediaPlayer::metaDataAvailableChanged, this, &MainWidget::updateInfo);//每次播放新的歌曲更新相关图片和文字的显示
    connect(player, &QMediaPlayer::stateChanged, this, &MainWidget::updatePlayBtn);//播放状态改变时更新相关控件的样式和文字
}

void MainWidget::init_actions()
{
    //“当前播放”列表右键菜单初始化（Action作用：让多个信号对应同一个槽函数）
    ui->playListWidget->setContextMenuPolicy(Qt::CustomContextMenu);//设置“当前播放”窗口中列表的条目开启自定义右键菜单
    QAction *action_playlist_delete=new QAction(QIcon(":/image/image/image/remove.png"),u8"移除");//初始化“当前播放”窗口中列表的"删除"动作及其图标
    connect(action_playlist_delete,&QAction::triggered,this,&MainWidget::playlist_removeMusic);//把点击该选项的信号关联到MainWidget中"移除当前播放列表的选中歌曲"
    QAction *action_playlist_showfile=new QAction(QIcon(":/image/image/image/music-dir.png"),u8"打开所在文件夹");
    connect(action_playlist_showfile,&QAction::triggered,ui->playListWidget,&MusicListWidget::showInExplorer);
    QAction *action_playlist_detail=new QAction(QIcon(":/image/image/image/detail.png"),u8"歌曲详情");//@@
    connect(action_playlist_detail,&QAction::triggered,ui->playListWidget,&MusicListWidget::detail);
    QAction *action_play_to_favor=new QAction(QIcon(":/image/image/image/To-like.png"),u8"添加到我喜欢");
    connect(action_play_to_favor,&QAction::triggered,this,&MainWidget::play_to_favor);//以上六行同理
    menu_playlist=new QMenu(this);
    menu_playlist->addAction(action_playlist_delete);
    menu_playlist->addAction(action_playlist_showfile);
    menu_playlist->addAction(action_playlist_detail);
    menu_playlist->addAction(action_play_to_favor);//以上五行把上边设置好的动作添加到右键菜单的动作选项列表中

    //“本地音乐”列表右键菜单初始化
    ui->localMusicWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    QAction *action_locallist_delete=new QAction(QIcon(":/image/image/image/remove.png"),u8"移除");
    connect(action_locallist_delete,&QAction::triggered,ui->localMusicWidget,&MusicListWidget::removeMusic);//管理本地音乐列表不用管当前播放条目的顺序（只在“当前播放”窗口体现）
    QAction *action_locallist_showfile=new QAction(QIcon(":/image/image/image/music-dir.png"),u8"打开所在文件夹");
    connect(action_locallist_showfile,&QAction::triggered,ui->localMusicWidget,&MusicListWidget::showInExplorer);
    QAction *action_locallist_detail=new QAction(QIcon(":/image/image/image/detail.png"),u8"歌曲详情");//@@
    connect(action_locallist_detail,&QAction::triggered,ui->localMusicWidget,&MusicListWidget::detail);
    QAction *action_local_to_favor=new QAction(QIcon(":/image/image/image/To-like.png"),u8"添加到我喜欢");
    connect(action_local_to_favor,&QAction::triggered,this,&MainWidget::local_to_favor);
    QAction *action_local_to_playlist=new QAction(QIcon(":/image/image/image/To-playlist.png"),u8"添加到当前播放列表");
    connect(action_local_to_playlist,&QAction::triggered,this,&MainWidget::local_to_playlist);
    menu_locallist=new QMenu(this);
    menu_locallist->addAction(action_locallist_delete);
    menu_locallist->addAction(action_locallist_showfile);
    menu_locallist->addAction(action_locallist_detail);
    menu_locallist->addAction(action_local_to_favor);
    menu_locallist->addAction(action_local_to_playlist);//以上和“当前播放”列表右键菜单初始化同理

    //“我喜欢”列表右键菜单初始化
    ui->favorMusicWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    QAction *action_favorlist_delete = new QAction(QIcon(":/image/image/image/remove.png"),u8"移除");
    connect(action_favorlist_delete, &QAction::triggered, ui->favorMusicWidget, &MusicListWidget::removeMusic);
    QAction *action_favorlist_showfile = new QAction(QIcon(":/image/image/image/music-dir.png"),u8"打开所在文件夹");
    connect(action_favorlist_showfile, &QAction::triggered, ui->favorMusicWidget, &MusicListWidget::showInExplorer);
    QAction *action_favorlist_detail = new QAction(QIcon(":/image/image/image/detail.png"),u8"歌曲详情");//@@
    connect(action_favorlist_detail, &QAction::triggered, ui->favorMusicWidget, &MusicListWidget::detail);
    QAction *action_favor_to_playlist=new QAction(QIcon(":/image/image/image/To-playlist.png"),u8"添加到当前播放列表");
    connect(action_favor_to_playlist,&QAction::triggered,this,&MainWidget::favor_to_playlist);
    menu_favorlist = new QMenu(this);
    menu_favorlist->addAction(action_favorlist_delete);
    menu_favorlist->addAction(action_favorlist_showfile);
    menu_favorlist->addAction(action_favorlist_detail);
    menu_favorlist->addAction(action_favor_to_playlist);//以上和“当前播放”列表右键菜单初始化同理

    //“我的歌单”列表右键菜单初始化
    ui->nameListWidget->setContextMenuPolicy(Qt::CustomContextMenu);//nameListWidget是系统中的类QListWidget
    QAction *action_namelist_delete=new QAction(QIcon(":/image/image/image/remove.png"),u8"删除");
    connect(action_namelist_delete,&QAction::triggered,this,&MainWidget::namelist_delete);
    menu_namelist=new QMenu(this);
    menu_namelist->addAction(action_namelist_delete);//以上和“当前播放”列表右键菜单初始化同理

    //歌单展示列表右键菜单初始化
    ui->musicListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    QAction *action_musiclist_delete = new QAction(QIcon(":/image/image/image/remove.png"), u8"移除");
    connect(action_musiclist_delete, &QAction::triggered, this, &MainWidget::musiclist_removeMusic);
    QAction *action_musiclist_showfile = new QAction(QIcon(":/image/image/image/music-dir.png"), u8"打开所在文件夹");
    connect(action_musiclist_showfile, &QAction::triggered, ui->musicListWidget, &MusicListWidget::showInExplorer);
    QAction *action_musiclist_detail = new QAction(QIcon(":/image/image/image/detail.png"), u8"歌曲详情");//@@
    connect(action_musiclist_detail, &QAction::triggered, ui->musicListWidget, &MusicListWidget::detail);
    QAction *action_musiclist_to_favor = new QAction(QIcon(":/image/image/image/To-like.png"), u8"添加到我喜欢");
    connect(action_musiclist_to_favor, &QAction::triggered, this, &MainWidget::musiclist_to_favor);
    QAction *action_musiclist_to_playlist = new QAction(QIcon(":/image/image/image/To-playlist.png"), u8"添加到当前播放列表");
    connect(action_musiclist_to_playlist, &QAction::triggered, this, &MainWidget::musiclist_to_playlist);
    menu_musiclist = new QMenu(this);
    menu_musiclist->addAction(action_musiclist_delete);
    menu_musiclist->addAction(action_musiclist_showfile);
    menu_musiclist->addAction(action_musiclist_detail);
    menu_musiclist->addAction(action_musiclist_to_favor);
    menu_musiclist->addAction(action_musiclist_to_playlist);//以上和“当前播放”列表右键菜单初始化同理

    //“换肤”的菜单项
    QAction *action_backgroud_to_default = new QAction(QIcon(":/image/image/image/default.png"),u8"更换到默认背景");//没有设置CustomContextMenu，故默认左键点击才显示菜单
    connect(action_backgroud_to_default,&QAction::triggered,this,&MainWidget::background_to_default);
    QAction *action_backgroud_setting=new QAction(QIcon(":/image/image/image/setting.png"),u8"自定义背景");
    connect(action_backgroud_setting,&QAction::triggered,this,&MainWidget::background_setting);
    menu_changeSkin=new QMenu(this);
    menu_changeSkin->addAction(action_backgroud_to_default);
    menu_changeSkin->addAction(action_backgroud_setting);//以上和“当前播放”列表右键菜单初始化同理
}

void MainWidget::init_sqlite()//为一个指定歌单（列表）的歌曲信息，和"我的歌单"(列表的列表)创建数据库@@（没太明白怎么区分一个数据库中的两个表？）
{
    QSqlDatabase database;
    if (QSqlDatabase::contains("qt_sql_default_connection"))
    {
        database = QSqlDatabase::database("qt_sql_default_connection");
    }//database是名为qt_sql_default_connection的默认数据库连接
    else
    {
        database = QSqlDatabase::addDatabase("QSQLITE");
        database.setDatabaseName("Music.db");
        database.setUserName("NJUTang");
        database.setPassword("123456");//以上，如果默认连接不存在则创建新的数据库并设置账号密码
        if (!database.open())
        {
            QMessageBox::critical(this,"无法打开数据库文件：Music.db",database.lastError().databaseText());//打开消息窗口并返回一个最近错误信息的描述到该窗口
            exit(-1);
        }
    }
    //检查两个表是否存在，不存在则创建不存在的表
    QSqlQuery query;
    query.exec(QString("select count(*) from sqlite_master where type='table' and name='%1'").arg("MusicInfo"));
    if(query.next()){
        if(query.value(0).toInt()==0){
            QSqlQuery sql_query;
            //QString create_sql = "create table MusicInfo (id int primary key, name varchar(30), url varchar(200), author varchar(50), title varchar(50), duration bigint, albumTitle varchar(50), audioBitRate int)";
            QString create_sql = "create table MusicInfo (name varchar(30), url varchar(200), author varchar(50), title varchar(50), duration bigint, albumTitle varchar(50), audioBitRate int)";
            sql_query.prepare(create_sql);
            sql_query.exec();
        }
    }
    QSqlQuery query2;
    query2.exec(QString("select count(*) from sqlite_master where type='table' and name='%1'").arg("MusicLists"));
    if(query2.next()){
        if(query2.value(0).toInt()==0){
            QSqlQuery sql_query;
            QString create_sql = "create table MusicLists (name varchar(30))";
            sql_query.prepare(create_sql);
            sql_query.exec();
        }
    }
}

void MainWidget::init_musicList()
{
    //本地音乐 初始化
    ui->localMusicWidget->musicList.setName("LocalMusic");
    ui->localMusicWidget->musicList.read_fromSQL();//每次打开主窗口都从数据库恢复"本地音乐"窗口列表中每一首歌的信息
    ui->localMusicWidget->refresh();//把"本地音乐"列表的条目和图标更新并显示到本地音乐窗口中
    //我喜欢 初始化
    ui->favorMusicWidget->musicList.setName("FavorMusic");
    ui->favorMusicWidget->musicList.read_fromSQL();
    ui->favorMusicWidget->refresh();//以上三行同理

    //从数据库中恢复歌单（主窗口右边"我的歌单"窗口列表（表项是歌曲列表，不是歌！））
    QSqlQuery sql_query;
    QString select_sql = "select name from MusicLists";
    sql_query.prepare(select_sql);
    if(sql_query.exec())
    {
        while(sql_query.next())//遍历数据库
        {
            QString musicListName=sql_query.value(0).toString();//返回当前遍历到的歌单名的值
            MusicList tempList;
            tempList.setName(musicListName);//把查询到的歌单名赋值给创建的一个临时歌单
            tempList.read_fromSQL();//根据该歌单名恢复这个歌单的所有歌曲和信息
            musiclist.push_back(tempList);//把该临时歌单添加到"我的歌单"窗口中的列表尾部
        }
    }
    namelist_refresh();//把"我的歌单"列表的条目和图标更新并显示到我的歌单窗口中
}

void MainWidget::namelist_refresh()
{
    //先清空
    QSqlQuery sql_query;
    QString delete_sql = "delete from MusicLists";
    sql_query.prepare(delete_sql);//prepare里的字符串可以决定做什么操作吗？？
    sql_query.exec();

    for(size_t i=0;i<musiclist.size();i++){//遍历MainWidget中的歌单列表（列表的列表）
        QSqlQuery sql_query2;
        QString insert_sql = "insert into MusicLists values (?)";
        sql_query2.prepare(insert_sql);
        sql_query2.addBindValue(musiclist[i].getName());//根据歌单名关键字把MinWidget中歌单列表中的每一个列表插入到数据库中的MusicLists表中
        sql_query2.exec();
    }
    //展示列表刷新
    ui->nameListWidget->clear();//调用QListWidget的系统方法清空主窗口右侧"我的歌单"窗口列表中的所有可见条目
    for(size_t i=0;i<musiclist.size();i++){ //遍历MainWidget中的歌单列表
        QListWidgetItem *item = new QListWidgetItem;
        item->setIcon(QIcon(":/image/image/image/music_list.png"));
        item->setText(musiclist[i].getName());
        ui->nameListWidget->addItem(item);//以上四行用QListWidgetItem的系统方法把MainWidget中歌单列表的每一个歌单条目的名字和图标显示到"我的歌单"窗口中
    }
}

void MainWidget::init_settings()
{
    QSettings mysettings("./LightMusicPlayer.ini",QSettings::IniFormat);
    mysettings.setIniCodec("UTF8");//以上两行和background_setting同理
    QString fileName = mysettings.value("background/image-url").toString();//返回初始化设置文件.ini中主窗口背景的本地资源文件路径
    QImage testImage(fileName);
    if(!fileName.isEmpty()&&!testImage.isNull())//如果.ini文件中有上次自定义的背景或默认背景则直接按照.ini文件设置主窗口样式
    {
        setStyleSheet(QString("QWidget#Widget{"
                              "border-radius:10px;"
                              "border-image: url(%1);}").arg(fileName));
    }else{//如果.ini文件中没有设置背景则把主窗口样式改成默认背景
        fileName=":/image/image/background/default.jpg";
        QSettings mysettings("./LightMusicPlayer.ini",QSettings::IniFormat);
        mysettings.setIniCodec("UTF8");
        mysettings.setValue("background/image-url",fileName);
        setStyleSheet(QString("QWidget#Widget{"
                              "border-radius:10px;"
                              "border-image: url(%1);}").arg(fileName));
    }
}

void MainWidget::musicListWidget_refresh()//把指定歌单的歌曲条目显示到“歌单”窗口列表中
{
    if(musiclist_index != -1){//标识当前是哪个歌单
        ui->musicListNameLabel->setText(u8"歌单 - "+musiclist[musiclist_index].getName());//在“歌单”窗口上方显示歌单名
        ui->musicListWidget->setMusicList_playing(musiclist[musiclist_index]);//把指定歌单的歌曲条目显示到“歌单窗口上”
    }
}

void MainWidget::on_playListWidget_customContextMenuRequested(const QPoint &pos)
{
    if(ui->playListWidget->itemAt(pos)==Q_NULLPTR)
    {
        return;
    }
    menu_playlist->exec(QCursor::pos());
}

void MainWidget::playlist_removeMusic()//移除当前播放列表中的一首选中歌曲
{
    int pos=ui->playListWidget->currentRow();//返回“当前播放”列表中选中歌曲条目的序号（调用MusicListWidget父类的方法）
    int playing_pos=playlist->currentIndex();//返回“当前播放”列表中正在播放的条目的序号
    ui->playListWidget->removeMusic();//用MusicListWidget中的removeMusic()调用它的成员类MusicList中的removeMusic(int pos),并保证移除后的列表不乱序
    if(pos<playing_pos){//如果删除的歌曲条目在正在播放条目之前
        //移除前备份
        QMediaPlayer::State state=player->state();//记录下删除前正在播放条目的播放状态（暂停、播放等）
        qint64 position_backup=player->position();//记录下删除前正在播放条目的播放进度
        playing_pos=playing_pos-1;//删除后下面条目上移，故正在播放条目序号应该减一
        playlist->removeMedia(pos);//删除选中条目
        //移除后恢复
        playlist->setCurrentIndex(playing_pos);//更正删除前正在播放条目的序号
        player->setPosition(position_backup);//恢复删除前正在播放条目的播放进度
        ui->positionSlider->setValue(position_backup);//恢复进度条控件显示的删除前正在播放条目的播放进度
        if(state==QMediaPlayer::PlayingState){
            player->play();
        }else if(state==QMediaPlayer::PlayingState){
            player->pause();
        }//以上四行恢复删除前正在播放条目的播放状态（暂停、播放等）
    }else if(pos<playing_pos){//如果删除的歌曲条目在正在播放条目之后
        playlist->removeMedia(pos);//不备份当前播放歌曲，直接删除选中条目
        playlist->setCurrentIndex(playing_pos);//下面这句话就算要加也应该这样吧？
        //playlist->setCurrentIndex(pos);//???这句话目的是啥???确定没有写错吗？不加好像也没影响啊
    }else{
        playlist->removeMedia(pos);//如果选中的（将要移除的）条目是正在播放的歌曲，直接删除
    }
}

void MainWidget::play_to_favor()//“当前播放”列表的选中条目添加到“我喜欢播放列表”
{
    int pos=ui->playListWidget->currentRow();//获取选中条目在列表中的序号
    ui->favorMusicWidget->musicList.addMusic(ui->playListWidget->musicList.getMusic(pos));//把选中Music条目的引用传给addMusic，即在"我喜欢"窗口中的列表尾部增加一首歌
    ui->favorMusicWidget->refresh();//更新"我喜欢"窗口列表条目的显示状态
}

void MainWidget::local_to_favor()//“本地音乐”列表”的选中条目添加到“我喜欢”播放列表
{
    int pos=ui->localMusicWidget->currentRow();
    ui->favorMusicWidget->musicList.addMusic(ui->localMusicWidget->musicList.getMusic(pos));
    ui->favorMusicWidget->refresh();
}//与play_to_favor()同理

void MainWidget::local_to_playlist()//“本地音乐”列表”的选中条目添加到“当前播放”列表
{
    int pos=ui->localMusicWidget->currentRow();
    Music tempMusic=ui->localMusicWidget->musicList.getMusic(pos);//返回“本地音乐”列表里选中的一首音乐
    ui->playListWidget->musicList.addMusic(tempMusic);//添加到"当前播放"窗口中列表的尾部,用于显示新增条目
    ui->playListWidget->refresh();//更新"当前播放"窗口列表条目的显示状态
    //添加到播放器
    playlist->addMedia(tempMusic.getUrl());//把新增条目添加到与播放器关联的播放列表
}

void MainWidget::favor_to_playlist()
{
    int pos=ui->favorMusicWidget->currentRow();
    Music tempMusic=ui->favorMusicWidget->musicList.getMusic(pos);
    ui->playListWidget->musicList.addMusic(tempMusic);
    ui->playListWidget->refresh();
    //添加到播放器
    playlist->addMedia(tempMusic.getUrl());
}//以上和local_to_playlist()同理

void MainWidget::namelist_delete()
{
    int pos=ui->nameListWidget->currentRow();
    musiclist[pos].remove_SQL_all();//先从数据库中删除歌曲信息
    //移除歌单
    int i=0;
    for(auto it=musiclist.begin();it!=musiclist.end();){
        if(i==pos){
            it= musiclist.erase(it);
            break;
        }
        else{
            it++;
        }
        i++;
    }
    namelist_refresh();
}

void MainWidget::musiclist_removeMusic()
{
    int pos=ui->musicListWidget->currentRow();
    musiclist[musiclist_index].removeMusic(pos);
    musicListWidget_refresh();
}

void MainWidget::musiclist_to_favor()
{
    int pos=ui->musicListWidget->currentRow();
    ui->favorMusicWidget->musicList.addMusic(musiclist[musiclist_index].getMusic(pos));
    ui->favorMusicWidget->refresh();
}

void MainWidget::musiclist_to_playlist()
{
    int pos=ui->musicListWidget->currentRow();
    Music tempMusic=ui->musicListWidget->musicList.getMusic(pos);
    ui->playListWidget->musicList.addMusic(tempMusic);
    ui->playListWidget->refresh();
    //添加到播放器
    playlist->addMedia(tempMusic.getUrl());
}

void MainWidget::background_to_default()//把主窗口背景设置成默认图片
{
    QString fileName=":/image/image/background/default.jpg";
    QSettings mysettings("./LightMusicPlayer.ini",QSettings::IniFormat);//把设置的结果保存到初始化文件.ini中，下次打开时会记住这些设置
    mysettings.setIniCodec("UTF8");//设置.ini文件编码
    mysettings.setValue("background/image-url",fileName);//设置.ini文件背景为默认背景
    setStyleSheet(QString("QWidget#Widget{"
                          "border-radius:10px;"
                          "border-image: url(%1);}").arg(fileName));//根据文件名设置主窗口样式（显示默认背景图）
}

void MainWidget::background_setting()
{

    QString fileName=QFileDialog::getOpenFileName(this,("选择自定义背景图片"),QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).first(),u8"图片文件(*jpg *png)");
    //以上一行，从默认图片位置打开文件选择框并返回选中的文件路径
    if(!fileName.isEmpty())
    {
        QImage testImage(fileName);
        if(!testImage.isNull()){
            QSettings mysettings("./LightMusicPlayer.ini",QSettings::IniFormat);
            mysettings.setIniCodec("UTF8");
            mysettings.setValue("background/image-url",fileName);
            setStyleSheet(QString("QWidget#Widget{"
                                  "border-radius:10px;"
                                  "border-image: url(%1);}").arg(fileName));//以上几行和background_to_default()同理
        }
    }
}



void MainWidget::on_localMusicWidget_customContextMenuRequested(const QPoint &pos)//用于右键点击后触发QAction
{
    if(ui->localMusicWidget->itemAt(pos)==Q_NULLPTR)
    {
        return;
    }
    menu_locallist->exec(QCursor::pos());//让触发的右键菜单显示在鼠标点击的位置处
}

void MainWidget::on_favorMusicWidget_customContextMenuRequested(const QPoint &pos)
{
    if(ui->favorMusicWidget->itemAt(pos)==Q_NULLPTR)
    {
        return;
    }
    menu_favorlist->exec(QCursor::pos());
}

void MainWidget::on_nameListWidget_customContextMenuRequested(const QPoint &pos)
{
    if(ui->nameListWidget->itemAt(pos)==Q_NULLPTR)
    {
        return;
    }
    menu_namelist->exec(QCursor::pos());
}


QString formatTime(qint64 timeMilliSeconds)
{
    qint64 seconds = timeMilliSeconds / 1000;
    const qint64 minutes = seconds / 60;
    seconds -= minutes * 60;
    return QStringLiteral("%1:%2")
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'));
}

void MainWidget::updatePosition(qint64 position)
{
    ui->positionSlider->setValue(static_cast<int>(position));//进度条随着播放进度变化
    ui->positionLabel->setText(formatTime(position)+"/"+formatTime(player->duration()));//进度条旁边显示时间的label控件也跟着变化
    if(playlist->currentIndex()>=0)ui->lyricWidget->show(position);//显示当前播放进度的歌词@@
}

void MainWidget::updateDuration(qint64 duration)
{
    ui->positionSlider->setRange(0, static_cast<int>(duration));//根据当前歌曲时长设置进度条的最大值
    ui->positionSlider->setEnabled(static_cast<int>(duration) > 0);//如果当前歌曲时长大于0则让进度条工作
    if(!(static_cast<int>(duration) > 0)) {
        //无音乐播放时，界面元素
        ui->infoLabel->setText("KEEP CALM AND CARRY ON ...");//设置进度条上面那一行默认的白字
        mySystemTray->setToolTip(u8"LightMusicPlayer · By NJU-TJL");//设置鼠标移动到托盘图标上时显示的文字@@
        QImage image(":/image/image/image/non-music.png");
        ui->coverLabel->setPixmap(QPixmap::fromImage(image));//以上两行设置进度条旁边那个显示专辑的区域为默认图片（唱片播放机图）
        ui->musicTitleLabel->setText("");
        ui->musicAlbumLabel->setText("");
        ui->musicAuthorLabel->setText("");//以上三行设置没有歌曲播放的时候“词”界面不显示歌名，歌手，专辑名
        ui->lyricWidget->clear();//歌词显示窗口的其他显示区都不显示文字
    }//以上为当前播放时长为0时相关的界面图标的设置
    ui->positionSlider->setPageStep(static_cast<int>(duration) / 10);//每点击进度条上某个位置一下进度条就以时长十分之一的粒度向点击位置靠近（太麻烦了！最好优化成一点击就调整到点击的进度去）
}

void MainWidget::setPosition(int position)//根据人为拉动的进度条改变当前播放进度（拖动的时候还快进播放太傻了！最好优化成拖动的时候不改变松开鼠标才改变）
{
    // avoid seeking when the slider value change is triggered from updatePosition()
    if (qAbs(player->position() - position) > 99)//如果是人为拉动进度条就改变当前进度,如果是自然播放进度改变则不调用重置进度的函数
        player->setPosition(position);
}

void MainWidget::updateInfo()
{
    if (player->isMetaDataAvailable()) {
        //返回可用MP3元数据列表（调试时可以查看）
        QStringList listInfo_debug=player->availableMetaData();//返回一个可以查看的，数据关键字的列表
        //歌曲信息
        QDateTime current_date_time =QDateTime::currentDateTime();
        QString current_date =current_date_time.toString("yyyy.MM.dd hh:mm:ss");
        int playing_pos=playlist->currentIndex();
        ui->playListWidget->musicList.music[playing_pos].setcurrent_playedtime(current_date);

        QString info="";
        QString author = player->metaData(QStringLiteral("Author")).toStringList().join(",");
        info.append(author);//返回关键字为"Author"的元数据的值，转换为列表，加上逗号分隔符，添加到字符串info中
        QString title = player->metaData(QStringLiteral("Title")).toString();
        QString albumTitle = player->metaData(QStringLiteral("AlbumTitle")).toString();
        info.append(" - "+title);
        info.append(" ["+formatTime(player->duration())+"]");//以上四行同理
        ui->infoLabel->setText(info);//设置进度条上面那一行默认的白字显示刚才的info，格式类似于"G.M.E 邓紫棋 - 喜欢你 [04:01]"
        mySystemTray->setToolTip("正在播放："+info);//设置鼠标移动到托盘图标上面时显示的文字，格式类似于"正在播放：G.M.E 邓紫棋 - 喜欢你 [04:01]"
        //封面图片（应获取"ThumbnailImage" From: https://www.zhihu.com/question/36859497）
        QImage picImage= player->metaData(QStringLiteral("ThumbnailImage")).value<QImage>();//获取元数据中关键字为缩略图的值@@
        if(picImage.isNull()) picImage=QImage(":/image/image/image/non-music.png");//如果当前歌曲没有专辑封面就显示没有歌曲播放时的那个唱片播放机的图片
        ui->coverLabel->setPixmap(QPixmap::fromImage(picImage));//把刚刚的picImage图片转化成位图（pixmap)并设置到进度条旁边那个显示专辑封面的区域上
        ui->coverLabel->setScaledContents(true);//调节专辑封面图片的大小让它填满显示封面的那个区域
        //改变正在播放歌曲的图标
        for(int i=0;i<playlist->mediaCount();i++){//遍历"当前播放"列表里所有的条目（playlist里有一个musiclist,musiclist是music的数组，所以这里的item指的是当前列表里的某个music）
            QListWidgetItem *p=ui->playListWidget->item(i);//p指针指向当前遍历到的一个条目（一个music对象）
            p->setIcon(ui->playListWidget->getIcon());//用列表控件中的设置图标系统方法，通过遍历把所有music对象的图标换成musicListWidget类初始化时的默认图标（黑色背景的白色音符）
        }
        int index=playlist->currentIndex();
        QListWidgetItem *p=ui->playListWidget->item(index);
        p->setIcon(QIcon(":/image/image/image/music-playing.png"));//以上三行再把当前播放的music条目的图标换成另外一个图片（彩色背景的音符）

        //歌词界面显示的信息
        //一下三行用于显示的"title"字符串是上方拼接info时查询player的元数据得到的
        ui->musicTitleLabel->setText(title);
        ui->musicAlbumLabel->setText(u8"专辑："+albumTitle);//u8是让中文显示为UTF8编码，保证其不乱码
        ui->musicAuthorLabel->setText(u8"歌手："+author);//以上三行在“词”界面上方三行文字显示标题、专辑、歌手
        //解析歌词
        ui->lyricWidget->process(ui->playListWidget->musicList.music[index].getLyricFile());//获取“本地已存在的”当前歌曲的歌词文件并实时显示一行歌词到“词”界面的大窗口上
    }
}

void MainWidget::updatePlayBtn()
{
    if(player->state()==QMediaPlayer::PlayingState)//如果当前播放状态是正在播放
    {
        ui->btnPlay->setStyleSheet(PlayStyle());//进度条下面的Qpushbutton控件的样式设置为“||”表示按下就暂停
        action_systemTray_play->setIcon(QIcon(":/image/image/image/pause2.png"));
        action_systemTray_play->setText(u8"暂停");//以上两行设置右键托盘显示的暂停/播放图标和文字的切换（QAction的作用是可以通过不同的控件改变同一件事的状态）
    }
    else{
        ui->btnPlay->setStyleSheet(PaseStyle());
        action_systemTray_play->setIcon(QIcon(":/image/image/image/play2.png"));
        action_systemTray_play->setText(u8"播放");
    }//以上同理
}

void MainWidget::systemTrayIcon_activated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::DoubleClick:
        //显/隐主界面
        if(isHidden()){
            show();
        }else{
            hide();
        }
        break;
    default:
        break;
    }
}//如果双击托盘可以切换显示应用还是最小化应用

void MainWidget::quitMusicPlayer()
{
    //退出应用
    QCoreApplication::quit();
}

void MainWidget::init_systemTrayIcon()
{
    mySystemTray=new QSystemTrayIcon(this);
    mySystemTray->setIcon(QIcon(":/image/image/image/systemTrayIcon.png"));
    mySystemTray->setToolTip(u8"LightMusicPlayer · By NJU-TJL");//以上设置托盘图标和默认文字提示
    connect(mySystemTray,&QSystemTrayIcon::activated,this,&MainWidget::systemTrayIcon_activated);//双击则打开或最小化主窗口
    //添加菜单项
    QAction *action_systemTray_pre = new QAction(QIcon(":/image/image/image/pre2.png"), u8"上一首");
    connect(action_systemTray_pre, &QAction::triggered, this, &MainWidget::on_btnPre_clicked);//把托盘右键菜单的“上一首”点击信号关联到主窗口进度条上方“播放上一首”的按钮控件的槽函数
    action_systemTray_play = new QAction(QIcon(":/image/image/image/play2.png"), u8"播放");
    connect(action_systemTray_play, &QAction::triggered, this, &MainWidget::on_btnPlay_clicked);//这两个为什么要放在类里面定义？
    QAction *action_systemTray_next = new QAction(QIcon(":/image/image/image/next2.png"), u8"下一首");
    connect(action_systemTray_next, &QAction::triggered, this, &MainWidget::on_btnNext_clicked);
    action_systemTray_playmode = new QAction(QIcon(":/image/image/image/loop2.png"), u8"循环播放");//这两个为什么要放在类里面定义？
    connect(action_systemTray_playmode, &QAction::triggered, this, &MainWidget::on_btnPlayMode_clicked);
    QAction *action_systemTray_quit = new QAction(QIcon(":/image/image/image/exit.png"), u8"退出应用");//以上同理
    connect(action_systemTray_quit, &QAction::triggered, this, &MainWidget::quitMusicPlayer);//托盘右键菜单的退出才是真正的退出程序，主窗口的close事件被重写成最小化到系统托盘

    QMenu *pContextMenu = new QMenu(this);
    pContextMenu->addAction(action_systemTray_pre);
    pContextMenu->addAction(action_systemTray_play);
    pContextMenu->addAction(action_systemTray_next);
    pContextMenu->addAction(action_systemTray_playmode);
    pContextMenu->addAction(action_systemTray_quit);
    mySystemTray->setContextMenu(pContextMenu);//把以上动作设置到系统托盘的右键菜单
    mySystemTray->show();//使右键菜单可见
}

void MainWidget::mousePressEvent(QMouseEvent *event)
{
    //实现点击界面中某点，音量条隐藏
    int x=event->pos().x();
    int y=event->pos().y();
    int x_widget=ui->volumeSlider->geometry().x(); //注：这里“wiget”的名字是要从UI文件编译后生成的ui_xxx.h文件中得知（在UI布局中看不到）
    int y_widget=ui->volumeSlider->geometry().y();
    int w=ui->volumeSlider->geometry().width();
    int h=ui->volumeSlider->geometry().height();
    if(!(x>=x_widget&&x<=x_widget+w && y>=y_widget&&y<=y_widget+h)){
        ui->volumeSlider->hide();
    }

    //记录窗口移动的初始位置
    offset = event->globalPos() - pos();
    event->accept();
}

void MainWidget::mouseMoveEvent(QMouseEvent *event)
{
    //int x=event->pos().x();
    int y=event->pos().y();
    //注：这里“layoutWidget1”的名字是要从UI文件编译后生成的ui_xxx.h文件中得知（在UI布局中看不到）
    if((y<ui->titleLabel->geometry().height())||y>ui->coverLabel->geometry().height()){
        move(event->globalPos() - offset);
        event->accept();
        setCursor(Qt::ClosedHandCursor);
    }
}

void MainWidget::mouseReleaseEvent(QMouseEvent *event)
{
    offset = QPoint();
    event->accept();
    setCursor(Qt::ArrowCursor);
}

void MainWidget::closeEvent(QCloseEvent *event)
{
    //最小化到托盘
    if(!mySystemTray->isVisible()){
        mySystemTray->show();
    }
    hide();
    event->ignore();
}

void MainWidget::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

void MainWidget::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    ui->localMusicWidget->musicList.addMusic(urls);
    ui->localMusicWidget->refresh();
    ui->stackedWidget->setCurrentIndex(1);//切换到本地音乐

}
void MainWidget::on_btnQuit_clicked()//点击主窗口的退出键（×）的槽函数是调用重写的close事件，最小到系统托盘
{
    close();
}

void MainWidget::on_btnPlay_clicked()//点击主窗口进度条上的控件改变播放器的播放状态
{
    if(player->state()==QMediaPlayer::PlayingState)
     {
        player->pause();

     }
    else if(player->state()==QMediaPlayer::PausedState){

        player->play();
    }
    else if(!playlist->isEmpty() && (player->state()==QMediaPlayer::StoppedState))//如果当前播放列表有歌曲且因为播放完了最后一首歌曲而停止
    {
        playlist->setCurrentIndex(0);
        player->play();//重新从列表第一项开始播放
    }
}

void MainWidget::on_btnNext_clicked()//点击主窗口中下一曲控件
{
    playlist->next();//调用基类QMediaPlaylist的方法播放下一曲
}

void MainWidget::on_btnPre_clicked()//点击主窗口中上一曲控件
{
    playlist->previous();//调用基类QMediaPlaylist的方法播放上一曲
}

void MainWidget::on_btnPlayMode_clicked()//点击主窗口中播放模式切换的控件（切换顺序：顺序->随机->单曲循环-》单曲播放一次）
{
    if(playlist->playbackMode()==QMediaPlaylist::Loop){//如果当前播放模式是列表循环（即顺序播放）则点击后需要切换到随机
        ui->btnPlayMode->setStyleSheet(RandomStyle());//修改主窗口中播放模式控件按钮的样式
        ui->btnPlayMode->setToolTip(u8"随机播放");//设置鼠标停留在播放模式按钮上时显示的提示文字为随机播放
        action_systemTray_playmode->setIcon(QIcon(":/image/image/image/random2.png"));//修改系统托盘右键菜单播放模式选项的图标
        action_systemTray_playmode->setText(u8"随机播放");//设置系统托盘右键菜单播放模式选项的提示文字为随机播放
        playlist->setPlaybackMode(QMediaPlaylist::Random);//修改播放器的播放列表的播放模式
    }
    else if(playlist->playbackMode()==QMediaPlaylist::Random){
        ui->btnPlayMode->setStyleSheet(LoopOneStyle());
        ui->btnPlayMode->setToolTip(u8"单曲循环");
        action_systemTray_playmode->setIcon(QIcon(":/image/image/image/loop-one2.png"));
        action_systemTray_playmode->setText(u8"单曲循环");
        playlist->setPlaybackMode(QMediaPlaylist::CurrentItemInLoop);
    }

    else if(playlist->playbackMode()==QMediaPlaylist::CurrentItemInLoop){
        ui->btnPlayMode->setStyleSheet(OnlyOnceStyle());//这里要记得改一下样式啊！(已改√)
        ui->btnPlayMode->setToolTip(u8"单次播放");
        action_systemTray_playmode->setIcon(QIcon(":/image/image/image/onlyonce2.png"));//这里要记得换一下图标啊！(已改√)
        action_systemTray_playmode->setText(u8"单次播放");
        playlist->setPlaybackMode(QMediaPlaylist::CurrentItemOnce);
    }//以上同理
    else if(playlist->playbackMode()==QMediaPlaylist::CurrentItemOnce){
        ui->btnPlayMode->setStyleSheet(LoopStyle());
        ui->btnPlayMode->setToolTip(u8"顺序播放");
        action_systemTray_playmode->setIcon(QIcon(":/image/image/image/loop2.png"));
        action_systemTray_playmode->setText(u8"顺序播放");
        playlist->setPlaybackMode(QMediaPlaylist::Loop);
    }//以上同理
}

void MainWidget::on_btnMin_clicked()//点击主窗口上的最小化按钮控件"-"
{
    showMinimized();//调用基类QWidget的方法让窗口最小化到托盘
}

void MainWidget::on_btnAdd_clicked()//点击主窗口进度条下边那个文件夹按钮控件来添加本地音乐
{
    QFileDialog fileDialog(this);
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);//设置窗口模式为打开文件
    fileDialog.setFileMode(QFileDialog::ExistingFiles);//设置窗口的可选文件为多个或一个文件
    fileDialog.setWindowTitle(tr("添加本地音乐（注：自动过滤，按下\"Ctrl+A\"全选添加即可；不支持添加文件夹）"));//设置窗口上方显示的标题
    QStringList list;list<<"application/octet-stream";//调用重载的"<<"将字符串添加到字符串列表的尾部
    fileDialog.setMimeTypeFilters(list);//过滤掉不符合规定的文件
    fileDialog.setDirectory(QStandardPaths::standardLocations(QStandardPaths::MusicLocation).value(0, QDir::homePath()));//默认从C盘的音乐文件夹打开
    if (fileDialog.exec() == QDialog::Accepted){
       QList<QUrl> urls=fileDialog.selectedUrls();//返回选中的多个文件的资源
       ui->localMusicWidget->musicList.addMusic(urls);//根据这些资源把多首歌曲添加到“本地音乐”列表中
       ui->localMusicWidget->refresh();//把“本地音乐”列表中的条目显示到该窗口上
       ui->stackedWidget->setCurrentIndex(1);//切换到本地音乐窗口
    }
}


void MainWidget::on_btnVolume_clicked()//如果点击主窗口中调节音量的按钮控件切换显示或隐藏
{
    if(ui->volumeSlider->isHidden()){
        ui->volumeSlider->show();
    }else{
        ui->volumeSlider->hide();
    }
}

void MainWidget::on_volumeSlider_valueChanged(int value)//主窗口中Slider控件值改变则改变音量
{
    player->setVolume(value);
}

void MainWidget::on_btnAddMusicList_clicked()//
{
    bool ok;
    QString text=QInputDialog::getText(this,u8"新建歌单",u8"请输入新歌单的名字：",QLineEdit::Normal,"",&ok);//弹出输入新建歌单名字的窗口
    if(ok && !text.isEmpty()){
        MusicList tempMusic;
        tempMusic.setName(text);
        musiclist.push_back(tempMusic);//添加歌曲列表到MainWidget的歌单列表
        namelist_refresh();//把歌单列表项显示到主窗口中的“我的歌单”窗口上
    }
}


void MainWidget::on_btnCurMusic_clicked()
{
    //切换到“当前播放”界面
    ui->stackedWidget->setCurrentIndex(0);
}
void MainWidget::on_btnLocalMusic_clicked()
{
    //切换到“本地音乐”界面
    ui->stackedWidget->setCurrentIndex(1);
}
void MainWidget::on_btnFavorMusic_clicked()
{
    //切换到“我喜欢”界面
    ui->stackedWidget->setCurrentIndex(2);
}

void MainWidget::on_playListWidget_doubleClicked(const QModelIndex &index)//双击当前播放列表中的一个歌曲条目则播放该歌曲
{
    int i=index.row();
    playlist->setCurrentIndex(i);//播放器的播放列表选中双击选择的那首歌
    player->play();//播放该歌曲
}
void MainWidget::on_localMusicWidget_doubleClicked(const QModelIndex &index)//双击本地音乐播放列表中的一个歌曲条目则添加到当前播放列表并在“当前播放窗口”播放该歌曲
{
    playlist->clear();
    ui->localMusicWidget->musicList.addToPlayList(playlist);//添加到当前播放列表
    ui->playListWidget->setMusicList_playing(ui->localMusicWidget->musicList);//把本地音乐播放列表的所有条目放到“当前播放”列表（不存入数据库，下次打开当前播放列表为空）
    int i=index.row();
    QDateTime current_date_time =QDateTime::currentDateTime();//
    QString current_date =current_date_time.toString("yyyy.MM.dd hh:mm:ss");//
    ui->localMusicWidget->musicList.music[i].setcurrent_playedtime(current_date);//
    playlist->setCurrentIndex(i);
    player->play();//以上同理
    ui->stackedWidget->setCurrentIndex(0);//跳转到当前播放列表
}
void MainWidget::on_favorMusicWidget_doubleClicked(const QModelIndex &index)//双击当前播放列表中的一个歌曲条目则播放该歌曲
{
    playlist->clear();
    ui->favorMusicWidget->musicList.addToPlayList(playlist);
    ui->playListWidget->setMusicList_playing(ui->favorMusicWidget->musicList);
    int i=index.row();
    QDateTime current_date_time =QDateTime::currentDateTime();
    QString current_date =current_date_time.toString("yyyy.MM.dd hh:mm:ss");
    ui->favorMusicWidget->musicList.music[i].setcurrent_playedtime(current_date);
    playlist->setCurrentIndex(i);
    player->play();
    ui->stackedWidget->setCurrentIndex(0);//跳转到当前播放列表
}//以上和on_localMusicWidget_doubleClicked同理

void MainWidget::on_nameListWidget_doubleClicked(const QModelIndex &index)//双击“我的歌单”中的条目
{
    ui->stackedWidget->setCurrentIndex(3);//跳转到歌单内容列表
    musiclist_index=index.row();
    musicListWidget_refresh();//把选中的歌单列表中的条目和相关界面元素显示到“歌单”窗口中
}

void MainWidget::on_btnSkin_clicked()
{
    menu_changeSkin->exec(QCursor::pos());
}

void MainWidget::on_btnAddtoMusicList_clicked()//点击主窗口中“歌单”窗口上面那个“＋”按钮控件
{
    MusicListDialog *dialog=new MusicListDialog(this);//打开一个添加歌曲到指定歌单的对话框
    int num=ui->localMusicWidget->count();
    bool *results=new bool[num];//results来标识多选选中的序号（从本地音乐中选择）
    dialog->setMusicList(ui->localMusicWidget->musicList,results);//把本地音乐中的条目显示到“添加歌曲到指定歌单”的对话框中
    if(dialog->exec()==QDialog::Accepted){
        for(int i=0;i<num;i++){
            if(results[i]){
                musiclist[musiclist_index].addMusic(ui->localMusicWidget->musicList.getMusic(i));
            }//把指定歌单中的多选选中的歌曲添加到该歌单的列表中
        }
        musicListWidget_refresh();//把“歌单”窗口列表中的条目显示到该窗口中
    }
    delete []results;//删除动态分配的数组
}

void MainWidget::on_musicListWidget_doubleClicked(const QModelIndex &index)//双击“歌单”窗口中的一个歌曲条目跳转到当前播放并播放该歌曲
{
    playlist->clear();
    musiclist[musiclist_index].addToPlayList(playlist);
    ui->playListWidget->setMusicList_playing(musiclist[musiclist_index]);//以上两行把“当前播放”列表和窗口清空再把歌单中的所有条目显示上去
    int i=index.row();
    QDateTime current_date_time =QDateTime::currentDateTime();
    QString current_date =current_date_time.toString("yyyy.MM.dd hh:mm:ss");
    ui->musicListWidget->musicList.music[i].setcurrent_playedtime(current_date);
    playlist->setCurrentIndex(i);
    player->play();
    ui->stackedWidget->setCurrentIndex(0);//跳转到当前播放列表
}

void MainWidget::on_musicListWidget_customContextMenuRequested(const QPoint &pos)
{
    if(ui->musicListWidget->itemAt(pos)==Q_NULLPTR)
    {
        return;
    }
    menu_musiclist->exec(QCursor::pos());
}

void MainWidget::on_btnAddtoFavor_clicked()//点击“我喜欢”窗口上方那个“+”按钮控件
{
    MusicListDialog *dialog=new MusicListDialog(this);
    int num=ui->localMusicWidget->count();
    bool *results=new bool[num];
    dialog->setMusicList(ui->localMusicWidget->musicList,results);
    if(dialog->exec()==QDialog::Accepted){
        for(int i=0;i<num;i++){
            if(results[i]){
                ui->favorMusicWidget->musicList.addMusic(ui->localMusicWidget->musicList.getMusic(i));
            }
        }
        ui->favorMusicWidget->refresh();
    }
    delete []results;
}//以上和on_btnAddtoMusicList_clicked()同理

//以下按钮标号1、2、3、4分别表示在不同切换窗口中的同样的按钮

void MainWidget::on_btnSortSinger_clicked()
{
    ui->localMusicWidget->musicList.sort_by(AUTHOR);
    ui->localMusicWidget->refresh();
}
void MainWidget::on_btnSortTitle_clicked()
{
    ui->localMusicWidget->musicList.sort_by(TITLE);
    ui->localMusicWidget->refresh();
}
void MainWidget::on_btnSortDuration_clicked()
{
    ui->localMusicWidget->musicList.sort_by(DURATION);
    ui->localMusicWidget->refresh();
}

void MainWidget::on_btnSortSinger_2_clicked()
{
    ui->favorMusicWidget->musicList.sort_by(AUTHOR);
    ui->favorMusicWidget->refresh();
}
void MainWidget::on_btnSortTitle_2_clicked()
{
    ui->favorMusicWidget->musicList.sort_by(TITLE);
    ui->favorMusicWidget->refresh();
}
void MainWidget::on_btnSortDuration_2_clicked()
{
    ui->favorMusicWidget->musicList.sort_by(DURATION);
    ui->favorMusicWidget->refresh();
}

void MainWidget::on_btnSortSinger_4_clicked()
{
    musiclist[musiclist_index].sort_by(AUTHOR);
    musicListWidget_refresh();
}
void MainWidget::on_btnSortTitle_4_clicked()
{
    musiclist[musiclist_index].sort_by(TITLE);
    musicListWidget_refresh();
}
void MainWidget::on_btnSortDuration_4_clicked()
{
    musiclist[musiclist_index].sort_by(DURATION);
    musicListWidget_refresh();
}

void MainWidget::on_btnNeaten_clicked()
{
    ui->localMusicWidget->musicList.neaten();
    ui->localMusicWidget->refresh();
}
void MainWidget::on_btnNeaten_2_clicked()
{
    ui->favorMusicWidget->musicList.neaten();
    ui->favorMusicWidget->refresh();
}
void MainWidget::on_btnNeaten_3_clicked()
{
    musiclist[musiclist_index].neaten();
    musicListWidget_refresh();
}

void MainWidget::on_btnTitle_clicked()
{
    on_btnAbout_clicked();
}

void MainWidget::on_btnLyric_clicked()
{
    ui->stackedWidget->setCurrentIndex(4);
}

void MainWidget::on_btnClear_clicked()
{
    QMessageBox::StandardButton btn;
    btn = QMessageBox::question(this, "提示", "此操作不可逆！\n确实要清空吗?", QMessageBox::Yes|QMessageBox::No);
    if (btn == QMessageBox::Yes) {
        ui->playListWidget->musicList.clear();
        ui->playListWidget->refresh();
        playlist->clear();
    }
}

void MainWidget::on_btnClear_2_clicked()
{
    QMessageBox::StandardButton btn;
    btn = QMessageBox::question(this, "提示", "此操作不可逆！\n确实要清空吗?", QMessageBox::Yes|QMessageBox::No);
    if (btn == QMessageBox::Yes) {
        ui->localMusicWidget->musicList.clear();
        ui->localMusicWidget->refresh();
    }
}

void MainWidget::on_btnClear_3_clicked()
{
    QMessageBox::StandardButton btn;
    btn = QMessageBox::question(this, "提示", "此操作不可逆！\n确实要清空吗?", QMessageBox::Yes|QMessageBox::No);
    if (btn == QMessageBox::Yes) {
        ui->favorMusicWidget->musicList.clear();
        ui->favorMusicWidget->refresh();
    }
}

void MainWidget::on_btnClear_4_clicked()
{
    QMessageBox::StandardButton btn;
    btn = QMessageBox::question(this, "提示", "此操作不可逆！\n确实要清空吗?", QMessageBox::Yes|QMessageBox::No);
    if (btn == QMessageBox::Yes) {
        musiclist[musiclist_index].clear();
        musicListWidget_refresh();
    }
}

void MainWidget::on_btnAbout_clicked()//主窗口上方的“i”按钮
{
    QMessageBox::about(this,u8"关于","LightMusicPlayer | 一款精致小巧的本地音乐播放器\n"
                                   "作者：NJU-TJL\n\n"
                                   "【歌词文件说明】需要与对应的歌曲MP3在同目录且同名（.lry文件）\n"
                                   "【快捷键说明】\n"
                                   "播放/暂停  -  空格\n"
                                   "上一曲/下一曲  -  Alt键+方向键←/→\n"
                                   "【添加本地音乐】可直接拖拽至软件界面内或者点击本地音乐界面的添加按钮（Ctrl键+O）\n"
                                   "【音乐文件类型】添加过程中会自动过滤得到可播放的文件类型（.mp3/.flac/.mpga文件），所以添加时无需考虑文件类型，使用\"Ctrl+A\"选择文件夹内全部文件添加即可\n"
                                   "\n注：鼠标移动到不认识的按钮上，会有说明哦~\n");
}
