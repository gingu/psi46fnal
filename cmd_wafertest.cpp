/* -------------------------------------------------------------
 *
 *  file:        command.cpp
 *
 *  description: command line interpreter for Chip/Wafer tester
 *
 *  author:      Beat Meier
 *  modified:    31.8.2007
 *
 *  rev:
 *
 * -------------------------------------------------------------
 */


#include "cmd.h"


// =======================================================================
//  chip/wafer test commands
// =======================================================================


int chipPos = 0;

char chipPosChar[] = "ABCD";

CMD_PROC(roctype)
{
	char s[256];
	PAR_STRING(s,250);

	if (strcmp(s, "ana") == 0) settings.rocType = 0;
	else if (strcmp(s, "dig") == 0) settings.rocType = 1;
	else printf("choose ana or dig\n");

	return true;
}


void GetTimeStamp(char datetime[])
{
	time_t t;
	struct tm *dt;
	time(&t);
	dt = localtime(&t);
	strcpy(datetime, asctime(dt));
}


bool ReportWafer()
{
	Log.section("WAFER", false);

	if (settings.proberPort>=0)      // CG
	{
		char *msg;

		// ProductID
		msg = prober.printf("GetProductID");
		if (strlen(msg)<=3)
		{
			printf("missing wafer product id!\n");
			Log.printf("productId?\n");
			return false;
		}
		Log.printf("%s", msg+3);
		strcpy(g_chipdata.productId, msg+3);

		// WaferID
		msg = prober.printf("GetWaferID");
		if (strlen(msg)<=3)
		{
			printf(" missing wafer id!\n");
			Log.printf(" waferId?\n");
			return false;
		}
		Log.printf(" %s", msg+3);
		strcpy(g_chipdata.waferId, msg+3);

		// Wafer Number
		int num;
		msg = prober.printf("GetWaferNum");
		if (strlen(msg)>3) if (sscanf(msg+3, "%i", &num) == 1)
		{
			Log.printf(" %i\n", num);
			strcpy(g_chipdata.waferNr, msg+3);
			return true;
		}

		printf(" missing wafer number!\n");
		Log.printf(" wafernum?\n");
		return false;
	}

	if (settings.proberPort==-2)      // CG
	{
        std::string resp = prober_nucleus.Execute(":prob:load?"); //C:\Cristian\CodeLib...\PSI46digV2.1_A7GMZVX_1.wfd 
        if (resp[0]=='@')
		{
			printf("Prober Nucleus: %s\n", resp.c_str());
            return false;
		}
		else
		{
		    unsigned found = resp.find_last_of("/\\");
		    std::string path = resp.substr(0,found);
		    std::string file = resp.substr(found+1);
		    int indx1 = file.find("_", 0);
		    int indx2 = file.find("_", indx1+1);
		    int indx3 = file.find(".", indx2+1);
		    if (indx1==-1 || indx2==-1 || indx3==-1)
		    {
			    printf(" error reading productId, waferId, waferNr from wafer map file %s\n", resp.c_str());
			    Log.printf(" error reading productId, waferId, waferNr from wafer map file %s\n", resp.c_str());
			    return false;
		    }
		    std::string productId = file.substr(0, indx1);
		    std::string waferId = file.substr(indx1+1, indx2-indx1-1);
		    std::string waferNr = file.substr(indx2+1, indx3-indx2-1);
		    printf("Prober Nucleus: productId: %s\n", productId.c_str());
		    printf("Prober Nucleus: waferId  : %s\n", waferId.c_str());
		    printf("Prober Nucleus: waferNr  : %s\n", waferNr.c_str());
		    Log.printf(" %s", productId.c_str());
		    Log.printf(" %s", waferId.c_str());
		    Log.printf(" %s\n", waferNr.c_str());
		    strcpy(g_chipdata.productId, productId.c_str());
		    strcpy(g_chipdata.waferId, waferId.c_str());
		    strcpy(g_chipdata.waferNr, waferNr.c_str());
		}
        return true;
	}
    return false;
}


