#include "Anzeige.h"
#include <algorithm>
#include <cmath>
#include <fcntl.h>
#include <iostream>
#include <sys/ioctl.h>
#include <unistd.h>

namespace {
    // ANSI-Farbcodes (256-Farben-Palette)
    const std::string RESET        = "\x1b[0m";
    const std::string FETT         = "\x1b[1m";
    const std::string HELL         = "\x1b[38;5;252m";
    const std::string DIM          = "\x1b[38;5;245m";
    const std::string DUNKEL       = "\x1b[38;5;238m";
    const std::string RAHMEN       = "\x1b[38;5;240m";
    const std::string TITEL        = "\x1b[1;38;5;117m";
    const std::string GELB         = "\x1b[38;5;221m";
    const std::string BLAU         = "\x1b[38;5;111m";
    const std::string GRUEN        = "\x1b[1;38;5;114m";
    const std::string CHIP_AN      = "\x1b[1;38;5;16;48;5;114m";
    const std::string CHIP_AUS     = "\x1b[38;5;252;48;5;238m";
    const std::string CHIP_PAUSE   = "\x1b[1;38;5;16;48;5;221m";
    const std::string CHIP_OFFLINE = "\x1b[1;38;5;231;48;5;131m";

    // Akzentfarbe je Sensor (Temperatur, Licht, Feuchte, dann wieder von vorn)
    const std::string AKZENT[3] = {
        "\x1b[38;5;209m",   // orange
        "\x1b[38;5;221m",   // gelb
        "\x1b[38;5;117m"    // hellblau
    };

    const int MIN_BREITE = 74;
    const int MAX_BREITE = 100;
}

// ---------------------------------------------------------------------------
// TerminalGuard (RAII)
// ---------------------------------------------------------------------------

TerminalGuard::TerminalGuard() {
    istTerminal = isatty(STDIN_FILENO);
    originalFlags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (istTerminal) {
        tcgetattr(STDIN_FILENO, &original);
        termios roh = original;
        roh.c_lflag &= ~(ICANON | ECHO);   // Tasten sofort und ohne Echo lesen
        roh.c_cc[VMIN] = 0;                // read() wartet nicht auf Eingaben
        roh.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &roh);
    } else {
        // stdin ist z.B. eine Pipe: dann nicht-blockierend lesen.
        // Achtung: bei einem echten Terminal darf O_NONBLOCK NICHT gesetzt
        // werden, weil stdin und stdout dieselbe Dateibeschreibung teilen
        // und sonst auch die Bildschirmausgabe verworfen werden kann.
        fcntl(STDIN_FILENO, F_SETFL, originalFlags | O_NONBLOCK);
    }

    // alternativen Bildschirm aktivieren und Cursor verstecken
    std::cout << "\x1b[?1049h\x1b[?25l" << std::flush;
}

TerminalGuard::~TerminalGuard() {
    std::cout << "\x1b[?1049l\x1b[?25h" << std::flush;
    fcntl(STDIN_FILENO, F_SETFL, originalFlags);
    if (istTerminal) {
        tcsetattr(STDIN_FILENO, TCSANOW, &original);
    }
}

// ---------------------------------------------------------------------------
// Anzeige
// ---------------------------------------------------------------------------

Anzeige::Anzeige() : breite(78) {
}

int Anzeige::tasteLesen() {
    char zeichen = 0;
    if (read(STDIN_FILENO, &zeichen, 1) == 1) {
        return zeichen;
    }
    return -1;
}

// Zaehlt nur sichtbare Zeichen: ANSI-Sequenzen und UTF-8-Folgebytes ignorieren
std::size_t Anzeige::sichtbareBreite(const std::string& text) {
    std::size_t anzahl = 0;
    for (std::size_t i = 0; i < text.size(); ++i) {
        unsigned char zeichen = text[i];
        if (zeichen == 0x1b) {
            while (i < text.size() && text[i] != 'm') ++i;
        } else if ((zeichen & 0xC0) != 0x80) {
            ++anzahl;
        }
    }
    return anzahl;
}

std::string Anzeige::kuerzen(const std::string& text, std::size_t maxBreite) {
    if (sichtbareBreite(text) <= maxBreite) {
        return text;
    }
    std::string ergebnis;
    std::size_t anzahl = 0;
    std::size_t i = 0;
    while (i < text.size()) {
        unsigned char zeichen = text[i];
        if (zeichen == 0x1b) {   // Farbcodes komplett uebernehmen
            do {
                ergebnis += text[i];
                ++i;
            } while (i < text.size() && text[i - 1] != 'm');
            continue;
        }
        if ((zeichen & 0xC0) != 0x80) {   // Beginn eines neuen Zeichens
            if (anzahl + 1 >= maxBreite) break;
            ++anzahl;
        }
        ergebnis += text[i];
        ++i;
    }
    return ergebnis + DIM + "…" + RESET;
}

