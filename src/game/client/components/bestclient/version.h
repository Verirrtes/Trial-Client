/* Copyright © 2026 BestProject Team */
#ifndef GAME_CLIENT_COMPONENTS_BESTCLIENT_VERSION_H
#define GAME_CLIENT_COMPONENTS_BESTCLIENT_VERSION_H

#define BESTCLIENT_BUILD_DATE "20.10 17:30"
#define BESTCLIENT_VERSIONNR 100
#define BESTCLIENT_VERSION "1.0.0"

// Trial Client fork marker. Keep BestClient's network identity and official
// code validation unchanged; use this only for local UI decisions.
#define TRIAL_CLIENT 1
#define TRIAL_CLIENT_VERSION BESTCLIENT_VERSION "-trial"

#ifndef BESTCLIENT_OFFICIAL_CODE
#define BESTCLIENT_OFFICIAL_CODE ""
#endif

#endif