bool ReportChip(int &x, int &y)
{
	float posx, posy, posz;

	if (settings.proberPort>=0)      // CG
	{
		char *pos = prober.printf("ReadMapPosition");
		int len = strlen(pos);
		if (len<3) return false;
		pos += 3;
		if (sscanf(pos, "%i %i %f %f", &x, &y, &posx, &posy) != 4)
		{
			printf(" error reading chip information\n");
			return false;
		}
	}
	if (settings.proberPort==-2)      // CG
	{
		std::string pos_microns = prober_nucleus.Execute(":mov:abs? 2");    //int x,y,z
		std::string pos_int = prober_nucleus.Execute(":mov:prob:abs:die?"); //int x,y
		//dieIndex = std::stoi(prober_nucleus.Execute(":mov:prob:abs:ind?"));
		//chipPos = std::stoi(prober_nucleus.Execute(":mov:prob:abs:subs?"));     //int 0,1,2,3
		int indx1 = pos_microns.find(" ", 0);
		int indx2 = pos_microns.find(" ", indx1+1);
		int indx3 = pos_int.find(" ", 0);
		int indx4 = pos_int.find(" ", indx3+1);
		if (indx1==-1 || indx2==-1 || indx3==-1 || indx4!=-1)
		{
			printf(" error reading chip information: %s %s\n", pos_microns.c_str(), pos_int.c_str());
			return false;
		}
		posx = std::stof(pos_microns.substr(0, indx1));
		posy = std::stof(pos_microns.substr(indx1+1, indx2-1));
		posz = std::stof(pos_microns.substr(indx2+1));
		x = std::stoi(pos_int.substr(0, indx3));
		y = std::stoi(pos_int.substr(indx3+1));
	}

	nEntry++;
	printf("#%05i: %i%i%c -> ", nEntry, y, x, chipPosChar[chipPos]);
	fflush(stdout);
	Log.section("CHIP", false);
	Log.printf(" %i %i %c %9.1f %9.1f\n",
		x, y, chipPosChar[chipPos], posx, posy);
	g_chipdata.mapX   = x;
	g_chipdata.mapY   = y;
	g_chipdata.mapPos = chipPos;
	return true;
}


CMD_PROC(pr)
{
	char s[256];
	PAR_STRINGEOL(s,250);
	if (settings.proberPort>=0)      // CG
	{
		printf(" REQ %s\n", s);
		char *answer = prober.printf("%s", s);
		printf(" RSP %s\n", answer);
	}
	if (settings.proberPort==-2)      // CG
	{
		printf("Prober Nucleus: REQ %s\n", s);
		printf("Prober Nucleus: RSP %s\n", prober_nucleus.Execute(s).c_str());
	}
	return true;
}


CMD_PROC(sep)
{
	if (settings.proberPort>=0)      // CG
		prober.printf("MoveChuckSeparation");
	if (settings.proberPort==-2)      // CG
		printf("Prober Nucleus: MoveDown: %s\n", prober_nucleus.MoveDown().c_str());
	return true;
}


CMD_PROC(contact)
{
	if (settings.proberPort>=0)      // CG
		prober.printf("MoveChuckContact");
	if (settings.proberPort==-2)      // CG
		printf("Prober Nucleus: MoveUp: %s\n", prober_nucleus.MoveUp().c_str());
	return true;
}


