#include "SmartHome.h"
#include <exception>
#include <iomanip>
#include <sstream>

// ---------------------------------------------------------------------------
// Aktor
// ---------------------------------------------------------------------------

Aktor::Aktor(std::string name)
    : name(name), an(false), schaltvorgaenge(0) {
}

const std::string& Aktor::getName() const { return name; }
bool Aktor::istAn() const { return an; }
int Aktor::getSchaltvorgaenge() const { return schaltvorgaenge; }

void Aktor::schalten(bool neuerZustand) {
    if (an != neuerZustand) {
        an = neuerZustand;
        schaltvorgaenge++;
    }
}

// ---------------------------------------------------------------------------
// Regel
// ---------------------------------------------------------------------------

Regel::Regel(Sensor* sensor, Aktor* aktor, double einSchwelle, double ausSchwelle)
    : sensor(sensor), aktor(aktor), einSchwelle(einSchwelle), ausSchwelle(ausSchwelle) {
}

std::string Regel::anwenden() {
    // Rueckwirkung des Aktors auf die Umgebung (z.B. Heizung waermt den Raum),
    // unabhaengig davon, ob der Sensor gerade misst.
    sensor->beeinflussen(aktor->istAn());

    if (!sensor->istInBetrieb() || !sensor->hatMesswert()) {
        return "";   // ohne aktuelle Daten wird nicht geschaltet
    }

    double wert = sensor->letzterMesswert().getZahlenwert();
    bool anBeiUnterschreitung = (einSchwelle < ausSchwelle);

    bool neuerZustand = aktor->istAn();
    double schwelle = 0.0;
    std::string vergleich;

    if (anBeiUnterschreitung) {
        if (wert < einSchwelle)      { neuerZustand = true;  schwelle = einSchwelle; vergleich = "<"; }
        else if (wert > ausSchwelle) { neuerZustand = false; schwelle = ausSchwelle; vergleich = ">"; }
    } else {
        if (wert > einSchwelle)      { neuerZustand = true;  schwelle = einSchwelle; vergleich = ">"; }
        else if (wert < ausSchwelle) { neuerZustand = false; schwelle = ausSchwelle; vergleich = "<"; }
    }

    if (neuerZustand == aktor->istAn()) {
        return "";   // nichts zu tun
    }

    aktor->schalten(neuerZustand);

    int stellen = sensor->getNachkommastellen();
    return aktor->getName() + (neuerZustand ? " EIN" : " AUS") + " – "
           + sensor->getName() + " " + alsText(wert, stellen) + " "
           + sensor->getEinheit() + " " + vergleich + " " + alsText(schwelle, stellen);
}

// ---------------------------------------------------------------------------
// SmartHome
// ---------------------------------------------------------------------------

SmartHome::SmartHome(std::string name, const std::string& csvPfad)
    : name(name), minuteDesTages(355), tag(1), csvPfad(csvPfad) {
    ereignis("System \"" + name + "\" gestartet");
}

Sensor* SmartHome::sensorHinzufuegen(std::unique_ptr<Sensor> sensor) {
    sensoren.push_back(std::move(sensor));
    return sensoren.back().get();
}

Aktor* SmartHome::aktorHinzufuegen(const std::string& aktorName) {
    aktoren.push_back(std::make_unique<Aktor>(aktorName));
    return aktoren.back().get();
}

void SmartHome::regelHinzufuegen(Sensor* sensor, Aktor* aktor,
                                 double einSchwelle, double ausSchwelle) {
    regeln.push_back(Regel(sensor, aktor, einSchwelle, ausSchwelle));
}

void SmartHome::messdatenLaden() {
    // 1) vorhandenes Protokoll einlesen (falls es eines gibt)
    std::ifstream eingabe(csvPfad);
    int geladen = 0;
    int uebersprungen = 0;
    if (eingabe.is_open()) {
        std::string zeile;
        std::getline(eingabe, zeile);   // Kopfzeile ueberspringen
        while (std::getline(eingabe, zeile)) {
            if (zeile.empty()) {
                continue;
            }
            if (messzeileEinlesen(zeile)) {
                geladen++;
            } else {
                uebersprungen++;
            }
        }
        eingabe.close();
    }

    // 2) neues Protokoll beginnen (alte Datei wird ueberschrieben)
    csvDatei.open(csvPfad);
    if (csvDatei.is_open()) {
        csvDatei << "Tag;Uhrzeit;Sensor;Messwert;Einheit\n";
    }

    if (geladen > 0) {
        std::string text = std::to_string(geladen) + " Messwerte aus " + csvPfad + " übernommen";
        if (uebersprungen > 0) {
            text += " (" + std::to_string(uebersprungen) + " ungültige Zeilen übersprungen)";
        }
        ereignis(text);
    } else {
        ereignis("Keine früheren Messdaten gefunden – neue Aufzeichnung: " + csvPfad);
    }
}

