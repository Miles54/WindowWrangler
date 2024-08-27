// Copyright Ethan M. Hardt 2024
// Licensed under the MIT License
// See License.txt for details


#include <Windows.h>
#include <psapi.h>
#include <iostream>
#include <string>
#include <vector>

void Print(std::string Str) {
	std::cout << Str << std::endl << std::flush;
}

// Options:
bool ListHidden		= false;
bool ListUnopened	= false;
bool DisplayPID		= false;
bool CaseSensitive	= true;
bool SendWMClose	= false;

// Filtering Parameters:
std::vector<std::string> LookFor;
std::vector<DWORD> MatchPID;
std::vector<std::string> nLookFor;
std::vector<DWORD> nMatchPID;

char Lower(char Ch) {
	char ChRet = Ch;
	if (ChRet >= 0x41 && ChRet <= 0x5A) {
		ChRet += 0x20;
	}
	return ChRet;
}

bool Search(std::string Search, std::string Term) {

	// Exact Match. (probably faster, but I'm not testing it)
	if (Search.size() == Term.size()) {
		if (Search == Term) {
			return true;
		}
	}

	// Partial Match (also Case-insensitive Exact, if enabled)
	if (Search.size() < Term.size()) {
		for (uint64_t A = 0; A < (Term.size() - Search.size()) + 1; A++) {
			bool Success = true;
			for (uint64_t B = 0; B < Search.size(); B++) {
				if (CaseSensitive) {
					if (Search.at(B) != Term.at(B + A)) {
						Success = false;
						break;
					}
				} else {
					if (Lower(Search.at(B)) != Lower(Term.at(B + A))) {
						Success = false;
						break;
					}
				}
			}
			if (Success) {
				return true;
			}
		}
	}

	// No Match.
	return false;
}




BOOL CALLBACK EnumWindowsProc(HWND Window, LPARAM lp) {
	if (!IsWindowVisible(Window) && !ListHidden) {
		return TRUE; // specify -h to list hidden windows.
	}
	char Ch[256];
	if (GetWindowTextA(Window, Ch, 256) > 0) {
		std::string WindowTitle = Ch;
		bool Skip = true;
		if (LookFor.size() > 0) {
			for (uint64_t G = 0; G < LookFor.size(); G++) {
				if (Search(LookFor.at(G),WindowTitle)) {
					Skip = false;
					break;
				}
			}
		} else {
			Skip = false;
		}
		if (nLookFor.size() > 0) {
			for (uint64_t G = 0; G < nLookFor.size(); G++) {
				if (Search(nLookFor.at(G),WindowTitle)) {
					Skip = true;
					break;
				}
			}
		}
		if (Skip) {
			return TRUE;
		}
		bool PIDValid = (MatchPID.size() == 0);
		DWORD ProcID;
		if (GetWindowThreadProcessId(Window, &ProcID) != 0) {
			if (MatchPID.size() > 0) {
				for (uint64_t Ind = 0; Ind < MatchPID.size(); Ind++) {
					if (MatchPID.at(Ind) == ProcID) {
						PIDValid = true;
					}
				}
			}
			if (!PIDValid) {
				return TRUE;
			}
			if (nMatchPID.size() > 0) {
				for (uint64_t Ind = 0; Ind < nMatchPID.size(); Ind++) {
					if (nMatchPID.at(Ind) == ProcID) {
						PIDValid = false;
					}
				}
			}
			if (!PIDValid) {
				return TRUE;
			}

			char Ch2[4096];
			std::string AdditionalInfo;
			if (DisplayPID) {
				AdditionalInfo += " with PID of " + std::to_string(ProcID);
			}

			HANDLE H = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, ProcID);
			if (H != nullptr) {
				DWORD ExSize = 4096;
				if (QueryFullProcessImageNameA(H, 0, Ch2, &ExSize) != 0) {
					std::string ProcessName = Ch2;
					Print(WindowTitle + " Belongs to " + ProcessName + AdditionalInfo);
					CloseHandle(H);
				} else {
					Print(WindowTitle + " Belongs to a process we query the name of" + AdditionalInfo);
				}
			} else {
				if (ListUnopened) {
					Print(WindowTitle + " Belongs to a process we couldn't open" + AdditionalInfo);
				}
			}
		} else {
			if (ListUnopened) {
				Print(WindowTitle + " Belongs to an unknown process");
			}
		}
		if (SendWMClose) {
			if (PIDValid) {
				SendMessage(Window, WM_CLOSE, 0, 0);
			}
		}

	}
	return TRUE;
}


