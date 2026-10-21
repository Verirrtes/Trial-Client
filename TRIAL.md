# Trial Client

Trial Client is a personal, modified fork of [BestClient](https://github.com/BestProjectTeam/BestClient),
which itself is based on [DDNet](https://github.com/ddnet/ddnet) and TClient.

Created by **Verirrtes**.

This fork is not affiliated with or endorsed by the BestClient, DDNet or TClient projects.

## Changes in this fork

- Rebranded user-visible strings to "Trial Client" (version 1.0.0; indicator
  announcements use the `1.0.0-trial` label).
- Disabled the built-in auto-updater so the client cannot replace itself with an
  official BestClient build.
- Added the creator card in **Settings -> Trial -> Info** and the default skin
  `icey-nanami`.
- Ported selected quality-of-life modules from Entity Client:
  - **Map overview**: a subtle trail of the path you have taken, with an opacity
    slider and an option to show it only while spectating.
  - **Performance statistics**: compact FPS, ping and snapshot-rate counters.
  - **Player actions**: a radial menu that runs configurable commands for the
    nearest tee. Bind `+playeractions` in **Settings -> Controls -> Binds**.
    Actions are stored in `trial_player_actions.cfg`.
- Added local timestamps to chat messages, optionally configurable and shown in
  the chat log and copied chat text.
- Added friend-online notifications: a four-second notification in the top-right
  corner, plus a chat line and desktop notification. Friends are detected on the
  current server, on other servers known to the server browser, and through the
  global BestClient/Trial presence snapshot, so the notification is not limited
  to the server you are currently playing on. The notification says whether the
  friend joined this server or another one, never fires for friends who were
  already online when the client started, and hides the friend name while
  streamer mode is enabled.
- Added particle trails behind tees: snowflakes, sparkles or a mixed style, with
  configurable length, size and opacity. They are enabled by default, can be
  shown for other players, and replace the classic ribbon trail while active.
  Settings are in **Settings -> Trial -> Particle Trails**
  (`tc_trial_particle_trail*`).
- The main-menu logo uses the original square **Trial T** artwork, centered above
  the menu without stretching. The bottom-left start-menu shortcut list only
  contains a clickable **Discord** button linking to the Trial community server.
- Redesigned the **Settings -> Trial** page with a branded header, two-column
  layout, clearer section cards and a direct Discord invite.
- Extended streamer mode to hide player nicknames, clans, chat contents and
  server addresses across chat, name plates, scoreboard, spectator list,
  killfeed, chat bubbles, flag-carrier HUD and server-info menus.
- Added configurable screenshot and demo-recording keys in
  **Settings -> Trial -> Settings -> Trial QoL**. The demo key uses the
  `tc_toggle_demo` command to start and stop a manual demo with one bind.
- Replaced Discord Rich Presence branding with **Trial Client** and the Trial
  logo. The activity name is sent explicitly over Discord's local IPC bridge, so
  Discord displays **Trial Client** even when the configured application ID is
  registered under another name. Discord RPC is always enabled and its settings
  are locked in the UI; the application ID and large-image URL can still be
  overridden directly with `tc_trial_discord_app_id` and
  `tc_trial_discord_large_image`.
- The in-game music overlay uses the Trial logo instead of the BestClient logo.
- Added a creator credit line to the credits page.
- Suppressed BestClient's local "fake" client-indicator badge for your own Trial
  clients.
- Disabled BestClient indicator presence announcement by default, so other
  BestClient users do not see Trial players as fake. While announcement is
  disabled they also do not see a BestClient/Trial badge for those players.
  It can be re-enabled in **Settings -> Trial -> Others -> Client Indicator**.
- The server browser is Trial-branded: its tab and indicator column header use
  the Trial artwork. The **Trial players** counter, aggregate server icon and
  per-player list include only players whose version label contains `trial`;
  BestClient players are not counted or listed as Trial. The scoreboard,
  spectator list and name plates use the same strict version check and always
  mark your own local client as Trial. Developer and FAKE warning icons remain
  available in their dedicated browser column.
- Added optional trajectory previews for the selected weapon:
  - **Grenade** shows the ballistic arc until it hits the map.
  - **Laser** shows the full ray path including ricochets.
  - **Shotgun** shows the ray path on DDRace servers and the five-pellet spread
    on vanilla servers.
  The preview is configured in **Settings -> Trial -> Trajectory preview**
  (`tc_trial_trajectory_grenade`, `tc_trial_trajectory_laser`,
  `tc_trial_trajectory_shotgun`, `tc_trial_trajectory_alpha`,
  `tc_trial_trajectory_width`) and is disabled by default.
- Added a **Weapon size** slider (`tc_weapon_size`, 0-300%) in
  **Settings -> Trial -> Trial QoL** and in the TClient visual settings. It
  scales weapon and muzzle sprites cosmetically without changing gameplay.
- Added an **FPS Spoofer** module in **Settings -> Trial** that visually
  multiplies the FPS counters from 2.0x to 10.0x in 0.1 steps. It only changes
  the displayed numbers and does not affect real frame rate or gameplay
  (`tc_trial_fps_spoofer`, `tc_trial_fps_spoofer_multiplier`).
- Hotkeys changed through the Trial settings UI are written to the config
  immediately, so they survive closing the client without a clean shutdown.

Future ports will keep the same rule: only quality-of-life features, no
automation, anti-cheat bypasses, fake statistics or proof spoofing. The optional
trajectory preview is a local visual guide and is disabled by default.

Internal identifiers, configuration keys, asset paths and network protocol strings
are intentionally left unchanged for compatibility with existing configs, data
folders and the BestClient master server.

## Building

Run `configure.bat`, then `build_client.bat`, then optionally `package_client.bat`.
The packaged client is written to `dist/Trial-1.0.0-win64.zip`.

## Licensing

- BestClient is licensed under the MIT License (see `LICENSE`).
- DDNet, TClient and the original Teeworlds/DDRace code are licensed under the
  zlib license (see `license.txt`).

All original copyright notices are retained. This file marks the project as a
modified version as required by those licenses.
