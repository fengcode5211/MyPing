#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUF_SZ 1024
#define PACK_LEN 56

int sendnum = 0;  //发送包编号
int recvnum = 0; 
char sendpack[BUF_SZ];    //发送缓冲区
char recvpack[BUF_SZ];
struct sockaddr_in from;
static long total_time = 0;

void handler(int s)
{
  (void)s;
  printf("--- ping statistics ---\n");
  printf("%d packets transmitted, %d received, %.3f%% packet loss, ttime %ld\n",
          sendnum, recvnum, ((double)(sendnum - recvnum) / sendnum) * 100, total_time); 
  exit(0);
}


long diftime(const struct timeval* end, const struct timeval* bg)    //计算时间（单位--毫秒）
{
  return (end->tv_sec - bg->tv_sec)* 1000 + (end->tv_usec - bg->tv_usec) / 1000;
}

unsigned short chksum(unsigned short* addr, int len)   //校验和
{
  unsigned int ret = 0;
  
  while(len > 1)
  {
    ret += *addr++;
    len -= 2;
  }
  if(len == 1)
  {
    ret += *(unsigned char*)addr;
  }

  ret = (ret >> 16) + (ret & 0xffff);
  ret += (ret >> 16);
  
  return (unsigned short)~ret;
}

int pack(int num, pid_t pid)
{
  memset(sendpack, 0x00, sizeof(sendpack));
  struct icmp* p= (struct icmp*)sendpack;
  p->icmp_type  = ICMP_ECHO;
  p->icmp_code  = 0;
  p->icmp_cksum = 0;
  p->icmp_seq   = num;
  p->icmp_id    = pid;
  struct timeval tval;
  gettimeofday(&tval, NULL);
  memcpy((void*)p->icmp_data, (void*)&tval, sizeof(tval));

  p->icmp_cksum = chksum((unsigned short*)sendpack, PACK_LEN + 8);  //ICMP报头有8个字节
  return PACK_LEN +8;
}

void send_packet(int sfd, pid_t pid, struct sockaddr_in ad)   //发送包
{
  sendnum++;
  int ret = pack(sendnum, pid);
  sendto(sfd,sendpack, ret, 0, (struct sockaddr*)&ad, sizeof(ad));
}

void unpack(char* buf)
{
  //先取前面的ip头部   ，才能取出ICMP的数据
    struct ip* pip = (struct ip*)buf;
    struct icmp* picmp = (struct icmp*)(buf+ (pip->ip_hl << 2));
    struct timeval end;
    gettimeofday(&end, NULL);
    long dif = diftime(&end, (struct timeval*)(picmp->icmp_data));
    total_time += dif;
    printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%ld ms\n",
             PACK_LEN, inet_ntoa(from.sin_addr), picmp->icmp_seq, 
             pip->ip_ttl, dif);

}

void recv_packet(int sfd)  
{
  memset(recvpack, 0x00, sizeof(recvpack));
  socklen_t len = sizeof(from);
  recvnum++;
  ssize_t ret = recvfrom(sfd, recvpack, BUF_SZ, 0, (struct sockaddr*)&from, &len);
  if(ret < 0)
  {
    perror("recvfrom");
    return;
  }
  unpack(recvpack);
}


int main(int argc, char* argv[]) {
  //测试gettimeofday函数的基本用法  
 // struct timeval begin;
 // gettimeofday(&begin, NULL);
 // sleep(1);
 // struct timeval end;
 // gettimeofday(&end, NULL);

 // long ret = diftime(&end, &begin);
 // printf("ret : %ld\n", ret);
  in_addr_t addr;
  struct sockaddr_in ad;

  if(argc != 2)
  {
    printf("Usage: ./ping ip\n");
    return 1;
  }

  signal(SIGINT, handler);
  ad.sin_family = AF_INET;

  if((addr = inet_addr(argv[1])) == INADDR_NONE) //当地址之间转换不出来 即不是ip
  { 
    struct hostent* pent = gethostbyname(argv[1]);
    if(pent == NULL)
    {
      perror("gethostbyname");
      return 1;
    }
    memcpy((char*)&ad.sin_addr, (char*)pent->h_addr, pent->h_length);
  }
  else
  {
    ad.sin_addr.s_addr = addr;
  }

  int sfd = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);    //PF--协议族 AF -- 地址族
  if(sfd == -1)
  {
    perror("socket");
    exit(1);
  }

  pid_t pid = getpid();
  printf("PING %s (%s) %d bytes of data.\n", argv[1], inet_ntoa(ad.sin_addr), PACK_LEN);
  while(1)
  {
    send_packet(sfd, pid, ad);    //发送包
    recv_packet(sfd);
    sleep(1);
  }

  return 0;
}



