#include "Messwert.h"
#include <iomanip>
#include <sstream>

Messwert::Messwert() {
    zahlenwert = 0.0;
    einheit = "?";
    minuteDesTages = 0;
    gueltig = false;
}

Messwert::Messwert(double zahlenwert, std::string einheit, int minuteDesTages, bool gueltig)
    : zahlenwert(zahlenwert), einheit(einheit), minuteDesTages(minuteDesTages), gueltig(gueltig) {
}

double Messwert::getZahlenwert() const {
    return zahlenwert;
}

const std::string& Messwert::getEinheit() const {
    return einheit;
}

int Messwert::getMinuteDesTages() const {
    return minuteDesTages;
}

bool Messwert::istGueltig() const {
    return gueltig;
}

std::string Messwert::zeitText() const {
    std::ostringstream aus;
    aus << std::setw(2) << std::setfill('0') << minuteDesTages / 60 << ":"
        << std::setw(2) << std::setfill('0') << minuteDesTages % 60;
    return aus.str();
}

std::string alsText(double wert, int nachkommastellen) {
    std::ostringstream aus;
    aus << std::fixed << std::setprecision(nachkommastellen) << wert;
    return aus.str();
}