bool test_wafer()
{
	int x, y;
	int bin;
	bool repeat;

	g_chipdata.Invalidate();

	if (!ReportWafer()) return true;
	if (!ReportChip(x,y)) return true;
	g_chipdata.nEntry = nEntry;

	GetTimeStamp(g_chipdata.startTime);
	Log.timestamp("BEGIN");
	if (!DEBUG_NODTB)                  // CG
	{
		tb.SetLed(0x10);
		bin = settings.rocType == 0 ? TestRocAna::test_roc(repeat) : TestRocDig::test_roc(repeat);
		tb.SetLed(0x00);
		tb.Flush();
	}
	else
	{
		// CG dummy code when there is no DTB/USB communication
        std::string resp = prober_nucleus.Execute(":mov:prob:abs:ind?");
        if (resp[0]=='@')
		{
			printf("Prober Nucleus: %s\n", resp.c_str());
            return false;
		}
		else
		{
            int dieIndex = std::stoi(resp);
		    bin = dieIndex % 10;
		    if (dieIndex % 5 == 0) 
		    {
			    //simulate a bad chip
			    bin = dieIndex % 10;
			    repeat = true;
		    }
		    else 
		    {
			    //simulate a good chip
			    bin = 0;
			    repeat = false;
		    }
		}
	}
	
	GetTimeStamp(g_chipdata.endTime);
	Log.timestamp("END");
	Log.puts("\n");
	Log.flush();
	printf("%3i\n", bin);

	if (settings.proberPort>=0)     // CG
		printf("RSP %s\n", prober.printf("BinMapDie %i", bin));
	//if (settings.port_prober==-2)  // CG
	//	prober.printf("BinMapDie %i", bin); ??? Nucleus ???

	return true;
}


bool test_chip(char chipid[])
{
	nEntry++;

	g_chipdata.Invalidate();
	g_chipdata.nEntry = nEntry;
	printf("#%05i: %s -> ", nEntry, chipid);
	fflush(stdout);
	Log.section("CHIP1", false);
	Log.printf(" %s\n", chipid);
	strcpy(g_chipdata.chipId, chipid);

	GetTimeStamp(g_chipdata.startTime);
	Log.timestamp("BEGIN");

	bool repeat;
	int bin;
	if (!DEBUG_NODTB)                  // CG
	{
		tb.SetLed(0x10);
		bin = settings.rocType == 0 ? TestRocAna::test_roc(repeat) : TestRocDig::test_roc(repeat);
		tb.SetLed(0x00);
		tb.Flush();
	}
	else
	{
        // CG dummy code when there is no DTB/USB communication
        std::string resp = prober_nucleus.Execute(":mov:prob:abs:ind?");
        if (resp[0]=='@')
		{
			printf("Prober Nucleus: %s\n", resp.c_str());
            return false;
		}
		else
		{
            int dieIndex = std::stoi(resp);
            bin = dieIndex % 10;
		    if (dieIndex % 5 == 0) 
		    {
			    //simulate a bad chip
			    bin = dieIndex % 10;
			    repeat = true;
		    }
		    else 
		    {
			    //simulate a good chip
			    bin = 0;
			    repeat = false;
		    }
		}
	}

	GetTimeStamp(g_chipdata.endTime);
	Log.timestamp("END");
	Log.puts("\n");
	Log.flush();

	printf("%3i\n", bin);

	return true;
}


CMD_PROC(test)
{

	if ((settings.proberPort >= 0) || (settings.proberPort == -2)) // CG
	{
		test_wafer();
	}
	else
	{
		char id[42];
		PAR_STRINGEOL(id,40);
		test_chip(id);
	}

//	FILE *f = fopen("g_chipdata.txt", "wt");
//	if (f) { g_chipdata.Save(f);  fclose(f); }

	return true;
}


#define CSX   8050
#define CSY  10451

const int CHIPOFFSET[4][4][2] =
{	// from -> to  0           1           2           3
	/*   0  */ { {   0,   0},{-CSX,   0},{   0,-CSY},{-CSX,-CSY} },
	/*   1  */ { { CSX,   0},{   0,   0},{ CSX,-CSY},{   0,-CSY} },
	/*   2  */ { {   0, CSY},{-CSX, CSY},{   0,   0},{-CSX,   0} },
	/*   3  */ { { CSX, CSY},{   0, CSY},{ CSX,   0},{   0,   0} },
};

