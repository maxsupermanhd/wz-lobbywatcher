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

#include <string>

#include "wz-lobbyreader/lobbyreader.h"

#define	StringSize		64
#define HostIPSize      40
#define extra_string_size	159
#define mapnameSize     40
#define hostnameSize    40
#define modlist_string_size	255

#define NO_VTOLS 1
#define NO_TANKS 2
#define NO_BORGS 4

#define recvBuffSize 4096*3

int interval = 2;
int notify = 0;
int notifynopass = 0;
int compact = 0;
int timeout = 2;

void delay(int milli_seconds) {
    clock_t start_time = clock();
    while (clock() < start_time + milli_seconds)
        ;
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
	printf("Lobby watcher v1.0 [use Ctr-C to stop]\n");
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
		} else if(mequalstr(argv[argcounter], "-w")) {
			if(argcounter+1 >= argc) {
				printf("Number expected after -w\n");
				exit(1);
			}
			timeout = atoi(argv[argcounter+1]);
			argcounter++;
		} else if(mequalstr(argv[argcounter], "-c")) {
			compact = 1;
		} else if(mequalstr(argv[argcounter], "--help") || mequalstr(argv[argcounter], "-h")) {
			printf("   Usage: %s [args]\n", argv[0]);
			printf("   Available args:\n");
			printf("   \n");
			printf("   == general ==\n");
			printf("   -h    [--help]  Shows this page.\n");
			printf("   -t <dealy>      Delay between refresh. (integer)\n");
			printf("   -w <timeout>    Seconds to timeout lobby connection.\n");
			printf("   -notify         Enable new room notification.\n");
			printf("   -n-nopass       Don't notify for passworded rooms.\n");
			printf("   -c              Compact view.\n");
			printf("\n");
			exit(0);
		} else {
			printf("Undefined argument [%s]\n", argv[argcounter]);
			exit(1);
		}
	}
	return 0;
}

char* dupesc(char* sstr) {
	char* ret = (char*)malloc(1024);
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

// from warzone2100/lib/framework/string_ext.h
template <typename... P>
static inline std::string astringf(char const *format, P &&... params) {
	int len = snprintf(nullptr, 0, format, std::forward<P>(params)...);
	if (len <= 0) {
		return {};
	}
	std::string str;
	str.resize(len + 1);
	snprintf(&str[0], len + 1, format, std::forward<P>(params)...);
	str.resize(len);
	return str;
}

int main(int argc, char** argv) {
	setbuf(stdout, 0);
	ArgParse(argc, argv);
	//printf("Initializing player databse...\n");
	InitPlayersLog();
	signal(SIGINT, CatchInterrupt);
	uint32_t GIDhistory[128] = {0};
	int GIDhistcount = -1;
	LobbyResponse l;
	while(1) {
		printf("Reading lobby...\n");
		if(GetLobby(&l, timeout) == LOBBYREADER_FAIL) {
			printf("Can not read lobby responce!\n");
			WritePlayers();
			return 1;
		}
		uint32_t GIDhistorynew[128] = {0};
		int GIDhistcountnew = 0;
		ClearScreen();
		std::string msg = astringf("Games in lobby: %d\n", l.rooms.size());
		if(!compact) {
			msg += "players |        room name          |      host       |            map            |     version    | \n";
		}
		for(uint32_t gamenumber = 0; gamenumber < l.rooms.size(); gamenumber++) {
			LobbyGame game = l.rooms[gamenumber];
			std::string extrastr;
			if(compact) {
				extrastr = astringf("%s%s %s:%d", game.privateGame ? "P" : ".", game.privateGame ? "M" : ".", game.host.c_str(), game.hostPort);
			} else {
				extrastr = astringf("%s%s%s %s:%d", game.privateGame ? "Password " : "", game.privateGame ? "Map-mod " : "",  game.dwFlags & NO_VTOLS ? "No VTOL" : "", game.host.c_str(), game.hostPort);
			}
			char message[2000];
			if(compact) {
				msg += astringf("╔%2d/%-2d Map: [%-25.25s][%-39.39s]\n╚     Host: [%-25.25s](%-16.16s) %s\n",
					game.dwCurrentPlayers, game.dwMaxPlayers, game.mapname.c_str(), game.name.c_str(), game.hostname.c_str(), game.versionstring.c_str(), extrastr.c_str());
			} else {
				msg += astringf("%2d/%-2d   | %25.25s | %15.15s | %25.25s | %14.14s | %s\n",
					game.dwCurrentPlayers, game.dwMaxPlayers, game.name.c_str(), game.hostname.c_str(), game.mapname.c_str(), game.versionstring.c_str(), extrastr.c_str());
			}
			LogPlayer((char*)game.hostname.c_str(), (char*)game.host.c_str());

			GIDhistorynew[GIDhistcountnew] = game.gameId;
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
						if(notifynopass && !game.privateGame) {
							break;
						}
						char* messagecmd = (char*)malloc(512);
						for(int i=0; i<512; i++) {messagecmd[i] = '\0';}
						char* escapedmapname = dupesc((char*)game.mapname.c_str());
						char* escapedgname = dupesc((char*)game.name.c_str());
						char* escapedhostname = dupesc((char*)game.hostname.c_str());
						snprintf(messagecmd, 511, "notify-send \"New room\" \"%s [%s] (%d/%d) by %s\"", escapedmapname, escapedgname, game.dwCurrentPlayers, game.dwMaxPlayers, escapedhostname);
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
		msg += l.motd.c_str();
		puts(msg.c_str());
		sleep(interval);
	}
	return 0;
}
