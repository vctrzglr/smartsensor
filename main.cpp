// ============================================================================
//  SMART SENSOR – Simulation smarter Sensoren im Eigenheim
//
//  Einstiegspunkt des Programms: hier wird das Smart Home aus Sensoren,
//  Aktoren und Regeln zusammengebaut und anschliessend die Simulations-
//  schleife (Messen -> Reagieren -> Anzeigen) ausgefuehrt.
// ============================================================================

#include <atomic>
#include <chrono>
#include <csignal>
#include <memory>
#include <thread>

#include "Anzeige.h"
#include "Sensor.h"
#include "SmartHome.h"

// Sorgt dafuer, dass auch Strg+C die Schleife sauber beendet und der
// TerminalGuard das Terminal wiederherstellen kann (siehe Anzeige.h).
static std::atomic<bool> laufen(true);

static void signalBehandeln(int) {
    laufen = false;
}

int main() {
    std::signal(SIGINT, signalBehandeln);

    // 1) Smart Home zusammenbauen ------------------------------------------
    //    Das SmartHome besitzt Sensoren und Aktoren (Komposition), die
    //    Regeln verknuepfen sie nur ueber Zeiger (Assoziation).
    SmartHome haus("Mein Zuhause", "messdaten.csv");

    Sensor* temperatur = haus.sensorHinzufuegen(std::make_unique<TemperaturSensor>("Temperatur Wohnzimmer"));
    Sensor* helligkeit = haus.sensorHinzufuegen(std::make_unique<LichtSensor>("Helligkeit Garten"));
    Sensor* feuchte    = haus.sensorHinzufuegen(std::make_unique<FeuchtigkeitsSensor>("Luftfeuchte Bad"));

    Aktor* heizung  = haus.aktorHinzufuegen("Heizung");
    Aktor* licht    = haus.aktorHinzufuegen("Gartenlicht");
    Aktor* lueftung = haus.aktorHinzufuegen("Lüftung");

    // Regeln mit Hysterese: einSchwelle < ausSchwelle -> EIN bei Unterschreitung
    haus.regelHinzufuegen(temperatur, heizung, 19.5, 22.5);   // heizen, wenn kalt
    haus.regelHinzufuegen(helligkeit, licht, 150.0, 260.0);   // Licht an, wenn dunkel
    haus.regelHinzufuegen(feuchte, lueftung, 65.0, 55.0);     // lueften, wenn feucht

    // 2) Simulationsschleife -------------------------------------------------
    TerminalGuard terminal;   // RAII: stellt das Terminal am Ende wieder her
    Anzeige anzeige;

    int intervallMs = 700;    // Echtzeit pro Simulationsschritt (5 Sim-Minuten)
    bool pause = false;
    bool neuZeichnen = true;  // nur zeichnen, wenn sich etwas geaendert hat
    int leerlauf = 0;
    auto letzterSchritt = std::chrono::steady_clock::now();

    haus.simulationsschritt();   // erste Messung sofort ausfuehren

    while (laufen) {
        // Tastatureingaben verarbeiten
        int taste = Anzeige::tasteLesen();
        if (taste != -1) {
            neuZeichnen = true;
        }
        if (taste == 'q' || taste == 'Q') {
            laufen = false;
        } else if (taste == ' ') {
            pause = !pause;
        } else if (taste == '+' || taste == '=') {
            intervallMs = std::max(200, intervallMs - 150);   // schneller
        } else if (taste == '-') {
            intervallMs = std::min(2000, intervallMs + 150);  // langsamer
        } else if (taste == 'k' || taste == 'K') {
            haus.kalibrieren();
        } else if (taste >= '1' && taste <= '9') {
            haus.sensorUmschalten(static_cast<std::size_t>(taste - '1'));
        }

        // naechsten Simulationsschritt ausfuehren, wenn das Intervall um ist
        auto jetzt = std::chrono::steady_clock::now();
        if (!pause && jetzt - letzterSchritt >= std::chrono::milliseconds(intervallMs)) {
            haus.simulationsschritt();
            letzterSchritt = jetzt;
            neuZeichnen = true;
        }

        // einmal pro Sekunde auch ohne Aenderung zeichnen (z.B. nach Resize)
        if (++leerlauf >= 20) {
            neuZeichnen = true;
        }
        if (neuZeichnen) {
            anzeige.zeichnen(haus, intervallMs, pause);
            neuZeichnen = false;
            leerlauf = 0;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    return 0;
}
