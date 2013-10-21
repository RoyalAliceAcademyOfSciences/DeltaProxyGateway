#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <linux/netfilter.h>            /* for NF_ACCEPT */
#include <libnetfilter_queue/libnetfilter_queue.h>

static int sockfd;
static struct sockaddr_in servaddr;

static unsigned short ip_cksum(unsigned short *addr, int len)
{
	unsigned short cksum;
	unsigned int sum = 0;

	while (len > 1)
	{
		sum += *addr++;
		len -= 2;
	}
	if (len == 1)
		sum += *(unsigned char*) addr;
	sum = (sum >> 16) + (sum & 0xffff);  //把高位的进位，加到低八位，其实是32位加法
	sum += (sum >> 16);  //add carry
	cksum = ~sum;   //取反
	return (cksum);
}

static void send_udp(const unsigned char *data, uint16_t len)
{
	sendto(sockfd, data, len, 0, (struct sockaddr *) &servaddr,
			sizeof(servaddr));
}

static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
		struct nfq_data *nfa, void *data)
{

	struct nfqnl_msg_packet_hdr *ph;
	u_int32_t id = 0;
	u_int32_t pkg_data_len;
	unsigned char *pkg_data;
	char nat_cmd[256];
	struct in_addr nat_src = {0};
	struct in_addr nat_dst = {0};
	char nat_src_str[16];
	char nat_dst_str[16];
	char *nat_wan_str = "";
	unsigned short nat_sport = 0;
	unsigned short nat_dport = 0;

	printf("entering callback\n");

	ph = nfq_get_msg_packet_hdr(nfa);
	if (ph)
	{
		id = ntohl(ph->packet_id);
		printf("hw_protocol=0x%04x hook=%u id=%u ", ntohs(ph->hw_protocol),
				ph->hook, id);
	}

	pkg_data_len = nfq_get_payload(nfa, &pkg_data);
	if (pkg_data_len >= 0)
	{
		printf("payload_len=%d ", pkg_data_len);
		//TODO add NAT
		//TODO get ip and port from packet

//		bzero(nat_cmd, sizeof(nat_cmd));
//
//		bzero(&nat_src, sizeof(nat_src));
//		bzero(&nat_dst, sizeof(nat_dst));
//		nat_src.s_addr = ((struct iphdr*) pkg_data)->saddr;
//		nat_dst.s_addr = ((struct iphdr*) pkg_data)->daddr;
//		strcpy(nat_src_str, inet_ntoa(nat_src));
//		strcpy(nat_dst_str, inet_ntoa(nat_dst));
//
//		nat_sport = ((struct tcphdr*)(pkg_data+sizeof(struct iphdr)))->source;
//		nat_dport = ((struct tcphdr*)(pkg_data+sizeof(struct iphdr)))->dest;
//
//		sprintf(nat_cmd,
//				"conntrack --create --protonum tcp --timeout 120 --state SYN_SENT --orig-src %s --orig-dst %s --reply-src %s --reply-dst %s --sport %d --dport %d",
//				nat_src_str, nat_dst_str, nat_dst_str, nat_wan_str, nat_sport, nat_dport);
//
//		if(((struct tcphdr*)(pkg_data+sizeof(struct iphdr)))->syn == 1)
//		{
//			if(system(nat_cmd) == -1)
//				printf("error: system(nat_cmd)\n");
//			printf("\n%s\n", nat_cmd);
//		}

		//send packets to forwarder
		send_udp(pkg_data, pkg_data_len);

		//packet unable to reach GFW
		((struct iphdr*) pkg_data)->ttl = 2;
		((struct iphdr*) pkg_data)->check = 0;
		((struct iphdr*) pkg_data)->check = ip_cksum(
				(unsigned short *) pkg_data, 20);
	}

//	return nfq_set_verdict(qh, id, NF_DROP, 0, NULL );
//	sleep(1);
	return nfq_set_verdict(qh, id, NF_ACCEPT, pkg_data_len, pkg_data);
}

int main(int argc, char **argv)
{
	struct nfq_handle *h;
	struct nfq_q_handle *qh;
	int fd;
	int rv;
	char buf[4096] __attribute__ ((aligned));

	printf("opening socket\n");
	if (argc != 3)
	{
		printf("usage:  dpgd <IP address> <Port>\n");
		exit(1);
	}
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd == -1)
	{
		fprintf(stderr, "error during socket()\n");
		exit(1);
	}
	else
	{
		bzero(&servaddr, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = inet_addr(argv[1]);
		servaddr.sin_port = htons(atoi(argv[2]));
	}

	printf("opening library handle\n");
	h = nfq_open();
	if (!h)
	{
		fprintf(stderr, "error during nfq_open()\n");
		exit(1);
	}

	printf("unbinding existing nf_queue handler for AF_INET (if any)\n");
	if (nfq_unbind_pf(h, AF_INET) < 0)
	{
		fprintf(stderr, "error during nfq_unbind_pf()\n");
		exit(1);
	}

	printf("binding nfnetlink_queue as nf_queue handler for AF_INET\n");
	if (nfq_bind_pf(h, AF_INET) < 0)
	{
		fprintf(stderr, "error during nfq_bind_pf()\n");
		exit(1);
	}

	printf("binding this socket to queue '0'\n");
	qh = nfq_create_queue(h, 0, &cb, NULL );
	if (!qh)
	{
		fprintf(stderr, "error during nfq_create_queue()\n");
		exit(1);
	}

	printf("setting copy_packet mode\n");
	if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0)
	{
		fprintf(stderr, "can't set packet_copy mode\n");
		exit(1);
	}

	fd = nfq_fd(h);

	while ((rv = recv(fd, buf, sizeof(buf), 0)) && rv >= 0)
	{
		printf("pkt received\n");
		nfq_handle_packet(h, buf, rv);
	}

	printf("unbinding from queue 0\n");
	nfq_destroy_queue(qh);

#ifdef INSANE
	/* normally, applications SHOULD NOT issue this command, since
	 * it detaches other programs/sockets from AF_INET, too ! */
	printf("unbinding from AF_INET\n");
	nfq_unbind_pf(h, AF_INET);
#endif

	printf("closing library handle\n");
	nfq_close(h);

	exit(0);
}
