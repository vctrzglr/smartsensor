#pragma once
#include <string>
#include <vector>
#include "Messwert.h"

// Abstrakte Basisklasse fuer alle Sensoren im Smart Home.
//
// messen() steuert den gemeinsamen Ablauf jeder Messung (Betriebszustand,
// Stoerungen, Offset-Drift, Plausibilitaetspruefung, Historie).
// Die eigentliche "Physik" liefert jede Unterklasse ueber die rein
// virtuelle Methode simuliereRohwert() -> Vererbung + Polymorphie.
class Sensor {
private:
    std::string name;
    std::string einheit;
    bool betriebszustand;
    double offset;              // langsame Drift, wird durch kalibrieren() entfernt
    double bereichVon;          // Plausibilitaetsbereich: alles ausserhalb wird verworfen
    double bereichBis;
    double anzeigeVon;          // Skala fuer die Balkenanzeige im Dashboard
    double anzeigeBis;
    int nachkommastellen;
    std::vector<Messwert> historie;   // nur gueltige Messwerte

    static const std::size_t MAX_HISTORIE = 288;   // ein kompletter Simulationstag

public:
    Sensor(std::string name, std::string einheit,
           double bereichVon, double bereichBis,
           double anzeigeVon, double anzeigeBis, int nachkommastellen);
    virtual ~Sensor();

    // Gemeinsamer Messablauf fuer alle Sensortypen (Schablone)
    Messwert messen(int minuteDesTages);

    bool kalibrieren();                          // setzt den Offset zurueck
    void setBetriebszustand(bool an);

    // Rueckwirkung eines Aktors auf die Umgebung (Standard: keine Wirkung).
    // Wird von den Unterklassen ueberschrieben, z.B. Heizung waermt den Raum.
    virtual void beeinflussen(bool aktorAn);

    const std::string& getName() const;
    const std::string& getEinheit() const;
    bool istInBetrieb() const;
    double getOffset() const;
    double getAnzeigeVon() const;
    double getAnzeigeBis() const;
    int getNachkommastellen() const;
    const std::vector<Messwert>& getHistorie() const;

    bool hatMesswert() const;
    const Messwert& letzterMesswert() const;

    // Statistik ueber die gespeicherte Historie
    double minimum() const;
    double maximum() const;
    double mittelwert() const;

protected:
    // liefert den "wahren" physikalischen Wert zum angegebenen Zeitpunkt
    virtual double simuliereRohwert(int minuteDesTages) = 0;
};

// ---------------------------------------------------------------------------
// Konkrete Sensoren
// ---------------------------------------------------------------------------

// Raumtemperatur: Tagesgang (nachts kuehl, nachmittags warm) + Heizungswaerme
class TemperaturSensor : public Sensor {
private:
    double heizEffekt;   // zusaetzliche Waerme durch die Heizung (0..4 Grad)

public:
    TemperaturSensor(std::string name);
    void beeinflussen(bool heizungAn) override;

protected:
    double simuliereRohwert(int minuteDesTages) override;
};

// Aussenhelligkeit: Sonnenstand zwischen 06:00 und 21:00 Uhr, nachts dunkel
class LichtSensor : public Sensor {
public:
    LichtSensor(std::string name);

protected:
    double simuliereRohwert(int minuteDesTages) override;
};

// Luftfeuchte: zufaelliges Auf und Ab, die Lueftung senkt die Feuchte
class FeuchtigkeitsSensor : public Sensor {
private:
    double feuchte;      // aktueller "wahrer" Zustand des Raums
    bool lueftungAn;

public:
    FeuchtigkeitsSensor(std::string name);
    void beeinflussen(bool lueftungAn) override;

protected:
    double simuliereRohwert(int minuteDesTages) override;
};
