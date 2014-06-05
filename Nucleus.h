/* -------------------------------------------------------------
 *  file:        Nucleus.h and Nucleus.cpp
 *  description: Interface to DDE Communication for Fermilab's
 *               Cascade Microtech Nucleus Probe Station Software
 *  author:      Cristian Gingu
 *  modified:    April 2014
 *  rev:         1
 * -------------------------------------------------------------
 */

#pragma once
#include <iostream>

#ifdef _WIN32
#include <Windows.h>
#include <ddeml.h>
#else
//#include "linux/?????"
#endif

#ifdef UNICODE
#define lpstr  LPCWSTR
#else
#define lpstr  LPCSTR
#endif

class CNucleus
{
public:
	CNucleus(void);
	~CNucleus(void);
	bool GetError();
	void Open();
	void Close();
	void SendCommand(char* cmd);
	void GetResponse(std::string* resp);
	std::string Execute(char*cmd);
	//Load wafer map file and show wafer map window 
	std::string LoadWaferMap();
	//Move the chuck UP, make contact with wafer.
	std::string MoveUp();
	//Move the chuck DOWN, separate from wafer.
	std::string MoveDown();
	//Move to wafer center.
	std::string MoveCenter();
	//Move to the load wafer position.
	std::string MoveLoad();
	//Move to the first die site designated for testing in the active probe plan file
	std::string MoveFirstDie();
	//Move to the first subsite on the current die designated for testing in the active probe plan file
	std::string MoveFirstSubsite();
	//Move to the next die site designated for testing in the active probe plan file
	std::string MoveNextDie();
	//Move to the next site designated for testing in the active probe plan file.
	//The site can be either a die site or a subsite, whichever comes first
	std::string MoveNextSite();
	//Move to a specified die and subsite, using the testable die index and 
	//the numeric index for the subsite
	std::string MoveAbsDieSubsite(int dieIndex, int subsiteIndex);

	static HDDEDATA CALLBACK MyDDECallBack(UINT wType, UINT wFmt, HCONV HConv, 
		HSZ dataHandle1, HSZ dataHandle2, HDDEDATA data, DWORD myword1, DWORD myword2);
	//HDDEDATA (*pMyDDECallBack)(UINT wType, UINT wFmt, HCONV m_hConv, 
	//	HSZ dataHandle1, HSZ dataHandle2, HDDEDATA data, DWORD myword1, DWORD myword2);

private:
	// Variables and initial values.
	BOOL m_err             ;
	DWORD m_idInst         ;
	HSZ m_hszService       ;  // Service handle for "EDMAIN"
	HSZ m_hszTopic         ;  // Topic   handle for "CMI Commands"
	HSZ m_hszCmd           ;  // Command handle for given string command 
	lpstr m_NucleusService ;
	lpstr m_NucleusTopic   ;
	lpstr m_NucleusCmd     ;
	lpstr m_msg_str        ;
	lpstr m_msg_cap        ;
	HCONV m_hConv          ;
	HDDEDATA m_transResult ;
	std::string m_resp     ;
	char m_result[300]     ;
	DWORD m_dwTimeout      ; // need long timeout to allow the station to move and respond.

};

