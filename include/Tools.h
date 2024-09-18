#include <Arduino.h>
#include <vector>

String splitString(String data, char separator, int index);
std::vector<String> split(const String& str, char delimiter);
std::vector<String> splitParameters(const String& paramStr);
bool isInteger(const String& str);
