/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_VERSION_H
#define GAME_VERSION_H

// ddnet
#define GAME_NAME "DDNet"
#define DDNET_VERSION_NUMBER 17636
extern const char *GIT_SHORTREV_HASH;
#ifndef GAME_RELEASE_VERSION_INTERNAL
#define GAME_RELEASE_VERSION_INTERNAL 20.1
#endif
#define GAME_RELEASE_VERSION STRINGIFY(GAME_RELEASE_VERSION_INTERNAL)

// teeworlds
#define CLIENT_VERSION7 0x0705
// For compatibility with DDNet client 15.8 and older we need to include the prefix `0.6` in the version string
// because this was used for a "Compatible version" filter in the server browser.
#define GAME_VERSION "0.6, " GAME_RELEASE_VERSION
#define GAME_NETVERSION "0.6 626fce9a778df4d4"
#define GAME_NETVERSION7 "0.7 802f1be60a05665f"

// TClient: the upstream base we build on. Kept for the upstream update check
// and the version message, not shown as this client's own version.
#ifndef TCLIENT_VERSION
#define TCLIENT_VERSION "10.9.0"
#endif

// kizashi
#define CLIENT_NAME "kizashi"
#define CLIENT_LONG_NAME "kizashi client"
#define KIZASHI_VERSION "1.1.1"
// Keep in sync with DDNET_VERSION_NUMBER above; STRINGIFY is not used here so
// the literal stays readable in the one place the version line is built.
#define KIZASHI_DDNET_VERSION "17636"
#define CLIENT_RELEASE_VERSION KIZASHI_VERSION
#define CLIENT_VERSION_STRING CLIENT_LONG_NAME " - " KIZASHI_VERSION " (DDNet " KIZASHI_DDNET_VERSION ")"

#endif
