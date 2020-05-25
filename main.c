#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/ioctl.h>

#define	StringSize		64
#define HostIPSize      40
#define extra_string_size	159
#define mapnameSize     40
#define hostnameSize    40
#define modlist_string_size	255

#define NO_VTOLS 1
#define NO_TANKS 2
#define NO_BORGS 4

#define recvBuffSize 4096

void delay(int milli_seconds) {
    clock_t start_time = clock();
    while (clock() < start_time + milli_seconds)
        ;
}

char* ConnectReadLobby() {
	int sockfd = 0, n = 0;
	char* recvBuff = (char*)malloc(recvBuffSize);
	struct sockaddr_in serv_addr;
	memset(recvBuff, 'g', recvBuffSize);
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n Error : Could not create socket \n");
		return NULL;
	}
	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(9990);
	if(inet_pton(AF_INET, "88.198.45.216", &serv_addr.sin_addr)<=0) {
		printf("\n inet_pton error occured\n");
		return NULL;
	}
	if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		printf("\n Error : Connect Failed \n");
		return NULL;
	}
	write(sockfd, "list", strlen("list"));
	delay(50000);
	while ( (n = read(sockfd, recvBuff, recvBuffSize-1)) > 0) {
		recvBuff[n] = 0;
		if(fputs(recvBuff, stdout) == EOF) {
			printf("\n Error : Fputs error\n");
		}
	}
	if(n < 0){
	printf("\n Read error \n");
	}
	return recvBuff;
}

int main() {
	char* msg = ConnectReadLobby();
	FILE* lobbyfile;
	lobbyfile = fmemopen(msg, recvBuffSize, "r");
	if(lobbyfile == NULL) {
		printf("File opening error: %s\n", strerror(errno));
		return 1;
	}
	uint32_t gamescount = 0;
	fread(&gamescount, sizeof(uint32_t), 1, lobbyfile);
	gamescount = ntohl(gamescount);
	char msgcount[2000];
	snprintf(msgcount, 2000, "Games in lobby: %d\n", gamescount);
	strcat(msgcount, "`players |        room name          |      host       |            map            |     version    | `\n");
	for(uint32_t gamenumber = 0; gamenumber < gamescount; gamenumber++) {
		uint32_t gamestructversion = 0;
		fread(&gamestructversion, sizeof(uint32_t), 1, lobbyfile);
		gamestructversion = ntohl(gamestructversion);
		
		char gname[StringSize];
		fread(&gname, sizeof(char), StringSize, lobbyfile);
		
		uint32_t dw[2];
		fread(&dw, sizeof(uint32_t), 2, lobbyfile);
		dw[0] = htonl(dw[0]);
		dw[1] = htonl(dw[1]);
		
		char hip[HostIPSize];
		fread(&hip, sizeof(char), HostIPSize, lobbyfile);
		
		uint32_t maxplayers;
		fread(&maxplayers, sizeof(uint32_t), 1, lobbyfile);
		maxplayers = htonl(maxplayers);
		uint32_t currplayers;
		fread(&currplayers, sizeof(uint32_t), 1, lobbyfile);
		currplayers = htonl(currplayers);
		
		uint32_t dwflags[4];
		fread(&dwflags, sizeof(uint32_t), 4, lobbyfile);
		
		char sechosts[2][HostIPSize];
		fread(&sechosts[0], sizeof(char), HostIPSize, lobbyfile);
		fread(&sechosts[1], sizeof(char), HostIPSize, lobbyfile);
		
		char extra[extra_string_size];
		fread(&extra, sizeof(char), extra_string_size, lobbyfile);
		
		char mapname[mapnameSize];
		fread(&mapname, sizeof(char), mapnameSize, lobbyfile);
		
		char hostname[hostnameSize];
		fread(&hostname, sizeof(char), hostnameSize, lobbyfile);
		
		char versionstr[StringSize];
		fread(&versionstr, sizeof(char), StringSize, lobbyfile);
		
		char modlist[modlist_string_size];
		fread(&modlist, sizeof(char), modlist_string_size, lobbyfile);
		
		uint32_t VesionMajor, VersionMinor, IsPrivate, IsPure, Mods, GID, Limits, future1, future2;
		fread(&VesionMajor, sizeof(uint32_t), 1, lobbyfile);
		fread(&VersionMinor, sizeof(uint32_t), 1, lobbyfile);
		fread(&IsPrivate, sizeof(uint32_t), 1, lobbyfile);
		fread(&IsPure, sizeof(uint32_t), 1, lobbyfile);
		fread(&Mods, sizeof(uint32_t), 1, lobbyfile);
		fread(&GID, sizeof(uint32_t), 1, lobbyfile);
		fread(&Limits, sizeof(uint32_t), 1, lobbyfile);
		fread(&future1, sizeof(uint32_t), 1, lobbyfile);
		fread(&future2, sizeof(uint32_t), 1, lobbyfile);
		VesionMajor = htonl(VesionMajor);
		VersionMinor = htonl(VersionMinor);
		IsPrivate = htonl(IsPrivate);
		IsPure = htonl(IsPure);
		Mods = htonl(Mods);
		GID = htonl(GID);
		Limits = htonl(Limits);
		
		char extrastr[2048];
		snprintf(extrastr, 2048, "%s%s%s", IsPrivate ? "Password " : "", IsPure ? "Map-mod " : "",  Limits & NO_VTOLS ? "No VTOL" : "");
		
		char message[2000];
		snprintf(message, 2000, "%2d/%-2d   | %25.25s | %15.15s | %25.25s | %14.14s | %.24s\n", currplayers, maxplayers, gname, hostname, mapname, versionstr, extrastr);
		strcat(msgcount, message);
	}
	uint32_t lobbyCode, motdlen;
	fread(&lobbyCode, sizeof(uint32_t), 1, lobbyfile);
	fread(&motdlen, sizeof(uint32_t), 1, lobbyfile);
	char* motd;
	motd = (char*)malloc(motdlen+1);
	fread(motd, sizeof(char), motdlen, lobbyfile);
	motd[motdlen-1]='\0';
	strcat(msgcount, motd);
	puts(msgcount);
	fclose(lobbyfile);
	return 0;
}