std::string Anzeige::balken(double anteil, int laenge, const std::string& farbe) {
    anteil = std::clamp(anteil, 0.0, 1.0);
    int voll = static_cast<int>(std::lround(anteil * laenge));
    std::string ergebnis = farbe;
    for (int i = 0; i < voll; ++i) ergebnis += "█";
    ergebnis += DUNKEL;
    for (int i = voll; i < laenge; ++i) ergebnis += "░";
    return ergebnis + RESET;
}

// Kleine Verlaufskurve der letzten Messwerte ("Plot" im Terminal)
std::string Anzeige::sparkline(const Sensor& sensor, int laenge) {
    static const char* BLOECKE[8] = {"▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"};
    const std::vector<Messwert>& historie = sensor.getHistorie();
    if (historie.empty() || laenge <= 0) {
        return "";
    }
    std::size_t anzahl = std::min(historie.size(), static_cast<std::size_t>(laenge));
    std::size_t start = historie.size() - anzahl;

    double kleinster = historie[start].getZahlenwert();
    double groesster = kleinster;
    for (std::size_t i = start; i < historie.size(); ++i) {
        kleinster = std::min(kleinster, historie[i].getZahlenwert());
        groesster = std::max(groesster, historie[i].getZahlenwert());
    }

    std::string ergebnis;
    for (std::size_t i = start; i < historie.size(); ++i) {
        int stufe = 3;
        if (groesster > kleinster) {
            double relativ = (historie[i].getZahlenwert() - kleinster) / (groesster - kleinster);
            stufe = static_cast<int>(std::lround(relativ * 7.0));
        }
        ergebnis += BLOECKE[std::clamp(stufe, 0, 7)];
    }
    return ergebnis;
}

void Anzeige::rand(std::string& bild, bool oben) const {
    bild += RAHMEN;
    bild += oben ? "┌" : "└";
    for (int i = 0; i < breite - 2; ++i) bild += "─";
    bild += oben ? "┐" : "┘";
    bild += RESET;
    bild += "\x1b[K\n";
}

void Anzeige::trenner(std::string& bild, const std::string& titel) const {
    bild += RAHMEN;
    bild += "├─";
    int rest = breite - 4;
    if (!titel.empty()) {
        bild += " " + titel + " ";
        rest -= static_cast<int>(titel.size()) + 2;   // Titel ist reines ASCII
    }
    for (int i = 0; i < rest; ++i) bild += "─";
    bild += "─┤";
    bild += RESET;
    bild += "\x1b[K\n";
}

void Anzeige::zeile(std::string& bild, const std::string& inhalt) const {
    std::size_t innen = breite - 4;
    std::string text = kuerzen(inhalt, innen);
    std::size_t belegt = sichtbareBreite(text);
    bild += RAHMEN + "│ " + RESET + text;
    if (belegt < innen) {
        bild += std::string(innen - belegt, ' ');
    }
    bild += RAHMEN + " │" + RESET;
    bild += "\x1b[K\n";
}

void Anzeige::zeileLR(std::string& bild, const std::string& links, const std::string& rechts) const {
    std::size_t innen = breite - 4;
    std::size_t breiteLinks = sichtbareBreite(links);
    std::size_t breiteRechts = sichtbareBreite(rechts);
    std::string inhalt = links;
    if (breiteLinks + breiteRechts < innen) {
        inhalt += std::string(innen - breiteLinks - breiteRechts, ' ');
    } else {
        inhalt += " ";
    }
    inhalt += rechts;
    zeile(bild, inhalt);
}

