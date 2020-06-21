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
#include <signal.h>
#include <unistd.h>

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

int interval = 2;
int notify = 0;
int notifynopass = 0;

void delay(int milli_seconds) {
    clock_t start_time = clock();
    while (clock() < start_time + milli_seconds)
        ;
}

char* ConnectReadLobby() {
	int sockfd = 0;
	//printf("Allocating receive buffer... ");
	char* recvBuff = (char*)malloc(recvBuffSize);
	//printf("DONE!\nAllocating socket... ");
	struct sockaddr_in serv_addr;
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n Error : Could not create socket \n");
		return NULL;
	}
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(9990);
	if(inet_pton(AF_INET, "88.198.45.216", &serv_addr.sin_addr)<=0) {
		printf("\n inet_pton error occured\n");
		return NULL;
	}
	//printf("DONE!\nConnecting... ");
	if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		printf("\n Error : Connect Failed \n");
		return NULL;
	}
	//printf("DONE!\nRequesting list... ");
	write(sockfd, "list", strlen("list"));
	//printf("DONE!\nReading answer... ");
	int n = 0, s = 0, t = 0;
	while(1) {
		n = read(sockfd, recvBuff+s, recvBuffSize-1-s);
		if(n>0)
			s+=n;
		else
			break;
	}
	recvBuff[s]='\0';
	close(sockfd);
	//printf("DONE!\n");
	return recvBuff;
}

struct player {
	char name[hostnameSize];
	int ipcount;
	char *ips;
} *players;
int playerscount;

void InitPlayersLog() {
	int check = -3;
	FILE* f = fopen("players.known", "r");
	if(f == NULL) {
		players = NULL;
		playerscount = 0;
	} else {
		char magic[4] = {'g'};
		check = fread(magic, 1, 4, f);
		if(check != 4)
			printf("MAGIC READ\n");
		if(strncmp(magic, "plkn", 4) != 0)
			printf("WRONG MAGIC! %c%c%c%c\n", magic[0], magic[1], magic[2], magic[3]);
		if(interval<1) {exit(0);}
		check = fread(&playerscount, sizeof(int), 1, f);
		if(check != 1)
			printf("FREAD1\n");
		if(playerscount > 0) {
			players = (struct player*)malloc(playerscount*sizeof(struct player));
		} else {
			players = NULL;
		};
		for(int i=0; i<playerscount; i++) {
			check = fread(players[i].name, sizeof(char), hostnameSize, f);
			if(check != hostnameSize)
				printf("FREAD2\n");
			check = fread(&players[i].ipcount, sizeof(int), 1, f);
			players[i].ips = (char*)malloc(players[i].ipcount * sizeof(char)*HostIPSize);
			if(players[i].ips == NULL) {
				printf("READ ALLOC FAIL\n");
				exit(1);
			}
			if(check != 1)
				printf("FREAD3\n");
			check = fread(players[i].ips, sizeof(char)*HostIPSize, players[i].ipcount, f);
			if(check != players[i].ipcount)
				printf("FREAD4\n");
		}
		fclose(f);
	}
}

void LogPlayer(char* name, char* ip) {
	int foundname = 0;
	for(int i=0; i<playerscount; i++) {
		if(strcmp(players[i].name, name) == 0) {
			foundname = 1;
			int foundip = 0;
			for(int j=0; j<players[i].ipcount; j++) {
				if(strcmp(players[i].ips+j*HostIPSize, ip) == 0) {
					foundip = 1;
					break;
				}
			}
			if(!foundip) {
				if((players[i].ips = (char*)realloc(players[i].ips, (players[i].ipcount+1) * sizeof(char)*40)) == NULL) {
					printf("realloc fail!\n");
					exit(1);
				}
				strncpy(players[i].ips + players[i].ipcount*HostIPSize, ip, HostIPSize);
				players[i].ipcount++;
			}
			break;
		}
	}
	if(foundname == 0) {
		if((players = (struct player*)realloc(players, (playerscount+1) * sizeof(struct player))) == NULL) {
			printf("realloc fail!\n");
			exit(1);
		}
		strncpy(players[playerscount].name, name, hostnameSize);
		players[playerscount].ips = (char*)malloc(sizeof(char)*40);
		if(players[playerscount].ips == NULL) {
			printf("Malloc error!\n");
			abort();
		}
		players[playerscount].ipcount = 1;
		strncpy(players[playerscount].ips + 0, ip, HostIPSize);
		playerscount++;
	}
}

