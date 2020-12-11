#include "MusicList.h"
#include <QCoreApplication>
#include <QProgressDialog>
#include <QDesktopServices>
#include <QtSql>
#include <algorithm>
#include "MusicListWidget.h"

MusicList::MusicList(const QList<QUrl> &urls, QString iname)
{
    addMusic(urls);
    setName(iname);
}

void MusicList::addMusic(const QList<QUrl> &urls)
{
    //实测这里耗时较长，所以添加一个进度显示对话框
    QProgressDialog proDialog(u8"添加进度",u8"取消",0,urls.size());//u8前缀保证编码为utf8保证中文显示正确
    proDialog.setMinimumSize(350,150);
    proDialog.setWindowModality(Qt::WindowModal);
    proDialog.setWindowTitle("添加中...请稍后");
    proDialog.show();
    int x=0;
    foreach (QUrl i, urls) {
        x++;
        proDialog.setValue(x);//设置加载窗口显示的进度+1
        //过滤Url的类型
        QMimeDatabase db;
        QMimeType mime = db.mimeTypeForFile(i.toLocalFile());
        if(mime.name()!="audio/mpeg"&&mime.name()!="audio/flac"){
            continue;
        }
        //剩下的符合类型
        music.push_back(Music(i));
        if(sql_flag){
            music[music.size()-1].insertSQL(name);
        }
        if(proDialog.wasCanceled()) break;
    }
}

void MusicList::addMusic(const Music &iMusic)
{
    music.push_back(iMusic);//用vector类中的pushback方法添加一个Music对象到music数组后面
    if(sql_flag){
        music[music.size()-1].insertSQL(name);
    }
}

Music MusicList::getMusic(int pos)
{
    return music[pos];
}

void MusicList::addToPlayList(QMediaPlaylist *playlist)
{
    for(auto i=music.begin();i!=music.end();i++){
        playlist->addMedia(i->getUrl());
    }
}

void MusicList::addToListWidget(MusicListWidget *listWidget)
{
    foreach(const Music &i,music){
        QListWidgetItem *item = new QListWidgetItem;
        item->setIcon(listWidget->getIcon());
        item->setText(i.getInfo());
        listWidget->addItem(item);
    }
}

void MusicList::removeMusic(int pos)
{
    if(sql_flag){
        remove_SQL_all();
        int i=0;
        for(auto it=music.begin();it!=music.end();){
            if(i==pos){
                it= music.erase(it);
                break;
            }
            else{
                it++;
            }
            i++;

        }
        insert_SQL_all();
    }else{
        int i=0;
        for(auto it=music.begin();it!=music.end();){
            if(i==pos){
                it= music.erase(it);
                break;
            }
            else{
                it++;
            }
            i++;

        }
    }

}

void MusicList::showInExplorer(int pos)
{
    QString str=music[pos].getUrl().toString();
    str.remove(str.split("/").last());//切割字符串获得所在的文件夹路径
    QDesktopServices::openUrl(str);
}

void MusicList::detail(int pos)
{
    music[pos].detail();
}

void MusicList::remove_SQL_all()
{
    QSqlQuery sql_query;
    QString delete_sql = "delete from MusicInfo where name = ?";
    sql_query.prepare(delete_sql);
    sql_query.addBindValue(name);
    sql_query.exec();
}

void MusicList::insert_SQL_all()
{
    for(auto i=music.begin();i!=music.end();i++){
        i->insertSQL(name);
    }
}

void MusicList::read_fromSQL()//遍历指定歌单的MusicInfo表,每次查询把从数据库中查到的歌曲信息赋给Music对象并添加到音乐列表的尾部
{
    QSqlQuery sql_query;
    QString select_sql = "select url, author, title, duration, albumTitle, audioBitRate from MusicInfo where name = ?";
    sql_query.prepare(select_sql);
    sql_query.addBindValue(name);//根据name关键字(歌单名例如“当前播放”“我喜欢”等)查询信息并赋值
    if(sql_query.exec())
    {
        while(sql_query.next())//如果表不为空
        {
            Music tempMusic;
            tempMusic.url=QUrl(sql_query.value(0).toString());
            tempMusic.author=sql_query.value(1).toString();
            tempMusic.title=sql_query.value(2).toString();
            tempMusic.duration=sql_query.value(3).toLongLong();
            tempMusic.albumTitle=sql_query.value(4).toString();
            tempMusic.audioBitRate=sql_query.value(5).toInt();
            music.push_back(tempMusic);//把tempMusic添加到MusicList尾部
        }
    }
}

void MusicList::sort_by(COMPARE key)
{
    sort(music.begin(),music.end(),MusicCompare(key));
    if(sql_flag){
        remove_SQL_all();
        insert_SQL_all();
    }
}

void MusicList::neaten()
{
    sort(music.begin(),music.end(),MusicCompare(DEFAULT));
    music.erase(unique(music.begin(),music.end(),MusicCompare(EQUALITY)), music.end());
    if(sql_flag){
        remove_SQL_all();
        insert_SQL_all();
    }
}

void MusicList::clear()
{
    music.clear();
    if(sql_flag){
        remove_SQL_all();
    }
}
