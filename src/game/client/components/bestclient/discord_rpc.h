/* Copyright © 2026 BestProject Team */
#ifndef GAME_CLIENT_COMPONENTS_BESTCLIENT_DISCORD_RPC_H
#define GAME_CLIENT_COMPONENTS_BESTCLIENT_DISCORD_RPC_H

class IDiscord;

IDiscord *CreateBestClientDiscord();

// Trial: the Discord SDK translation unit cannot include the full config
// header on Windows because it clashes with the COM IStorage interface.
// These getters are implemented in bestclient.cpp, which already includes
// the config.
const char *TrialDiscordAppId();
const char *TrialDiscordLargeImage();
bool TrialDiscordStreamerMode();

#endif