bool ChangeChipPos(int pos)
{
	if (settings.proberPort>=0)      // CG
	{
		int rsp;
		char *answer = prober.printf("MoveChuckSeparation");
		if (sscanf(answer, "%i", &rsp)!=1) rsp = -1;
		if (rsp != 0) { printf(" RSP %s\n", answer); return false; }

		int x = CHIPOFFSET[chipPos][pos][0];
		int y = CHIPOFFSET[chipPos][pos][1];

		answer = prober.printf("MoveChuckPosition %i %i H", x, y);
		if (sscanf(answer, "%i", &rsp)!=1) rsp = -1;
		if (rsp != 0) { printf(" RSP %s\n", answer); return false; }

		answer = prober.printf("SetMapHome");
		if (sscanf(answer, "%i", &rsp)!=1) rsp = -1;
		if (rsp != 0) { printf(" RSP %s\n", answer); return false; }
	}
	if (settings.proberPort==-2)      // CG
	{
        std::string result = prober_nucleus.Execute(":mov:prob:abs:ind?");
        if (result[0]=='@')
		{
			printf("Prober Nucleus: %s\n", result.c_str());
            return false;
		}
		else
		{
		    int dieIndex = std::stoi(result);
		    printf("Prober Nucleus: MoveAbsDieSubsite(%d,%d): %s\n", dieIndex, pos+1, prober_nucleus.MoveAbsDieSubsite(dieIndex,pos+1).c_str());
		}
	}

	chipPos = pos;
	return true;
}


CMD_PROC(chippos)
{
	char s[4];
	PAR_STRING(s,2);
	if (s[0] >= 'a') s[0] -= 'a' - 'A';
	//if (s[0] == 'B') return true; // chip B not existing  // CG commented out, does exist!

	int i;
	for (i=0; i<4; i++)
	{
		if (s[0] == chipPosChar[i])
            return ChangeChipPos(i);   // CG code change
		//{
		//	ChangeChipPos(i);
		//	return true;
		//}
	}
	//return true; 
    printf("chippos parameter must be a, b, c or d\n"); // CG code adjustment
    return false;
}


CDefectList deflist[4];


bool goto_def(int i)
{
	int x, y;
	int rsp = 0;                          // CG
	if (!deflist[chipPos].get(i, x, y))   // CG
	{
		printf("Prober Nucleus: Subsite=%d, End Of Wafer\n", chipPos);
		return false;
	}

	if (settings.proberPort>=0)           // CG
	{
		char *answer = prober.printf("StepNextDie %i %i", x, y);
		if (sscanf(answer, "%i", &rsp)!=1) rsp = -1;
		if (rsp!=0) printf(" RSP %s\n", answer);
	}
	if (settings.proberPort==-2)         // CG
	{
		printf("Prober Nucleus: MoveDown: %s\n", prober_nucleus.MoveDown().c_str());
		char s1[50] = ":mov:prob:abs:die ";
		char s2[10] = "";
		sprintf_s(s2, "%d ", x);
		strcat_s(s1, s2);
		sprintf_s(s2, "%d ", y);
		strcat_s(s1, s2);
		printf("Prober Nucleus: Execute(%s)\n", s1, prober_nucleus.Execute(s1).c_str());
		char s3[50] = ":mov:prob:abs:subs ";
		sprintf_s(s2, "%d ", chipPos+1);
		strcat_s(s3, s2);
		printf("Prober Nucleus: Execute(%s)\n", s3, prober_nucleus.Execute(s3).c_str());
		printf("Prober Nucleus: MoveUp: %s\n", prober_nucleus.MoveUp().c_str());
	}

	return rsp == 0;
}


bool go_TestDefects()
{
	if (deflist[chipPos].size() == 0) return true;

	printf(" Begin Defect Chip %c Test\n", chipPosChar[chipPos]);

	// goto first position
	int i = 0;
	if (!goto_def(i)) return false;

	prober.printf("MoveChuckContact");

	do
	{
		int x, y;
		g_chipdata.Invalidate();

		if (!ReportChip(x,y)) break;
		GetTimeStamp(g_chipdata.startTime);
		Log.timestamp("BEGIN");
		bool repeat;
		int bin;
		if (!DEBUG_NODTB) bin = settings.rocType == 0 ? TestRocAna::test_roc(repeat) : TestRocDig::test_roc(repeat);// CG
		GetTimeStamp(g_chipdata.endTime);
		Log.timestamp("END");
		Log.puts("\n");
		Log.flush();
		printf("%3i\n", bin);

		if (settings.proberPort>=0)      // CG
			prober.printf("BinMapDie %i", bin);
		//if (settings.port_prober==-2)  // CG
		//	prober.printf("BinMapDie %i", bin); ??? Nucleus ???

		if (keypressed())
		{
			printf(" wafer test interrupted!\n");
			break;
		}

		if (!DEBUG_NODTB) tb.mDelay(100);        // CG
		i++;
	} while (goto_def(i));

	if (settings.proberPort>=0)                 // CG
		prober.printf("MoveChuckSeparation");
	if (settings.proberPort==-2)                // CG
		printf("Prober Nucleus: MoveDown: %s\n", prober_nucleus.MoveDown().c_str());

	return true;
}


