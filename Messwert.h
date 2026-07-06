#pragma once
#include <string>

// Ein einzelner Messwert eines Sensors: Zahlenwert + Einheit + Zeitpunkt.
// Unplausible Werte (Stoerungen) werden ueber gueltig = false markiert.
class Messwert {
private:
    double zahlenwert;
    std::string einheit;
    int minuteDesTages;   // Zeitpunkt der Messung in Simulationsminuten (0..1439)
    bool gueltig;

public:
    Messwert();
    Messwert(double zahlenwert, std::string einheit, int minuteDesTages, bool gueltig = true);

    double getZahlenwert() const;
    const std::string& getEinheit() const;
    int getMinuteDesTages() const;
    bool istGueltig() const;

    std::string zeitText() const;   // z.B. "06:45"
};

// Hilfsfunktion: Zahl mit fester Nachkommastellen-Anzahl als Text (z.B. fuer Anzeige und CSV)
std::string alsText(double wert, int nachkommastellen);
