#include "Sensor.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace {
    const double PI = 3.14159265358979;

    // Gleichverteilte Zufallszahl im Bereich [von, bis]
    double zufall(double von, double bis) {
        static std::mt19937 generator(std::random_device{}());
        std::uniform_real_distribution<double> verteilung(von, bis);
        return verteilung(generator);
    }
}

// ---------------------------------------------------------------------------
// Basisklasse Sensor
// ---------------------------------------------------------------------------

Sensor::Sensor(std::string name, std::string einheit,
               double bereichVon, double bereichBis,
               double anzeigeVon, double anzeigeBis, int nachkommastellen)
    : name(name), einheit(einheit), betriebszustand(true), offset(0.0),
      bereichVon(bereichVon), bereichBis(bereichBis),
      anzeigeVon(anzeigeVon), anzeigeBis(anzeigeBis),
      nachkommastellen(nachkommastellen) {
}

Sensor::~Sensor() {
}

Messwert Sensor::messen(int minuteDesTages) {
    if (!betriebszustand) {
        return Messwert();   // ausser Betrieb -> ungueltiger Standard-Messwert
    }

    // Seltene Stoerung: der Sensor liefert einen voellig unplausiblen Wert
    if (zufall(0.0, 1.0) < 0.012) {
        double spanne = bereichBis - bereichVon;
        double stoerwert = (zufall(0.0, 1.0) < 0.5)
                               ? bereichVon - zufall(0.1, 0.6) * spanne
                               : bereichBis + zufall(0.1, 0.6) * spanne;
        return Messwert(stoerwert, einheit, minuteDesTages, false);
    }

    // Der Offset driftet langsam, bis wieder kalibriert wird
    double spanne = bereichBis - bereichVon;
    offset += spanne * zufall(-0.0015, 0.002);
    offset = std::clamp(offset, -0.04 * spanne, 0.04 * spanne);

    double wert = simuliereRohwert(minuteDesTages) + offset;

    // Plausibilitaetspruefung gegen den zulaessigen Bereich
    bool gueltig = (wert >= bereichVon && wert <= bereichBis);
    Messwert messwert(wert, einheit, minuteDesTages, gueltig);

    if (gueltig) {
        historie.push_back(messwert);
        if (historie.size() > MAX_HISTORIE) {
            historie.erase(historie.begin());
        }
    }
    return messwert;
}

bool Sensor::kalibrieren() {
    offset = 0.0;
    return true;
}

void Sensor::setBetriebszustand(bool an) {
    betriebszustand = an;
}

void Sensor::beeinflussen(bool) {
    // Standard: ein Aktor hat keine Rueckwirkung auf diesen Sensor
}

const std::string& Sensor::getName() const { return name; }
const std::string& Sensor::getEinheit() const { return einheit; }
bool Sensor::istInBetrieb() const { return betriebszustand; }
double Sensor::getOffset() const { return offset; }
double Sensor::getAnzeigeVon() const { return anzeigeVon; }
double Sensor::getAnzeigeBis() const { return anzeigeBis; }
int Sensor::getNachkommastellen() const { return nachkommastellen; }
const std::vector<Messwert>& Sensor::getHistorie() const { return historie; }

bool Sensor::hatMesswert() const {
    return !historie.empty();
}

const Messwert& Sensor::letzterMesswert() const {
    return historie.back();
}

double Sensor::minimum() const {
    if (historie.empty()) return 0.0;
    double kleinster = historie[0].getZahlenwert();
    for (const Messwert& messwert : historie) {
        if (messwert.getZahlenwert() < kleinster) {
            kleinster = messwert.getZahlenwert();
        }
    }
    return kleinster;
}

double Sensor::maximum() const {
    if (historie.empty()) return 0.0;
    double groesster = historie[0].getZahlenwert();
    for (const Messwert& messwert : historie) {
        if (messwert.getZahlenwert() > groesster) {
            groesster = messwert.getZahlenwert();
        }
    }
    return groesster;
}

double Sensor::mittelwert() const {
    if (historie.empty()) return 0.0;
    double summe = 0.0;
    for (const Messwert& messwert : historie) {
        summe += messwert.getZahlenwert();
    }
    return summe / historie.size();
}

// ---------------------------------------------------------------------------
// TemperaturSensor
// ---------------------------------------------------------------------------

TemperaturSensor::TemperaturSensor(std::string name)
    : Sensor(name, "°C", 5.0, 45.0, 12.0, 30.0, 1), heizEffekt(0.0) {
}

void TemperaturSensor::beeinflussen(bool heizungAn) {
    if (heizungAn) {
        heizEffekt = std::min(4.0, heizEffekt + 0.18);   // Raum waermt sich auf
    } else {
        heizEffekt = std::max(0.0, heizEffekt - 0.12);   // Raum kuehlt langsam ab
    }
}

double TemperaturSensor::simuliereRohwert(int minuteDesTages) {
    // Tagesgang: Minimum 16.5 °C gegen 04:30 Uhr, Maximum 26.5 °C gegen 16:30 Uhr
    double tagesgang = 21.5 - 5.0 * std::cos(2.0 * PI * (minuteDesTages - 270) / 1440.0);
    return tagesgang + heizEffekt + zufall(-0.25, 0.25);
}

// ---------------------------------------------------------------------------
// LichtSensor
// ---------------------------------------------------------------------------

LichtSensor::LichtSensor(std::string name)
    : Sensor(name, "lx", 0.0, 2000.0, 0.0, 1000.0, 0) {
}

double LichtSensor::simuliereRohwert(int minuteDesTages) {
    double licht = zufall(0.0, 3.0);   // nachts nur Restlicht
    if (minuteDesTages >= 360 && minuteDesTages <= 1260) {   // 06:00 - 21:00 Uhr
        double sonnenstand = std::sin(PI * (minuteDesTages - 360) / 900.0);
        double wolken = 0.8 + 0.2 * std::sin(minuteDesTages * 0.045);
        licht += 900.0 * std::pow(sonnenstand, 1.4) * wolken + zufall(0.0, 10.0);
    }
    return licht;
}

// ---------------------------------------------------------------------------
// FeuchtigkeitsSensor
// ---------------------------------------------------------------------------

FeuchtigkeitsSensor::FeuchtigkeitsSensor(std::string name)
    : Sensor(name, "%", 15.0, 99.0, 0.0, 100.0, 1), feuchte(55.0), lueftungAn(false) {
}

void FeuchtigkeitsSensor::beeinflussen(bool lueftungAn) {
    this->lueftungAn = lueftungAn;
}

double FeuchtigkeitsSensor::simuliereRohwert(int) {
    feuchte += zufall(-0.55, 0.75);          // Duschen, Kochen, Atmen ...
    if (lueftungAn) {
        feuchte -= 1.2;                      // Lueftung trocknet die Luft
    }
    feuchte = std::clamp(feuchte, 34.0, 88.0);
    return feuchte + zufall(-0.3, 0.3);
}
