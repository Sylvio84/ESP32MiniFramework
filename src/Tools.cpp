#include "../include/Tools.h"

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