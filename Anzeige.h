#pragma once
#include <string>
#include <termios.h>
#include "SmartHome.h"

// Versetzt das Terminal beim Erzeugen in den "Raw-Modus" (Tasten ohne Enter
// lesen, kein Echo, eigener Bildschirm) und stellt im Destruktor alles
// wieder her. Klassisches RAII: Lebensdauer des Objekts = Dauer der Anzeige.
class TerminalGuard {
private:
    termios original;
    int originalFlags;
    bool istTerminal;

public:
    TerminalGuard();
    ~TerminalGuard();
};

// Zeichnet das Dashboard des Smart Homes mit ANSI-Escape-Sequenzen ins
// Terminal (Farben, Balken, Verlaufskurven) und liest Tastendruecke ein.
// Die Anzeige liest das SmartHome nur (const-Referenz) und veraendert nichts.
class Anzeige {
private:
    int breite;   // aktuelle Zeichenbreite des Dashboards

    static std::size_t sichtbareBreite(const std::string& text);
    static std::string kuerzen(const std::string& text, std::size_t maxBreite);
    static std::string balken(double anteil, int laenge, const std::string& farbe);
    static std::string sparkline(const Sensor& sensor, int laenge);

    void rand(std::string& bild, bool oben) const;
    void trenner(std::string& bild, const std::string& titel) const;
    void zeile(std::string& bild, const std::string& inhalt) const;
    void zeileLR(std::string& bild, const std::string& links, const std::string& rechts) const;

public:
    Anzeige();

    void zeichnen(const SmartHome& haus, int intervallMs, bool pause);

    // liefert die gedrueckte Taste oder -1, wenn keine Eingabe vorliegt
    static int tasteLesen();
};