void Anzeige::zeichnen(const SmartHome& haus, int intervallMs, bool pause) {
    // Breite an das Terminalfenster anpassen
    winsize fenster{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &fenster) == 0 && fenster.ws_col > 0) {
        breite = std::clamp(static_cast<int>(fenster.ws_col), 40, MAX_BREITE);
    }
    if (breite < MIN_BREITE) {
        std::cout << "\x1b[H\x1b[JBitte Terminalfenster vergrößern (mind. "
                  << MIN_BREITE << " Spalten)." << std::flush;
        return;
    }

    std::string bild = "\x1b[H";
    std::size_t innen = breite - 4;

    // Kopfzeile ------------------------------------------------------------
    rand(bild, true);
    std::string linksOben = TITEL + "SMART SENSOR" + RESET + DIM + " · Smart-Home-Simulation" + RESET;
    std::string rechtsOben = FETT + "Tag " + std::to_string(haus.getTag())
                             + " · " + haus.uhrzeitText() + RESET + "  "
                             + (haus.istTag() ? GELB + "TAG" : BLAU + "NACHT") + RESET + "  ";
    if (pause) {
        rechtsOben += CHIP_PAUSE + " PAUSE " + RESET;
    } else {
        rechtsOben += DIM + "Takt " + alsText(intervallMs / 1000.0, 1) + " s" + RESET;
    }
    zeileLR(bild, linksOben, rechtsOben);

    // Sensoren ---------------------------------------------------------------
    trenner(bild, "SENSOREN");
    const std::vector<std::unique_ptr<Sensor>>& sensoren = haus.getSensoren();
    for (std::size_t i = 0; i < sensoren.size(); ++i) {
        const Sensor& sensor = *sensoren[i];
        const std::string& akzent = AKZENT[i % 3];
        int stellen = sensor.getNachkommastellen();

        // Zeile 1: Taste, Name, aktueller Wert, Balken, Status
        std::string wertText = "–";
        double anteil = 0.0;
        if (sensor.hatMesswert()) {
            double wert = sensor.letzterMesswert().getZahlenwert();
            wertText = alsText(wert, stellen);
            anteil = (wert - sensor.getAnzeigeVon())
                     / (sensor.getAnzeigeBis() - sensor.getAnzeigeVon());
        }
        std::string name = kuerzen(sensor.getName(), 22);
        std::string namePad(22 - std::min<std::size_t>(22, sichtbareBreite(name)), ' ');
        std::string wertPad(wertText.size() > 7 ? 0 : 7 - sichtbareBreite(wertText), ' ');
        std::string einheitText = sensor.getEinheit();
        einheitText += std::string(einheitText.size() > 3 ? 0 : 3 - sichtbareBreite(einheitText), ' ');

        int balkenLaenge = std::clamp(static_cast<int>(innen) - 49, 12, 34);
        std::string status = sensor.istInBetrieb() ? GRUEN + " AKTIV " + RESET
                                                   : CHIP_OFFLINE + "OFFLINE" + RESET;

        std::string zeile1 = DIM + "[" + std::to_string(i + 1) + "] " + RESET
                             + akzent + name + RESET + namePad + " "
                             + FETT + wertPad + wertText + RESET + " "
                             + DIM + einheitText + RESET + "  "
                             + balken(anteil, balkenLaenge, akzent) + "  " + status;
        zeile(bild, zeile1);

        // Zeile 2: Statistik (min / Mittelwert / max), Offset und Verlauf
        std::string statistik;
        if (sensor.hatMesswert()) {
            double offset = sensor.getOffset();
            statistik = DIM + "    min " + alsText(sensor.minimum(), stellen)
                        + " · Ø " + alsText(sensor.mittelwert(), stellen)
                        + " · max " + alsText(sensor.maximum(), stellen)
                        + " · Offset " + (offset >= 0 ? "+" : "") + alsText(offset, stellen)
                        + RESET;
        } else {
            statistik = DIM + "    noch keine gültigen Messwerte" + RESET;
        }
        int kurvenLaenge = static_cast<int>(innen) - static_cast<int>(sichtbareBreite(statistik)) - 2;
        std::string kurve = akzent + sparkline(sensor, std::clamp(kurvenLaenge, 10, 40)) + RESET;
        zeileLR(bild, statistik, kurve);
    }

    // Aktoren ----------------------------------------------------------------
    trenner(bild, "AKTOREN");
    std::string aktorenZeile;
    for (const std::unique_ptr<Aktor>& aktor : haus.getAktoren()) {
        aktorenZeile += HELL + aktor->getName() + RESET + " "
                        + (aktor->istAn() ? CHIP_AN + " AN " : CHIP_AUS + " AUS ") + RESET
                        + DUNKEL + " ×" + std::to_string(aktor->getSchaltvorgaenge()) + RESET + "   ";
    }
    zeile(bild, aktorenZeile);

    // Ereignisse -------------------------------------------------------------
    trenner(bild, "EREIGNISSE");
    const std::vector<std::string>& ereignisse = haus.getEreignisse();
    for (std::size_t i = 0; i < 5; ++i) {
        if (i < ereignisse.size()) {
            zeile(bild, (i == 0 ? HELL : DIM) + ereignisse[i] + RESET);
        } else {
            zeile(bild, "");
        }
    }

    // Fusszeile --------------------------------------------------------------
    trenner(bild, "");
    std::string tasten = DIM + "[1-3]" + RESET + " Sensor an/aus  "
                         + DIM + "[k]" + RESET + " Kalibrieren  "
                         + DIM + "[+/-]" + RESET + " Tempo  "
                         + DIM + "[Leer]" + RESET + " Pause  "
                         + DIM + "[q]" + RESET + " Ende";
    zeile(bild, tasten);
    rand(bild, false);

    bild += "\x1b[J";
    std::cout << bild << std::flush;
}
