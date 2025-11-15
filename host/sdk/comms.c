#include <sdk/game.h>
#include <sdk/panic.h>
#include <sdk/comms.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

static int rf_sock;
static int rf_channel = 42;
static struct sockaddr_in rf_addr;

static bool debug = true;

bool sdk_send_ir(uint32_t word)
{
	(void)word;
	return true;
}

void sdk_decode_ir_raw(int16_t sample, int *oI, int *oQ, int *dm)
{
	(void)sample;
	(void)oI;
	(void)oQ;
	(void)dm;
}

void sdk_decode_ir(int16_t sample)
{
	return sdk_decode_ir_raw(sample, NULL, NULL, NULL);
}

bool sdk_send_rf(uint8_t addr, const uint8_t *data, int len)
{
	static uint32_t last_tx = 0;
	uint32_t now = time_us_32();

	if (now - last_tx < 1000) {
		printf("\x1b[1;31mcomms/rf: sending too fast!\x1b[0m\n");
		return false;
	}

	if (debug) {
		printf("\x1b[1;33m[%i] -> (%i) %02hhx: ", rf_channel, len, addr);

		for (int i = 0; i < len; i++)
			printf("%02hhx", data[i]);

		printf("\x1b[0m\n");
	}

	last_tx = now;

	uint8_t buf[6 + len];

	int pid = getpid();
	memcpy(buf, &pid, 4);
	buf[4] = rf_channel;
	buf[5] = addr;
	memcpy(buf + 6, data, len);

	if (0 > sendto(rf_sock, buf, 6 + len, 0, (struct sockaddr *)&rf_addr, sizeof(rf_addr)))
		return false;

	return true;
}

void sdk_comms_poll(void)
{
	uint8_t buf[6 + SDK_RF_MAX];

	while (true) {
		int len = recv(rf_sock, buf, sizeof(buf), MSG_DONTWAIT);
		if (len < 6)
			return;

		int pid;
		memcpy(&pid, buf, 4);

		if (pid == getpid())
			continue;

		if (debug)
			printf("\x1b[1;34m[%hhu] <- (%i) ", buf[4], len - 6);

		if (rf_channel != buf[4]) {
			if (debug)
				printf(" wrong-channel\x1b[0m\n");

			continue; /* Reject different channels. */
		}

		if (!(rand() >> 27)) {
			if (debug)
				printf(" \x1b[1;31mdropped\x1b[0m\n");

			continue; /* Simulate packet loss. */
		}

		uint8_t addr = buf[5];

		if (debug)
			printf("%02hhx: ", addr);

		sdk_message_t msg = {
			.type = SDK_MSG_RF,
			.rf = {
				.addr = addr,
				.data = buf + 6,
				.length = len - 6,
			},
		};

		if (debug) {
			for (int i = 0; i < msg.rf.length; i++)
				printf("%02hhx", msg.rf.data[i]);

			printf("\x1b[0m\n");
		}

		game_inbox(msg);
	}
}

bool sdk_set_rf_channel(int ch)
{
	if (ch < 1 || ch > 69)
		return false;

	rf_channel = ch;
	return true;
}

void sdk_comms_init(void)
{
	if ((rf_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		sdk_panic("socket() failed");

	int reuse = 1;
	setsockopt(rf_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	setsockopt(rf_sock, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));

	int broadcast = 1;
	setsockopt(rf_sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

	// LISTEN on all IPs
	struct sockaddr_in bind_addr = { 0 };
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_port = htons(13571);
	bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(rf_sock, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0)
		sdk_panic("bind failed");

	// SEND to LAN broadcast address
	rf_addr.sin_family = AF_INET;
	rf_addr.sin_port = htons(13570);
	rf_addr.sin_addr.s_addr = inet_addr("255.255.255.255");
}
