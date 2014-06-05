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
//#include "stdafx.h"
#include "Nucleus.h"
#include <sstream>

HDDEDATA CNucleus::MyDDECallBack(UINT wType, UINT wFmt, HCONV m_hConv, 
	HSZ dataHandle1, HSZ dataHandle2, HDDEDATA data, DWORD myword1, DWORD myword2)
{
	return NULL;
};


CNucleus::CNucleus(void)
{
	// Variables and initial values.
	m_err      = FALSE;
	m_idInst     = 0;
	m_hszService = NULL;  // Service handle for "EDMAIN"
	m_hszTopic   = NULL;  // Topic   handle for "CMI Commands"
	m_hszCmd     = NULL;  // Command handle for given string command 
	m_NucleusService = "EDMAIN";
	m_NucleusTopic   = "CMI Commands";
	m_NucleusCmd     = ":mov:down 2";
	m_msg_str        = "";
	m_msg_cap        = "";
	m_hConv          = NULL;
	m_transResult    = NULL;
	m_resp           = "";
	//m_result[300];
	m_dwTimeout      = 60000; 
	// need long timeout to allow the station to move and respond.

	// Initialize the DDEML environment. Create an instance that is used in many other calls to DDE.
	if (DMLERR_NO_ERROR != DdeInitialize(&m_idInst, CNucleus::MyDDECallBack, APPCMD_CLIENTONLY, 0))
	{
		m_err = TRUE;
		m_msg_str = "\nWINAPI DdeInitialize() ERROR";
		m_msg_cap = "CNucleus::CNucleus() constructor";
		MessageBox(NULL, m_msg_str, m_msg_cap, MB_ICONSTOP);
	}

	// Create string handles for the server name, topic and item.
	if (!m_err)
	{
		m_hszService = DdeCreateStringHandle(m_idInst, m_NucleusService, CP_WINANSI);
		m_hszTopic   = DdeCreateStringHandle(m_idInst, m_NucleusTopic, CP_WINANSI);
		m_err = (m_hszService==NULL || m_hszTopic==NULL);
		if (m_err) 
		{
			m_msg_str = "\nWINAPI DdeCreateStringHandle() ERROR";
			m_msg_cap = "CNucleus::CNucleus() constructor";
			MessageBox(NULL, m_msg_str, m_msg_cap, MB_ICONSTOP);
		}
	}

}

bool CNucleus::GetError(){ return m_err;}

void CNucleus::Open()
{
	// Connect to Nucleus DDE Server. Open a conversation.
	if (!m_err)
	{
		m_hConv = DdeConnect(m_idInst, m_hszService, m_hszTopic, NULL);
		m_err = (m_hConv==NULL);
		if (m_err) 
		{
			m_msg_str = "\nWINAPI DdeConnect() ERROR";
			m_msg_cap = "CNucleus::Open()";
			MessageBox(NULL, m_msg_str, m_msg_cap, MB_ICONSTOP);
		}
	}
}

void CNucleus::SendCommand(char* buffer)
{
	if (!buffer[0]) 
		m_err = TRUE;
	m_NucleusCmd = (lpstr)buffer;
	// Create string handles for Nucleus server item.
	if (!m_err)
	{
		m_hszCmd = DdeCreateStringHandle(m_idInst, m_NucleusCmd, CP_WINANSI);
		m_err = (m_hszCmd==NULL);
	}
	// Send the command string to Nucleus server.
	if (!m_err)
	{
		m_transResult = DdeClientTransaction(NULL, 0, m_hConv, m_hszCmd, CF_TEXT, XTYP_REQUEST, m_dwTimeout, NULL);
		m_err = (m_transResult==NULL);
	}
	//if (m_err) 
	//{
	//	m_msg_str = "\nWINAPI DdeClientTransaction() ERROR";
	//	m_msg_cap = "CNucleus::SendCommand()";
	//	MessageBox(NULL, m_msg_str, m_msg_cap, MB_ICONSTOP);
	//}
}

void CNucleus::GetResponse(std::string* resp)
{
	if (!m_err)
	{
		DdeGetData(m_transResult,(LPBYTE)m_result, sizeof(m_result), 0);
		m_msg_str = m_result;
		m_msg_cap = "RESULT";
		MessageBox(NULL, m_msg_str, m_msg_cap, MB_ICONINFORMATION);
		*resp = m_result;
		m_resp = m_result;
	}
	//if (m_err) 
	//{
	//	m_msg_str = "\nWINAPI DdeGetData() ERROR";
	//	m_msg_cap = "CNucleus::GetResponse()";
	//	MessageBox(NULL, m_msg_str, m_msg_cap, MB_ICONSTOP);
	//}
}

