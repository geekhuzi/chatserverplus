#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <string>
#include <cstring>      // strncpy
#include <sys/sem.h>    // SEM_UNDO
#include <sys/shm.h>    // shmget shmat shmdt
#include "log.h"

using namespace std;

// 信号量。
class csemp
{
private:
    union semun  // 用于信号量操作的共同体。
    {
      int val;
      struct semid_ds *buf;
      unsigned short  *arry;
    };

    int   m_semid;         // 信号量id（描述符）。

    // 如果把sem_flg设置为SEM_UNDO，操作系统将跟踪进程对信号量的修改情况，
    // 在全部修改过信号量的进程（正常或异常）终止后，操作系统将把信号量恢复为初始值。
    // 如果信号量用于互斥锁，设置为SEM_UNDO。
    // 如果信号量用于生产消费者模型，设置为0。
    short m_sem_flg;

    csemp(const csemp &) = delete;                      // 禁用拷贝构造函数。
    csemp &operator=(const csemp &) = delete;  // 禁用赋值函数。
public:
    csemp():m_semid(-1){}

    // 如果信号量已存在，获取信号量；如果信号量不存在，则创建它并初始化为value。
    // 如果用于互斥锁，value填1，sem_flg填SEM_UNDO。
    // 如果用于生产消费者模型，value填0，sem_flg填0。
    bool init(key_t key,unsigned short value=1,short sem_flg=SEM_UNDO);
    bool wait(short value=-1);    // 信号量的P操作，如果信号量的值是0，将阻塞等待，直到信号量的值大于0。
    bool post(short value=1);     // 信号量的V操作。
    int  getvalue();                       // 获取信号量的值，成功返回信号量的值，失败返回-1。
    bool destroy();                       // 销毁信号量。
    ~csemp();
};

// 进程心跳信息的结构体。
struct st_procinfo
{
    int      pid=0;                      // 进程id。
    char   pname[51]={0};        // 进程名称，可以为空。
    int      timeout=0;              // 超时时间，单位：秒。
    time_t atime=0;                 // 最后一次心跳的时间，用整数表示。
    st_procinfo() = default;     // 有了自定义的构造函数，编译器将不提供默认构造函数，所以启用默认构造函数。
    st_procinfo(const int in_pid,const string & in_pname,const int in_timeout, const time_t in_atime)
                    :pid(in_pid),timeout(in_timeout),atime(in_atime) { strncpy(pname,in_pname.c_str(),50); }
};

// 以下几个宏用于进程的心跳。
#define MAXNUMP     1000     // 最大的进程数量。
#define SHMKEYP    0x5095     // 共享内存的key。
#define SEMKEYP     0x5095     // 信号量的key。

// 查看共享内存：  ipcs -m
// 删除共享内存：  ipcrm -m shmid
// 查看信号量：      ipcs -s
// 删除信号量：      ipcrm sem semid

// 进程心跳操作类。
class cpactive
{
 private:
     int  m_shmid;                   // 共享内存的id。
     int  m_pos;                       // 当前进程在共享内存进程组中的位置。
     st_procinfo *m_shm;        // 指向共享内存的地址空间。

 public:
     cpactive();  // 初始化成员变量。

     // 把当前进程的信息加入共享内存进程组中。
     bool addpinfo(const int timeout,const string &pname="",clogfile *logfile=nullptr);

     // 更新共享内存进程组中当前进程的心跳时间。
     bool uptatime();

     ~cpactive();  // 从共享内存中删除当前进程的心跳记录。
};

class cpactive;
extern cpactive pactive;
#endif