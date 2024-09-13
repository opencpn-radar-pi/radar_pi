

#include <algorithm>
#include <bitset>
#include <cstdint>
#include <list>
#include <string>
#include <time.h>
#include <vector>
#include <wx/textfile.h>
#include <wx/wx.h>

class AisMaker {
public:
  std::string Str2Str(std::string str, const char* charsToRemove);
  float Str2Float(std::string str, const char* exc);
  int Str2Int(std::string str, const char* exc);
  std::string Int2BString(int value, int length);
  int findIntFromLetter(char letter);
  char findCharFromNumber(int mp);
  std::string Str2Six(std::string str, int length);
  int BString2Int(char* bitlist);
  std::string NMEAencapsulate(std::string BigString, int numsixes);
  wxString makeCheckSum(wxString mySentence);
  wxString nmeaEncode(wxString type, int MMSI, wxString status, double spd,
                      double ilat, double ilon, double crse, double hdg,
                      wxString channel, wxString timestamp);

  wxString nmeaEncode1_2_3(int message_id, int iMMSI, int nav_status,
                           float sog,  // Knots.
                           double ilat, double ilon,
                           double cog,  // Degrees.
                           double true_heading, wxString channel);

protected:
};