// Eine CSV-Zeile im Format "Tag;HH:MM;Sensorname;Zahlenwert;Einheit" einlesen.
// Liefert false bei jedem Fehler (Format, Zahl, Uhrzeit, unbekannter Sensor).
bool SmartHome::messzeileEinlesen(const std::string& zeile) {
    std::istringstream strom(zeile);
    std::string tagFeld, zeitFeld, nameFeld, wertFeld, einheitFeld;
    if (!std::getline(strom, tagFeld, ';') || !std::getline(strom, zeitFeld, ';')
        || !std::getline(strom, nameFeld, ';') || !std::getline(strom, wertFeld, ';')
        || !std::getline(strom, einheitFeld)) {
        return false;   // Zeile unvollstaendig
    }
    if (zeitFeld.size() != 5 || zeitFeld[2] != ':') {
        return false;   // Uhrzeit nicht im Format HH:MM
    }
    try {
        int minute = std::stoi(zeitFeld.substr(0, 2)) * 60 + std::stoi(zeitFeld.substr(3, 2));
        if (minute < 0 || minute >= 24 * 60) {
            return false;
        }
        double wert = std::stod(wertFeld);
        for (const std::unique_ptr<Sensor>& sensor : sensoren) {
            if (sensor->getName() == nameFeld) {
                return sensor->messwertLaden(Messwert(wert, einheitFeld, minute));
            }
        }
    } catch (const std::exception&) {
        return false;   // Zahl nicht lesbar (stoi/stod)
    }
    return false;   // unbekannter Sensor
}

void SmartHome::simulationsschritt() {
    minuteDesTages += MINUTEN_PRO_SCHRITT;
    if (minuteDesTages >= 24 * 60) {
        minuteDesTages = 0;
        tag++;
    }

    // Alle Sensoren polymorph ueber die Basisklasse auslesen
    for (const std::unique_ptr<Sensor>& sensor : sensoren) {
        if (!sensor->istInBetrieb()) {
            continue;
        }
        Messwert messwert = sensor->messen(minuteDesTages);
        if (messwert.istGueltig()) {
            if (csvDatei.is_open()) {
                csvDatei << tag << ";" << messwert.zeitText() << ";"
                         << sensor->getName() << ";"
                         << alsText(messwert.getZahlenwert(), sensor->getNachkommastellen())
                         << ";" << messwert.getEinheit() << "\n";
            }
        } else {
            ereignis("Störung: " + sensor->getName() + " lieferte unplausiblen Wert ("
                     + alsText(messwert.getZahlenwert(), sensor->getNachkommastellen())
                     + " " + messwert.getEinheit() + ") – verworfen");
        }
    }

    // Das Haus reagiert: Regeln pruefen und Aktoren schalten
    for (Regel& regel : regeln) {
        std::string text = regel.anwenden();
        if (!text.empty()) {
            ereignis(text);
        }
    }

    if (csvDatei.is_open()) {
        csvDatei.flush();
    }
}

void SmartHome::kalibrieren() {
    for (const std::unique_ptr<Sensor>& sensor : sensoren) {
        sensor->kalibrieren();
    }
    ereignis("Alle Sensoren kalibriert – Offsets zurückgesetzt");
}

void SmartHome::sensorUmschalten(std::size_t index) {
    if (index >= sensoren.size()) {
        return;
    }
    Sensor& sensor = *sensoren[index];
    sensor.setBetriebszustand(!sensor.istInBetrieb());
    ereignis(sensor.getName()
             + (sensor.istInBetrieb() ? " wieder in Betrieb" : " außer Betrieb genommen"));
}

void SmartHome::ereignis(const std::string& text) {
    ereignisse.insert(ereignisse.begin(), uhrzeitText() + "  " + text);
    if (ereignisse.size() > MAX_EREIGNISSE) {
        ereignisse.pop_back();
    }
}

const std::string& SmartHome::getName() const { return name; }
const std::vector<std::unique_ptr<Sensor>>& SmartHome::getSensoren() const { return sensoren; }
const std::vector<std::unique_ptr<Aktor>>& SmartHome::getAktoren() const { return aktoren; }
const std::vector<std::string>& SmartHome::getEreignisse() const { return ereignisse; }
int SmartHome::getTag() const { return tag; }

std::string SmartHome::uhrzeitText() const {
    std::ostringstream aus;
    aus << std::setw(2) << std::setfill('0') << minuteDesTages / 60 << ":"
        << std::setw(2) << std::setfill('0') << minuteDesTages % 60;
    return aus.str();
}

bool SmartHome::istTag() const {
    return minuteDesTages >= 360 && minuteDesTages < 1260;
}
