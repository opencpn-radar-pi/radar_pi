

#include <algorithm>
#include <bitset>
#include <cstdint>
#include <list>
#include <string>
#include <time.h>
#include <vector>
#include <wx/textfile.h>
#include <wx/wx.h>

using namespace std;

class AisMaker {

public:
    string Str2Str(string str, const char* charsToRemove);
    float Str2Float(string str, const char* exc);
    int Str2Int(string str, const char* exc);
    string Int2BString(int value, int length);
    int findIntFromLetter(char letter);
    char findCharFromNumber(int mp);
    string Str2Six(string str, int length);
    int BString2Int(char* bitlist);
    string NMEAencapsulate(string BigString, int numsixes);
    wxString makeCheckSum(wxString mySentence);
    wxString nmeaEncode(wxString type, int MMSI, wxString status, double spd,
        double ilat, double ilon, double crse, double hdg, wxString channel,
        wxString timestamp);

protected:
};
