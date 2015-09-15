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

#ifdef DEBUG
#define LOG(x, ...) printf(x, ##__VA_ARGS__)
#else
#define LOG(x, ...) if (verbose) { printf(x, ##__VA_ARGS__); }
#endif
#define ERROR(x, ...) fprintf(stderr, x, ##__VA_ARGS__)

static int sockfd_udp;
static struct sockaddr_in forwarder_addr;
static int queue_num = 0;
static int verbose = 0;
static int enable_checksum = 0;

void print_help() {
	printf("Usage:\n"
		"\tdpgateway --forwarder | -f forwarder --port | -p port [--verbose | -v] [--checksum] [--queue num]\n");
}

void parse_arguments(int argc, char **argv)
{
	int i = 1;
	u_int8_t flag = 0b11;
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
			print_help();
			exit(0);
		} else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--forwarder") == 0) {
			if (i + 1 < argc) {
				forwarder_addr.sin_addr.s_addr = inet_addr(argv[i + 1]);
				i++;
				flag &= 0b01;
			} else {
				ERROR("Invalid arguments.\n");
				print_help();
				exit(0);
			}
		} else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) {
			if (i + 1 < argc) {
				forwarder_addr.sin_port = htons(atoi(argv[i + 1]));
				i++;
				flag &= 0b10;
			} else {
				ERROR("Invalid arguments.\n");
				print_help();
				exit(0);
			}
		} else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
			verbose = 1;
		} else if (strcmp(argv[i], "--checksum") == 0) {
			enable_checksum = 1;
		} else if (strcmp(argv[i], "--queue") == 0) {
			if (i + 1 < argc) {
				queue_num = atoi(argv[i + 1]);
				i++;
			} else {
				ERROR("Invalid arguments.\n");
				print_help();
				exit(0);
			}
		}
	}

	if(flag) {
		ERROR("Invalid arguments.\n");
		print_help();
		exit(0);
	}
}

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
	sendto(sockfd_udp, data, len, 0, (struct sockaddr *) &forwarder_addr, sizeof(forwarder_addr));
}

static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa, void *data)
{

	struct nfqnl_msg_packet_hdr *ph;
	u_int32_t id = 0;
	u_int32_t pkg_data_len;
	unsigned char *pkg_data;
//	char nat_cmd[256];
//	struct in_addr nat_src = {0};
//	struct in_addr nat_dst = {0};
//	char nat_src_str[16];
//	char nat_dst_str[16];
//	char *nat_wan_str = "";
//	unsigned short nat_sport = 0;
//	unsigned short nat_dport = 0;

	LOG("entering callback\n");

	ph = nfq_get_msg_packet_hdr(nfa);
	if (ph)
	{
		id = ntohl(ph->packet_id);
		LOG("hw_protocol=0x%04x hook=%u id=%u ", ntohs(ph->hw_protocol),	ph->hook, id);
	}

	pkg_data_len = nfq_get_payload(nfa, &pkg_data);
	if (pkg_data_len >= 0)
	{
		LOG("payload_len=%d ", pkg_data_len);
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
		((struct iphdr*) pkg_data)->check = ip_cksum((unsigned short *) pkg_data, 20);
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

	bzero(&forwarder_addr, sizeof(forwarder_addr));
	forwarder_addr.sin_family = AF_INET;

	parse_arguments(argc, argv);

	LOG("opening UDP socket\n");
	sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd_udp == -1)
	{
		ERROR("error during socket()\n");
		exit(1);
	}

	LOG("opening library handle\n");
	h = nfq_open();
	if (!h)
	{
		ERROR("error during nfq_open()\n");
		exit(1);
	}

	LOG("unbinding existing nf_queue handler for AF_INET (if any)\n");
	if (nfq_unbind_pf(h, AF_INET) < 0)
	{
		ERROR("error during nfq_unbind_pf()\n");
		exit(1);
	}

	LOG("binding nfnetlink_queue as nf_queue handler for AF_INET\n");
	if (nfq_bind_pf(h, AF_INET) < 0)
	{
		ERROR("error during nfq_bind_pf()\n");
		exit(1);
	}

	LOG("binding this socket to queue '%d'\n", queue_num);
	qh = nfq_create_queue(h, queue_num, &cb, NULL );
	if (!qh)
	{
		ERROR("error during nfq_create_queue()\n");
		exit(1);
	}

	LOG("setting copy_packet mode\n");
	if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0)
	{
		ERROR("can't set packet_copy mode\n");
		exit(1);
	}

	fd = nfq_fd(h);

	while ((rv = recv(fd, buf, sizeof(buf), 0)) && rv >= 0)
	{
		LOG("pkt received\n");
		nfq_handle_packet(h, buf, rv);
	}

	LOG("unbinding from queue 0\n");
	nfq_destroy_queue(qh);

#ifdef INSANE
	/* normally, applications SHOULD NOT issue this command, since
	 * it detaches other programs/sockets from AF_INET, too ! */
	LOG("unbinding from AF_INET\n");
	nfq_unbind_pf(h, AF_INET);
#endif

	LOG("closing library handle\n");
	nfq_close(h);

	return 0;
}