bool TestSingleChip(int &bin, bool &repeat)
{
	int x, y;
	g_chipdata.Invalidate();

	if (!ReportChip(x,y)) return false;
	GetTimeStamp(g_chipdata.startTime);
	Log.timestamp("BEGIN");
	if (!DEBUG_NODTB)                  // CG
	{
		tb.SetLed(0x10);
		bin = settings.rocType == 0 ? TestRocAna::test_roc(repeat) : TestRocDig::test_roc(repeat);
		tb.SetLed(0x00);
		tb.Flush();
	}
	else
	{
		// CG dummy code when there is no DTB/USB communication
        std::string result = prober_nucleus.Execute(":mov:prob:abs:ind?");
        if (result[0]=='@')
		{
			printf("Prober Nucleus: %s\n", result.c_str());
            return false;
		}
		else
		{
		    int dieIndex = std::stoi(result);
		    bin = dieIndex % 10;
		    if (dieIndex % 5 == 0) 
		    {
			    //simulate a bad chip
			    bin = dieIndex % 10;
			    repeat = true;
		    }
		    else 
		    {
			    //simulate a good chip
			    bin = 0;
			    repeat = false;
		    }
		}
	}

	if (0<bin && bin<13) deflist[chipPos].add(x,y);  // CG uncommented this line
	GetTimeStamp(g_chipdata.endTime);
	Log.timestamp("END");
	Log.puts("\n");
	Log.flush();
	printf("%3i\n", bin);
	return true;
}