void WritePlayers() {
	printf("\n\nWrirting to file %d users:\n", playerscount);
	for(int i=0; i<playerscount; i++) {
		printf("[%-*s] with ips: ", hostnameSize, players[i].name);
		for(int j=0; j<players[i].ipcount; j++) {
			printf("[%-*s]", 15, players[i].ips + j*HostIPSize);
		}
		printf("\n");
	}
	FILE* f = fopen("players.known", "w");
	if(f == NULL) {
		printf("Error opening file on writing!\n");
		exit(1);
	}
	fwrite("plkn", sizeof(char), 4, f);
	fwrite(&playerscount, sizeof(int), 1, f);
	for(int i=0; i<playerscount; i++) {
		fwrite(players[i].name, sizeof(char), hostnameSize, f);
		fwrite(&players[i].ipcount, sizeof(int), 1, f);
		for(int j=0; j<players[i].ipcount; j++) {
			fwrite(players[i].ips + j*HostIPSize, sizeof(char)*40, 1, f);
		}
	}
	fclose(f);
	return;
}

void FreePlayers() {
	for(int i=0; i<playerscount; i++) {
		free(players[i].ips);
	}
	free(players);
}

void CatchInterrupt(int s) {
	printf("\n\nExiting and writing ips to file...");
	WritePlayers();
	FreePlayers();
	exit(0);
}

void ClearScreen() {
	printf("\033[H\033[J");
	printf("Lobby watcher v0.1 [use Ctr-C to stop]\n");
}

int mequalstr(char* trg, const char* chk) {
	if(strlen(trg)!=strlen(chk))
		return 0;
	for(unsigned int c=0; c<strlen(trg); c++)
		if(trg[c]!=chk[c])
			return 0;
	return 1;
}

int ArgParse(int argc, char **argv) {
	for(int argcounter=1; argcounter<argc; argcounter++) {
		if(mequalstr(argv[argcounter], "-t")) {
			if(argcounter+1 >= argc) {
				printf("Number expected after -t\n");
				exit(1);
			}
			interval = atoi(argv[argcounter+1]);
			argcounter++;
		} else if(mequalstr(argv[argcounter], "-notify")) {
			notify = 1;
		} else if(mequalstr(argv[argcounter], "-n-nopass")) {
			notifynopass = 1;
		} else if(mequalstr(argv[argcounter], "--help") || mequalstr(argv[argcounter], "-h")) {
			printf("   Usage: %s [args]\n", argv[0]);
			printf("   Available args:\n");
			printf("   \n");
			printf("   == general ==\n");
			printf("   -h    [--help]  Shows this page.\n");
			printf("   -t <dealy>      Delay between refresh. (integer)\n");
			printf("   -notify         Enable new room notification.\n");
			printf("   -n-nopass       Don't notify for passworded rooms.\n");
			printf("\n");
			exit(0);
		}
	}
	return 0;
}

char* dupesc(char* sstr) {
	char* ret = malloc(1024);
	int retc = 0;
	for(int i=0; i<strlen(sstr); i++) {
		if(sstr[i] != '\\' || sstr[i] != '\'' || sstr[i] != '\"') {
			ret[retc] = sstr[i];
			retc++;
		}
	}
	ret[retc] = '\0';
	return ret;
}

