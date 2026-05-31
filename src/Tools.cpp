#include <Tools.h>

String splitString(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i + 1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

std::vector<String> split(const String& str, char delimiter)
{
    std::vector<String> result;
    int start = 0;
    int end = str.indexOf(delimiter);
    while (end > -1) {
        result.push_back(str.substring(start, end));
        start = end + 1;
        end = str.indexOf(delimiter, start);
    }
    result.push_back(str.substring(start));
    return result;
}

bool isInteger(const String& str)
{
    for (unsigned int i = 0; i < str.length(); i++) {
        if (!isdigit(str[i])) {
            return false;
        }
    }
    return true;
}

std::vector<String> splitParameters(const String& paramStr)
{
    std::vector<String> params;
    String tempParam;
    bool inQuotes = false;

    for (unsigned int i = 0; i < paramStr.length(); ++i) {
        char c = paramStr[i];

        if (c == '"') {
            // Toggle the inQuotes flag
            inQuotes = !inQuotes;
            if (!inQuotes && !tempParam.isEmpty()) {
                params.push_back(tempParam);
                tempParam = "";
            }
        } else if (c == ' ' && !inQuotes) {
            // Space outside quotes indicates the end of a parameter
            if (!tempParam.isEmpty()) {
                params.push_back(tempParam);
                tempParam = "";
            }
        } else {
            // Accumulate characters for the parameter
            tempParam += c;
        }
    }

    // Push the last parameter if there's any left
    if (!tempParam.isEmpty()) {
        params.push_back(tempParam);
    }

    return params;
}

String calculateTimeStop(String timeStart, int duration) {
    int hh = timeStart.substring(0, 2).toInt();
    int mm = timeStart.substring(3, 5).toInt();
    int ss = timeStart.substring(6, 8).toInt();

    int totalSeconds = hh * 3600 + mm * 60 + ss + duration;

    int stop_hh = (totalSeconds / 3600) % 24; // modulo 24 pour rester dans la journée
    int stop_mm = (totalSeconds % 3600) / 60;
    int stop_ss = totalSeconds % 60;

    char buffer[9];
    sprintf(buffer, "%02d:%02d:%02d", stop_hh, stop_mm, stop_ss);

    return String(buffer);
}

bool isDateInRange(const std::tm &current, const String &start, const String &end)
{
    // Si aucune date définie, tout est valide
    if (start.isEmpty() && end.isEmpty()) return true;

    int sDay, sMonth, eDay, eMonth;
    if (std::sscanf(start.c_str(), "%2d/%2d", &sDay, &sMonth) != 2) return false;
    if (std::sscanf(end.c_str(),   "%2d/%2d", &eDay, &eMonth)   != 2) return false;

    int curDay   = current.tm_mday;
    int curMonth = current.tm_mon + 1;

    // Convertir en "jour de l'année" pour simplifier
    auto dayOfYear = [](int d, int m) {
        static const int daysBeforeMonth[12] =
            {0,31,59,90,120,151,181,212,243,273,304,334};
        return daysBeforeMonth[m-1] + d;
    };

    int cur  = dayOfYear(curDay,  curMonth);
    int startVal = dayOfYear(sDay, sMonth);
    int endVal   = dayOfYear(eDay, eMonth);

    if (startVal <= endVal) {
        // Plage dans la même année
        return cur >= startVal && cur <= endVal;
    } else {
        // Plage qui chevauche la fin d'année
        return (cur >= startVal) || (cur <= endVal);
    }
}