bool go_TestChips()
{
	int nDie;
	int nSubsite;
	printf(" Begin Chip %c Test\n", chipPosChar[chipPos]);

	if (settings.proberPort>=0)  // CG
		prober.printf("MoveChuckContact");
	if (settings.proberPort==-2)  // CG
	{
        std::string result1 = prober_nucleus.Execute(":prob:ntes?");
        std::string result2 = prober_nucleus.Execute(":prob:nsub?");
        if (result1[0]=='@' || result2[0]=='@')
		{
			printf("Prober Nucleus: %s %s\n", result1.c_str(), result2.c_str());
            return false;
		}
		else
		{
		    nDie = std::stoi(result1);
		    nSubsite = std::stoi(result2);
		    printf("Prober Nucleus: MoveUp: %s\n", prober_nucleus.MoveUp().c_str());
		}
	}
	if (!DEBUG_NODTB) tb.mDelay(200);

	while (true)
	{
		int bin = 0;
		bool repeat;
		if (!TestSingleChip(bin,repeat)) break;

		int nRep = settings.errorRep;
		if (nRep > 0 && repeat)
		{
			if (settings.proberPort>=0)  // CG
			{
				prober.printf("BinMapDie %i", bin);
				prober.printf("MoveChuckSeparation");
				if (!DEBUG_NODTB) tb.mDelay(100);
				prober.printf("MoveChuckContact");
				if (!DEBUG_NODTB) tb.mDelay(200);
				if (!TestSingleChip(bin,repeat)) break;
				nRep--;
			}
			if (settings.proberPort==-2)  // CG
			{
				//prober.printf("BinMapDie %i", bin); ??? Nucleus ???
				printf("Prober Nucleus: MoveDown: %s\n", prober_nucleus.MoveDown().c_str());
				if (!DEBUG_NODTB) tb.mDelay(100);
				printf("Prober Nucleus: MoveUpn: %s\n", prober_nucleus.MoveUp().c_str());
				if (!DEBUG_NODTB) tb.mDelay(200);
				if (!TestSingleChip(bin,repeat)) break;
				nRep--;
			}
		}

		if (keypressed())
		{
			if (settings.proberPort>=0)  // CG
				prober.printf("BinMapDie %i", bin);
			//if (settings.port_prober==-2)  // CG
			//	prober.printf("BinMapDie %i", bin); ??? Nucleus ???
			printf(" wafer test interrupted!\n");
			break;
		}

		// prober step
		int rsp;
		int dieIndex;                  // CG
		if (settings.proberPort>=0)
		{
			char *answer = prober.printf("BinStepDie %i", bin);
			if (sscanf(answer, "%i", &rsp)!=1) rsp = -1;
			if (rsp != 0) printf("RSP %s\n", answer);
			if (!DEBUG_NODTB) tb.mDelay(100);
		}
		if (settings.proberPort==-2)  // CG
		{
            std::string result = prober_nucleus.Execute(":mov:prob:abs:ind?");
            if (result[0]=='@')
		    {
			    printf("Prober Nucleus: %s\n", result.c_str());
                return false;
		    }
		    else
		    {
		        dieIndex = std::stoi(result);
			}
		}

		// last chip ?
		if (settings.proberPort>=0)  // CG
		{
			if (rsp == 0)   // ok -> next chip
				continue;
			if (rsp == 703) // end of wafer -> return
			{
				prober.printf("MoveChuckSeparation");
				return true;
			}
		}
		if (settings.proberPort==-2)  // CG
		{
			printf("Prober Nucleus: dieIndex=%d, subsiteIndex=%d test done\n", dieIndex, chipPos);
			if (dieIndex==nDie)        // end of wafer -> return
			{
				printf("Prober Nucleus: Subsite=%d, End Of Wafer\n", chipPos);
				printf("Prober Nucleus: MoveDown: %s\n", prober_nucleus.MoveDown().c_str());
				return true;
			}
			else // ok -> move to next dieIndex(1 to 62), keep same subsite==chipPos(0 to 3)
			{
				printf("Prober Nucleus: MoveDown: %s\n", prober_nucleus.MoveDown().c_str());
				printf("Prober Nucleus: MoveAbsDieSubsite(%d,%d): %s\n", dieIndex+1, chipPos+1, prober_nucleus.MoveAbsDieSubsite(dieIndex+1,chipPos+1).c_str());
				printf("Prober Nucleus: MoveUp: %s\n", prober_nucleus.MoveUp().c_str());
				continue;
			}
		}

		printf(" prober error! test stopped\n");
		break;
	}

	if (settings.proberPort>=0)   // CG
		prober.printf("MoveChuckSeparation");
	if (settings.proberPort==-2)  // CG
		printf("Prober Nucleus: MoveDown: %s\n", prober_nucleus.MoveDown().c_str());

	return false;
}