void CNucleus::Close()
{
	if (m_hConv != NULL)
		DdeDisconnect(m_hConv);
	if (m_idInst != NULL)
		// Delete string handles.
		if (m_hszService != NULL) 
			DdeFreeStringHandle(m_idInst, m_hszService);
		if (m_hszTopic != NULL) 
			DdeFreeStringHandle(m_idInst, m_hszTopic);
		if (m_hszCmd != NULL) 
			DdeFreeStringHandle(m_idInst, m_hszCmd);
		// Clean out the DDEML environment.
		DdeUninitialize(m_idInst);
}

CNucleus::~CNucleus(void)
{	
	Close();
}

std::string CNucleus::Execute(char* cmd)
{
	// 1. Simplified SendCommand(cmd)
	// 1.1. Create string handles for Nucleus server item.
	//if (!m_err)
	//{
		m_hszCmd = DdeCreateStringHandle(m_idInst, (lpstr)cmd, CP_WINANSI);
		m_err = (m_hszCmd==NULL);
	//}
	// 1.2. Send the command string to Nucleus server.
	if (!m_err)
	{
		m_transResult = DdeClientTransaction(NULL, 0, m_hConv, m_hszCmd, CF_TEXT, XTYP_REQUEST, m_dwTimeout, NULL);
		m_err = (m_transResult==NULL);
	}
	// 2. Simplified GetResponse(resp)
	if (!m_err)
		DdeGetData(m_transResult,(LPBYTE)m_result, sizeof(m_result), 0);
	return m_result;
}

std::string CNucleus::LoadWaferMap()
{
	char* cmd = ":prob:load C:\\Cristian\\CodeLib\\VC++CodeLib\\my_dde\\wafermaps\\PSI46dig_Wtest.wfd";
	m_resp = Execute(cmd);
	cmd = ":prob:show on on on"; // display wafer map, window on top, display die coordinates
	m_resp = Execute(cmd);
	return m_resp;
}

std::string CNucleus::MoveUp()
{
	m_resp = Execute(":mov:up 2");
	if (m_resp.compare("COMPLETE"))
		m_err=TRUE;
	return m_resp;
}

std::string CNucleus::MoveDown()
{
	m_resp = Execute(":mov:down 2");
	if (m_resp.compare("COMPLETE"))
		m_err=TRUE;
	return m_resp;
}

std::string CNucleus::MoveCenter()
{
	m_resp = Execute(":mov:cent 2");
	if (m_resp.compare("COMPLETE"))
		m_err=TRUE;
	return m_resp;
}

std::string CNucleus::MoveLoad()
{
	m_resp = Execute(":mov:load");
	if (m_resp.compare("COMPLETE"))
		m_err=TRUE;
	return m_resp;
}

std::string CNucleus::MoveFirstDie()
{
	m_resp = Execute(":mov:prob:firs:die");
	if (m_resp.compare("COMPLETE"))
		m_err=TRUE;
	return m_resp;
}

std::string CNucleus::MoveFirstSubsite()
{
	m_resp = Execute(":mov:prob:firs:subs");
	if (m_resp.compare("COMPLETE"))
		m_err=TRUE;
	return m_resp;
}

std::string CNucleus::MoveNextDie()
{
	m_resp = Execute(":mov:prob:next:die");
	if (m_resp.compare("COMPLETE"))
		m_err=TRUE;
	return m_resp;
}

std::string CNucleus::MoveNextSite()
{
	m_resp = Execute(":mov:prob:next:site");
	if (m_resp.compare("COMPLETE"))
		m_err=TRUE;
	return m_resp;
}

std::string CNucleus::MoveAbsDieSubsite(int dieIndex, int subsiteIndex)
{
	//One of many solutions
	//char s1[50] = ":mov:prob:abs:ind:subs ";
	//char s2[10] = "";
	//sprintf_s(s2, "%d ", dieIndex);
	//strcat_s(s1, s2);
	//sprintf_s(s2, "%d ", subsiteIndex);
	//strcat_s(s1, s2);

	//Another solution
    std::stringstream s; // must #include <sstream>
    s << ":mov:prob:abs:ind:subs " << dieIndex << " " << subsiteIndex;
    //std::string s1 = s.str();
	m_resp = Execute(&(s.str()[0]));
	if (m_resp.compare("COMPLETE"))
		m_err=TRUE;
	return m_resp;
}