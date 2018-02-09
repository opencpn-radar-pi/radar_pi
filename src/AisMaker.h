

#include <wx/wx.h>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <string>
#include <list>
#include <bitset>
#include <wx/textfile.h>
#include <time.h>


using namespace std;

class AisMaker{

public:

	string Str2Str(string str, char* charsToRemove);
	float Str2Float(string str, char* exc);
	int Str2Int(string str, char* exc);
	string Int2BString(int value, int length);
	int findIntFromLetter(char letter);
	char findCharFromNumber(int mp);
	string Str2Six(string str, int length);
	int BString2Int(char * bitlist);
	string NMEAencapsulate(string BigString, int numsixes);
	wxString makeCheckSum(wxString mySentence);
	wxString nmeaEncode(wxString type, int MMSI, wxString status, double spd, double ilat, double ilon, double crse, double hdg, wxString channel, wxString timestamp);

protected:





};