int main(int argc, char** argv) {
	setbuf(stdout, 0);
	ArgParse(argc, argv);
	//printf("Initializing player databse...\n");
	InitPlayersLog();
	signal(SIGINT, CatchInterrupt);
	uint32_t GIDhistory[128] = {0};
	int GIDhistcount = -1;
	while(1) {
		ClearScreen();
		//printf("Connecting to lobby...\n");
		char* msg = ConnectReadLobby();
		if(msg == NULL) {
			printf("Can not read lobby responce!\n");
			return 1;
		}
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
		uint32_t GIDhistorynew[128] = {0};
		int GIDhistcountnew = 0;
		strcat(msgcount, "players |        room name          |      host       |            map            |     version    | \n");
		for(uint32_t gamenumber = 0; gamenumber < gamescount; gamenumber++) {
			uint32_t gamestructversion = 0;
			fread(&gamestructversion, sizeof(uint32_t), 1, lobbyfile);
			gamestructversion = ntohl(gamestructversion);

			char gname[StringSize]; //game name
			fread(&gname, sizeof(char), StringSize, lobbyfile);

			uint32_t dw[2];
			fread(&dw, sizeof(uint32_t), 2, lobbyfile);
			dw[0] = htonl(dw[0]); //size of hostname
			dw[1] = htonl(dw[1]);

			char hip[HostIPSize]; // host ip
			fread(&hip, sizeof(char), HostIPSize, lobbyfile);

			uint32_t maxplayers;
			fread(&maxplayers, sizeof(uint32_t), 1, lobbyfile);
			maxplayers = htonl(maxplayers);
			uint32_t currplayers;
			fread(&currplayers, sizeof(uint32_t), 1, lobbyfile);
			currplayers = htonl(currplayers);

			uint32_t dwflags[4]; // unused
			fread(&dwflags, sizeof(uint32_t), 4, lobbyfile);

			char sechosts[2][HostIPSize]; // secondary ips, unused
			fread(&sechosts[0], sizeof(char), HostIPSize, lobbyfile);
			fread(&sechosts[1], sizeof(char), HostIPSize, lobbyfile);

			char extra[extra_string_size]; // idk what is this, unused
			fread(&extra, sizeof(char), extra_string_size, lobbyfile);

			char mapname[mapnameSize]; // map name
			fread(&mapname, sizeof(char), mapnameSize, lobbyfile);

			char hostname[hostnameSize]; // host nickname
			fread(&hostname, sizeof(char), hostnameSize, lobbyfile);

			char versionstr[StringSize]; // version string
			fread(&versionstr, sizeof(char), StringSize, lobbyfile);

			char modlist[modlist_string_size]; // modlist
			fread(&modlist, sizeof(char), modlist_string_size, lobbyfile);

			uint32_t VesionMajor, VersionMinor, IsPrivate, IsPure, Mods, GID, Limits, future1, future2;
			fread(&VesionMajor, sizeof(uint32_t), 1, lobbyfile);
			fread(&VersionMinor, sizeof(uint32_t), 1, lobbyfile);
			fread(&IsPrivate, sizeof(uint32_t), 1, lobbyfile);
			fread(&IsPure, sizeof(uint32_t), 1, lobbyfile); // map-mod
			fread(&Mods, sizeof(uint32_t), 1, lobbyfile); // mods count
			fread(&GID, sizeof(uint32_t), 1, lobbyfile); // game id
			fread(&Limits, sizeof(uint32_t), 1, lobbyfile); // not working
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
			snprintf(extrastr, 2048, "%s%s%s %s", IsPrivate ? "Password " : "", IsPure ? "Map-mod " : "",  Limits & NO_VTOLS ? "No VTOL" : "", hip);

			char message[2000];
			snprintf(message, 2000, "%2d/%-2d   | %25.25s | %15.15s | %25.25s | %14.14s | %.24s\n", currplayers, maxplayers, gname, hostname, mapname, versionstr, extrastr);
			strcat(msgcount, message);
			LogPlayer(hostname, hip);

			GIDhistorynew[GIDhistcountnew] = GID;
			GIDhistcountnew++;
			if(GIDhistcount != -1) {
				for(int i=0; i<GIDhistcountnew; i++) {
					int found = 0;
					for(int j=0; j<GIDhistcount; j++) {
						if(GIDhistory[j] == GIDhistorynew[i]) {
							found = 1;
							break;
						}
					}
					if(!found && notify) {
						if(notifynopass && !IsPrivate) {
							break;
						}
						char messagecmd[512] = {0};
						char* escapedmapname = dupesc(mapname);
						char* escapedgname = dupesc(gname);
						char* escapedhostname = dupesc(hostname);
						snprintf(messagecmd, 511, "notify-send \"New room\" \"%s [%s] (%d/%d) by %s\"", escapedmapname, escapedgname, currplayers, maxplayers, escapedhostname);
						system(messagecmd);
						free(escapedmapname);
						free(escapedgname);
						free(escapedhostname);
						break;
					}
				}
			}

		}
		for(int i=0; i<GIDhistcountnew; i++) {
			GIDhistory[i] = GIDhistorynew[i];
		}
		GIDhistcount = GIDhistcountnew;
		uint32_t lobbyCode, motdlen;
		fread(&lobbyCode, sizeof(uint32_t), 1, lobbyfile);
		fread(&motdlen, sizeof(uint32_t), 1, lobbyfile);
		char* motd = NULL;
		if(motdlen>0) {
			motd = (char*)malloc(motdlen+1);
			fread(motd, sizeof(char), motdlen, lobbyfile);
			motd[motdlen-1]='\0';
			strcat(msgcount, motd);
		} else {
			printf("len %ld\n", strlen(msg));
			if(msg == NULL)
				printf("NULL\n");
			for(int i=1; i<strlen(msg); i++) {
				printf("%02x ",msg[i-1]);
				if(i % 8 == 0 && i != 0)
					printf(" ");
				if(i % 16 == 0 && i != 0)
					printf(" | [%.*s]\n", 16, msg+(i-1)-16);
				if(i+1 == strlen(msg))
					printf(" | [%.*s]\n", i%16, msg+(i-1)-i%16);
			}
		}
		puts(msgcount);
		fclose(lobbyfile);
		free(msg);
		free(motd);
		sleep(interval);
	}
	return 0;
}
