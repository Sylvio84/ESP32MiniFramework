#include <Arduino.h>
#include <vector>
#include <string>
#include <ctime>

String splitString(String data, char separator, int index);
std::vector<String> split(const String& str, char delimiter);
std::vector<String> splitParameters(const String& paramStr);
bool isInteger(const String& str);
String calculateTimeStop(String timeStart, int duration);
bool isDateInRange(const std::tm &current, const String &start, const String &end);
