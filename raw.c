///////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>


int main() {
  int sfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if(sfd < 0)
  {
    perror("socket");
    return 1;
  }
  
  int opt = 1;
  setsockopt(sfd, IPPROTO_IP, IP_HDRINCL, &opt, sizeof(opt));

  char buf[1500];
  while(1)
  {
    memset(buf, 0x00, sizeof(buf));
    int ret = read(sfd, buf, 1500);
    if(ret <= 0)
    {
      break;
    }

    struct iphdr* pip = (struct iphdr*)(buf);
    struct in_addr ad;

    ad.s_addr = pip->saddr;
    //printf("protocol: %hhd, %s <-----> ", pip->protocol, inet_ntoa(ad));
    printf("数据报为%s <===>",inet_ntoa(ad));
    ad.s_addr = pip->daddr;
    printf("%s\n", inet_ntoa(ad));

  }
  return 0;
}

