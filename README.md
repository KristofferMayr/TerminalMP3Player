# TerminalMP3Player

Terminal-MP3-Player in C – spielt MP3s ab, verwaltet Playlists und läuft komplett im Terminal.

## Projektstruktur

```
TerminalMP3Player/
├── README.md
├── src/
│   ├── core/          Fehlercodes, Logging, Konfiguration/Tastenbelegung (US-13, US-15)
│   ├── audio/         Dekodierung (minimp3), Audioausgabe (PortAudio),
│   │                  Ringpuffer, Player-Threads (US-01 – US-06, US-09)
│   ├── library/       Songbibliothek & Suche, Playlist, ID3-Tags (US-07, US-08, US-10, US-12)
│   ├── ui/            ncurses-Oberfläche, Fortschrittsbalken, Visualisierung (US-10, US-11, US-14)
│   └── app/           Hauptschleife, verbindet Tastatureingaben mit Aktionen
├── tests/             Unit-Tests
├── docs/              Anforderungen (User Stories, Pitch) und Dokumentation
├── config/            Konfigurationsdateien (z. B. Tastenbelegung)
└── third_party/
    └── minimp3/       Header-only MP3-Decoder
```