CMD_PROC(go)
{
	static bool isRunning = false;

	char s[12];
	if (PAR_IS_STRING(s, 10))
	{
		if (strcmp(s,"init") == 0) { isRunning = false; }
		else if (strcmp(s,"cont") == 0) { isRunning = true; }
		else { printf(" illegal parameter");  return true; }
	}

	if (!isRunning)
	{
		ChangeChipPos(0);
		for (int k=0; k<4; k++) deflist[k].clear();
		if (settings.proberPort>=0)  // CG
		{
			ChangeChipPos(0);
			prober.printf("StepFirstDie");
		}
		if (settings.proberPort==-2)  // CG
		{
			printf("Prober Nucleus: MoveDown: %s\n", prober_nucleus.MoveDown().c_str());
			printf("Prober Nucleus: MoveFirstDie: %s\n", prober_nucleus.MoveFirstDie().c_str());
		}
		isRunning = true;
	}

	printf(" wafer test running\n");
	if (!ReportWafer()) return true;

	while (true)
	{
		// test chips
		if (!go_TestChips()) break;

		// test defect chips
		if (settings.proberPort>=0)  // CG
			prober.printf("StepFirstDie");
		if (settings.proberPort==-2)  // CG
			printf("Prober Nucleus: MoveFirstDie: %s\n", prober_nucleus.MoveFirstDie().c_str());
		if (!go_TestDefects()) break;

		// next chip position
		if (chipPos < 3)
		{
			if (chipPos != 0) // exclude chip B (1)
			{
				if (!ChangeChipPos(chipPos+1)) break;
			}
			else
			{
				if (!ChangeChipPos(chipPos+1)) break;
//				if (!ChangeChipPos(chipPos+2)) break; // exclude chip B (1)
			}
			if (settings.proberPort>=0)      // CG
			{
				char *answer = prober.printf("StepFirstDie");
				int rsp;
				if (sscanf(answer, "%i", &rsp)!=1) rsp = -1;
				if (rsp != 0)
				{
					printf(" RSP %s\n", answer);
					break;
				}
			}
			if (settings.proberPort==-2)      // CG
				printf("Prober Nucleus: MoveAbsDieSubsite(%d,%d): %s\n", 1, chipPos+1, prober_nucleus.MoveAbsDieSubsite(1,chipPos+1).c_str());
		}
		else
		{
			ChangeChipPos(0);
			isRunning = false;
			break;
		}
	}
	return true;
}


CMD_PROC(first)
{
	if (settings.proberPort>=0)       // CG
		printf("RSP %s\n", prober.printf("StepFirstDie"));
	if (settings.proberPort==-2)      // CG
		printf("Prober Nucleus: MoveFirstDie: %s\n", prober_nucleus.MoveFirstDie().c_str());
	return true;
}


CMD_PROC(next)
{
	if (settings.proberPort>=0)       // CG
		printf("RSP %s\n", prober.printf("StepNextDie"));
	if (settings.proberPort==-2)      // CG
		printf("Prober Nucleus: MoveNextDie: %s\n", prober_nucleus.MoveNextDie().c_str());
	return true;
}


CMD_PROC(goto)
{
	int x, y;
	if (settings.proberPort>=0)       // CG
	{
		PAR_INT(x, -100, 100);
		PAR_INT(y, -100, 100);
		char *msg = prober.printf("StepNextDie %i %i", x, y);
		printf("RSP %s\n", msg);
	}
	if (settings.proberPort==-2)      // CG
	{

        std::string result1 = prober_nucleus.Execute(":prob:ntes?");
        std::string result2 = prober_nucleus.Execute(":prob:nsub?");
        if (result1[0]=='@' || result2[0]=='@')
		{
			printf("Prober Nucleus: %s %s\n", result1.c_str(), result2.c_str());
            return false;
		}
		else
		{
		    int nDie = std::stoi(result1);
		    int nSubsite = std::stoi(result2);
		    printf("Prober Nucleus: MoveUp: %s\n", prober_nucleus.MoveUp().c_str());
            PAR_INT(x, 1, nDie);             // CG Nucleus Die Index for PSI46 map
		    PAR_INT(y, 1, nSubsite);         // CG Nucleus SubSite Index for PSI46 map
            printf("Prober Nucleus: MoveAbsDieSubsite: %s\n", prober_nucleus.MoveAbsDieSubsite(x,y).c_str());
		}
	}
	return true;
}



// -- Wafer Test Adapter commands ----------------------------------------
/*
CMD_PROC(vdreg)    // regulated VD
{
	double v = tb.GetVD_Reg();
	printf("\n VD_reg = %1.3fV\n", v);
	return true;
}

CMD_PROC(vdcap)    // unregulated VD for contact test
{
	double v = tb.GetVD_CAP();
	printf("\n VD_cap = %1.3fV\n", v);
	return true;
}

CMD_PROC(vdac)     // regulated VDAC
{
	double v = tb.GetVDAC_CAP();
	printf("\n V_dac = %1.3fV\n", v);
	return true;
}
*/
