#pragma once
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include "Sensor.h"

// Ein schaltbares Geraet im Smart Home (Heizung, Lampe, Lueftung, ...).
class Aktor {
private:
    std::string name;
    bool an;
    int schaltvorgaenge;

public:
    Aktor(std::string name);

    const std::string& getName() const;
    bool istAn() const;
    int getSchaltvorgaenge() const;
    void schalten(bool neuerZustand);
};

// Verknuepft einen Sensor mit einem Aktor ueber zwei Schwellwerte (Hysterese).
//   einSchwelle < ausSchwelle: EIN bei Unterschreitung  (Heizung, Lampe)
//   einSchwelle > ausSchwelle: EIN bei Ueberschreitung  (Lueftung)
// Die Regel haelt nur non-owning Zeiger (Assoziation) - Besitzer der
// Objekte ist das SmartHome.
class Regel {
private:
    Sensor* sensor;   // non-owning
    Aktor* aktor;     // non-owning
    double einSchwelle;
    double ausSchwelle;

public:
    Regel(Sensor* sensor, Aktor* aktor, double einSchwelle, double ausSchwelle);

    // Prueft den letzten Messwert, schaltet ggf. den Aktor und gibt die
    // Rueckwirkung an den Sensor weiter. Liefert einen Ereignistext,
    // wenn geschaltet wurde, sonst einen leeren String.
    std::string anwenden();
};

// Zentrale Steuerung des Hauses: besitzt Sensoren und Aktoren (Komposition),
// fuehrt die Simulationsschritte aus, wendet die Regeln an, protokolliert
// Ereignisse und schreibt alle Messwerte in eine CSV-Datei (Datenspeicher).
class SmartHome {
private:
    std::string name;
    std::vector<std::unique_ptr<Sensor>> sensoren;
    std::vector<std::unique_ptr<Aktor>> aktoren;
    std::vector<Regel> regeln;
    std::vector<std::string> ereignisse;   // neuestes Ereignis vorn
    int minuteDesTages;
    int tag;
    std::string csvPfad;
    std::ofstream csvDatei;

    static const std::size_t MAX_EREIGNISSE = 60;
    static const int MINUTEN_PRO_SCHRITT = 5;

    void ereignis(const std::string& text);
    bool messzeileEinlesen(const std::string& zeile);

public:
    SmartHome(std::string name, const std::string& csvPfad);

    // Aufbau des Systems: das SmartHome uebernimmt den Besitz,
    // zurueck kommt ein Zeiger zum Verknuepfen ueber Regeln.
    Sensor* sensorHinzufuegen(std::unique_ptr<Sensor> sensor);
    Aktor* aktorHinzufuegen(const std::string& aktorName);
    void regelHinzufuegen(Sensor* sensor, Aktor* aktor,
                          double einSchwelle, double ausSchwelle);

    // Liest die Messdaten des letzten Laufs aus der CSV-Datei in die
    // Sensor-Historien ein (fehlende Datei und ungueltige Zeilen werden
    // abgefangen) und beginnt danach ein neues Protokoll.
    // Nach dem Registrieren aller Sensoren aufrufen.
    void messdatenLaden();

    // Ein Simulationsschritt: Uhr weiterstellen, alle Sensoren messen,
    // Regeln anwenden, Messwerte in die CSV-Datei schreiben.
    void simulationsschritt();

    void kalibrieren();                          // alle Sensoren
    void sensorUmschalten(std::size_t index);    // Betriebszustand an/aus

    const std::string& getName() const;
    const std::vector<std::unique_ptr<Sensor>>& getSensoren() const;
    const std::vector<std::unique_ptr<Aktor>>& getAktoren() const;
    const std::vector<std::string>& getEreignisse() const;
    int getTag() const;
    std::string uhrzeitText() const;
    bool istTag() const;   // zwischen 06:00 und 21:00 Uhr
};