int main(int argc, char* argv[]) {
	for (int argi = 0; argi < argc; argi++) {
		std::string arg = argv[argi];
		if (arg == "-?" || arg == "/?") {
			Print("Window Wrangler Help:");
			Print("-a or /a			Shows the about screen");
			Print("-? or /?			Shows this Help Screen");
			Print("-h or /h			Lists windows that are hidden");
			Print("-v or /v			Lists windows of unknown processes (Verbose)");
			Print("-p or /p			Prints the PID");
			Print("-i or /i			Search is insensitive to case");
			Print("-c or /c			Send WM_CLOSE to the listed programs (filter with -f, -nf, and/or -m)");
			Print("-f or /f	[title]		Find a window by title");
			Print("-nf or /nf	[title]		Filter window out by title");
			Print("-m or /m	[PID]		Match a window by program ID");
			Print("-nm or /nm	[PID]		Unmatch a window by program ID");
			return 0;
		}
		if (arg == "-a" || arg == "/a" || arg == "-A" || arg == "/A") {
			Print("Window Wrangler");
			Print("by Miles (Ethan M. Hardt)");
			Print("version 1.0");
			Print("");
			Print("Licensed under MIT");
			Print("See license.txt for the license");
			Print("(Please do not change the");
			Print("license for downstream distributions)");
			Print("");
			Print("Personal Note:");
			Print("Please do not call me by my legal name,");
			Print("unless you are legally required to,");
			Print("or I have met you in person.");
			Print("It is included for legal reasons.");
			return 0;
		}
		if (arg == "-h" || arg == "/h" || arg == "-H" || arg == "/H") {
			ListHidden = true;
		}
		if (arg == "-v" || arg == "/v" || arg == "-V" || arg == "/V") {
			ListUnopened = true;
		}
		if (arg == "-p" || arg == "/p" || arg == "-P" || arg == "/P") {
			DisplayPID = true;
		}
		if (arg == "-i" || arg == "/i" || arg == "-I" || arg == "/I") {
			CaseSensitive = false;
		}
		if (arg == "-c" || arg == "/c" || arg == "-C" || arg == "/C") {
			SendWMClose = true;
		}
		if (arg == "-f" || arg == "/f" || arg == "-F" || arg == "/F") {
			if (argi + 1 < argc) {
				LookFor.push_back(std::string(argv[argi + 1]));
				argi++;
			}
		}
		if (arg == "-m" || arg == "/m" || arg == "-M" || arg == "/M") {
			if (argi + 1 < argc) {
				try {
					MatchPID.push_back(std::stoi(std::string(argv[argi + 1])));
					argi++;
				} catch (...) {
					Print("Could not convert "+std::string(argv[argi + 1])+" to a PID");
					return -1;
				}
			}
		}
		if (arg == "-nf" || arg == "/nf" || arg == "-NF" || arg == "/NF") {
			if (argi + 1 < argc) {
				nLookFor.push_back(std::string(argv[argi + 1]));
				argi++;
			}
		}
		if (arg == "-nm" || arg == "/nm" || arg == "-NM" || arg == "/NM") {
			if (argi + 1 < argc) {
				try {
					nMatchPID.push_back(std::stoi(std::string(argv[argi + 1])));
					argi++;
				} catch (...) {
					Print("Could not convert "+std::string(argv[argi + 1])+" to a PID");
					return -1;
				}
			}
		}
	}
	if (nMatchPID.size() > 0 && MatchPID.size() > 0) {
		Print("please do not use -nm and -m together");
		return -1;
	}

	EnumWindows(EnumWindowsProc, 0);
	return 0;
}
