// user/log_test.c  – multi-process logger w/ buffer-full demo
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

#define PGSIZE         4096
#define MAX_CHILDREN   10      // number of children to fork
#define HEAVY_WRITER   0       // child index that spams
#define HEAVY_WRITES   300     // spam iterations
#define YIELD_EVERY    10      // heavy writer sleep(0) every N msgs
#define MSG_LEN        64      // max payload length (no NUL)

/* 32-bit header: low 16 = len, high 16 = child idx */
struct log_header { uint16 len, idx; };

/* ---- helpers ----------------------------------------------------- */
static void itoa(int n, char *buf) {
  int i=0; if(n==0){buf[i++]='0';buf[i]=0;return;}
  while(n){buf[i++]='0'+n%10;n/=10;} buf[i]=0;
  for(int j=0;j<i/2;j++){char t=buf[j];buf[j]=buf[i-1-j];buf[i-1-j]=t;}
}
static int append(char *dst,int pos,const char*s){while(*s)dst[pos++]=*s++;return pos;}
static inline uint64 align4(uint64 x){return (x+3)&~3ULL;}

/* ---- main -------------------------------------------------------- */
int main(void){
  /* parent allocates one page-aligned log buffer */
  uint64 cur=(uint64)sbrk(0);
  uint64 base=(cur+PGSIZE-1)&~(PGSIZE-1);
  sbrk(base-cur+PGSIZE);
  char *logbuf=(char*)base;
  memset(logbuf,0,PGSIZE);

  int p2c[MAX_CHILDREN][2], pid[MAX_CHILDREN];
  /* fork children -------------------------------------------------- */
  for(int idx=0;idx<MAX_CHILDREN;idx++){
    pipe(p2c[idx]);
    if((pid[idx]=fork())==0){
      /* ---------------- CHILD ---------------- */
      close(p2c[idx][1]);
      uint64 shared; if(read(p2c[idx][0],&shared,8)!=8) exit(1);
      close(p2c[idx][0]);
      uint64 page_end=(shared&~(PGSIZE-1))+PGSIZE;

      for(int seq=0;;seq++){
        if(idx!=HEAVY_WRITER&&seq>0) break;
        if(idx==HEAVY_WRITER&&seq>=HEAVY_WRITES) break;

        /* build message */
        char msg[MSG_LEN], nbuf[8], sbuf[8];
        itoa(idx,nbuf); itoa(seq,sbuf);
        int pos=0;
        pos=append(msg,pos,"Hello from child ");
        pos=append(msg,pos,nbuf);
        if(idx==HEAVY_WRITER){                    // tag spam msgs
          pos=append(msg,pos," msg ");
          pos=append(msg,pos,sbuf);
        }
        msg[pos]=0; int len=pos;

        /* scan from top each attempt */
        uint8 *p=(uint8*)align4(shared);
        for(;;){
          if((uint64)p+sizeof(struct log_header)+len>page_end){
            exit(0);
          }
          uint32 packed=(idx<<16)|len;
          if(__sync_val_compare_and_swap((uint32*)p,0,packed)==0){
            memmove(p+sizeof(struct log_header),msg,len);
            if(idx==HEAVY_WRITER&&seq%YIELD_EVERY==YIELD_EVERY-1)
              sleep(0);               /* quick yield every N msgs */
            break;
          }
          uint16 skip=((struct log_header*)p)->len;
          if(skip==0) skip=1;
          p+=sizeof(struct log_header)+skip;
          p=(uint8*)align4((uint64)p);
        }
      }
      exit(0);
    }
    /* -------------- PARENT continues loop -------------- */
    close(p2c[idx][0]);
  }

  /* map buffer into children: everyone **except** heavy writer first */
  for(int i=0;i<MAX_CHILDREN;i++)
    if(i!=HEAVY_WRITER){
      uint64 a=map_shared_pages(logbuf,PGSIZE,pid[i]);
      write(p2c[i][1],&a,8); close(p2c[i][1]);
    }
  /* heavy writer last */
  {
    int i=HEAVY_WRITER;
    uint64 a=map_shared_pages(logbuf,PGSIZE,pid[i]);
    write(p2c[i][1],&a,8); close(p2c[i][1]);
  }

  /* parent prints first msg per child, stops when all children exit */
  uint8 *scan=(uint8*)align4(base); uint64 end=base+PGSIZE;
  int seen[MAX_CHILDREN]={0}, live=MAX_CHILDREN, printed=0;
  while(live>0){
    uint8 *p=scan;
    while((uint64)p+sizeof(struct log_header)<end){
      struct log_header *h=(struct log_header*)p;
      uint16 len=h->len, idx=h->idx;
      if(len&&idx<MAX_CHILDREN&&!seen[idx]){
        if((uint64)p+sizeof(struct log_header)+len>end) break;
        char buf[MSG_LEN+1];
        memmove(buf,p+sizeof(struct log_header),len);
        buf[len]=0;
        printf("Parent read from child %d: %s\n",idx,buf);
        seen[idx]=1; printed++;
        if(printed==MAX_CHILDREN) break;
      }
      p+=len?sizeof(struct log_header)+len:sizeof(struct log_header);
      p=(uint8*)align4((uint64)p);
    }
    int wpid; while((wpid=wait(0))>0) live--;
    sleep(1);
  }
  exit(0);
}
