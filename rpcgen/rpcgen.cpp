// rpcgen.cpp

#include "stdafx.h"


char hdrFileName[] = "pixel_dtb.h";
char className[] = "CTestboard";


char timestamp[80];

void CreateTimeStamp()
{
	time_t now = time(0);
	struct tm t;
#ifdef _WIN32
	if (!localtime_s(&t, &now))
#else
	if (!localtime_r(&now, &t))
#endif
	{
		if (!strftime(timestamp, 79, "%d.%m.%Y %H:%M:%S", &t)) timestamp[0] = 0;
	}
	else timestamp[0] = 0;
}


typedef list<string> functList;
typedef list<string>::iterator functListIterator;


bool ReadFunctDefinitions(const char *filename, functList &fl)
{
	string s;
	ifstream src(filename);
	if (!src) { printf("ERROR: could not open source file\n"); return false; }

	while (!src.eof())
	{
		getline(src,s);
		unsigned int found = s.find_last_of('$');
		if (found == string::npos) continue;
		fl.push_back(s);
	}

	src.close();
    
    return true;
}



// === generate server code ====================================================

void GenerateServerEntry(FILE *f, unsigned int cmd, const char *fname, const char *parameter)
{
	string cmdId = string("rpc__") + fname + "$" + parameter;

	// read parameter list
	CParameterList plist;
	plist.Read(parameter);

	// generate declaration
	fprintf(f,
		"bool %s(CRpcIo &rpc_io, rpcMessage &msg)\n"
		"{\n"
		"\tif (!msg.CheckSize(%u)) return false;\n",
		cmdId.c_str(), plist.GetTotalParBytes());
	plist.WriteAllDtbSendPar(f);
	plist.WriteAllDtbSendDat(f);

	// call function
	plist.WriteDtbFunctCall(f, fname);

	if (plist.HasRetValues())
	{
		fprintf(f, "\tmsg.Create(%u);\n", cmd);
		plist.WriteAllDtbRecvPar(f);
		fputs("\tif (!msg.Send(rpc_io)) return false;\n", f);
		plist.WriteAllDtbRecvDat(f);
		fputs("\trpc_io.Flush();\n", f);
	}

	fputs("\treturn true;\n}\n\n", f);
}


void GenerateServerCodeHeader(FILE *f)
{
	fprintf(f,
		"// RPC functions for DTB\n"
		"// created: %s\n"
		"// This is an auto generated file\n"
		"// *** DO NOT EDIT THIS FILE ***\n\n"
		"#include \"%s\"\n"
		"const char rpc_timestamp[] = \"%s\";\n\n"
		"extern CRpcError rpc_error;\n"
		"extern %s tb;\n\n",
		timestamp, hdrFileName, timestamp, className
	);
}


void GenerateServerCode(functList &fl, FILE *f)
{
	string name;
	string parameter;

	try
	{
		unsigned int cmd = 0;
		functListIterator i;
		for (i = fl.begin(); i != fl.end(); i++)
		{
			unsigned int found = i->find_last_of('$');
			name = i->substr(0,found);
			parameter = i->substr(found+1);
			GenerateServerEntry( f,  cmd, name.c_str(), parameter.c_str());
			cmd++;
		}
	}
	catch(const char*e) { printf("ERROR: %s\n", e); }
}


void GenerateServerCodeTrailer(functList &fl, FILE *f)
{
	// generate command dispatcher for DTB code
	fprintf(f, "const uint16_t rpc_cmdListSize = %lu;\n\n", fl.size());
	fprintf(f,
		"const CRpcCall rpc_cmdlist[] =\n{\n"
	);
	unsigned int cmd = 0;
	list<string>::iterator i;
	for (i = fl.begin(); i != fl.end(); i++)
	{
		if (cmd == 0)
			fprintf(f, "\t/* %5u */ { rpc__%s, \"%s\" }", cmd, i->c_str(), i->c_str());
		else
			fprintf(f, ",\n\t/* %5u */ { rpc__%s, \"%s\" }", cmd, i->c_str(), i->c_str());
		cmd++;
	}
	fprintf
	(
		f,
		"\n};\n\n"
		"void rpc_Dispatcher(CRpcIo &rpc_io)\n"
		"{\n"
		"\trpcMessage msg;\n"
		"\twhile (true)\n"
		"\t{\n"
		"\t\tmsg.Receive(rpc_io);\n"
		"\t\tif (rpc_error.HasError()) continue;\n"
		"\t\tif (msg.GetCmd() >= %u) continue;\n"
		"\t\trpc_cmdlist[msg.GetCmd()].call(rpc_io, msg);\n"
		"\t}\n}\n", (unsigned int)(fl.size())
	);
}


// === generate client code ===================================================

void GenerateClientEntry(FILE *f, unsigned int cmd, const char *fname, const char *parameter)
{
	string cmdId = string("rpc__") + fname + "$" + parameter;

	// read parameter list
	CParameterList plist;
	plist.Read(parameter);

	plist.WriteCDeclaration(f, fname);
	fprintf(f,
		"{\n"
		"\tuint16_t rpc_clientCallId = rpc_GetCallId(%u);\n"
		"\tRPC_THREAD_LOCK\n"
		"\trpcMessage msg;\n"
		"\tmsg.Create(rpc_clientCallId);\n",
		cmd
	);
	plist.WriteAllSendPar(f);
	fputs("\tmsg.Send(*rpc_io);\n", f);
	plist.WriteAllSendDat(f);
	
	if (plist.HasRetValues())
	{
		fprintf(f,
			"\trpc_io->Flush();\n"
			"\tmsg.Receive(*rpc_io);\n"
			"\tmsg.Check(rpc_clientCallId,%u);\n",
			plist.GetTotalRetBytes());
		plist.WriteAllRecvPar(f);
		plist.WriteAllRecvDat(f);
	}
	fprintf(f, "\tRPC_THREAD_UNLOCK\n");
	if (plist.begin()->HasRetValue())
		fputs("\treturn rpc_par0;\n", f);
	fputs("}\n\n", f);
}


void GenerateClientCodeHeader(functList &fl, FILE *f)
{
	fprintf(f,
		"// RPC functions\n"
		"// created: %s\n"
		"// This is an auto generated file\n"
		"// *** DO NOT EDIT THIS FILE ***\n\n"
		"#include \"%s\"\n\n"
		"const char %s::rpc_timestamp[] = \"%s\";\n\n",
		timestamp, hdrFileName, className, timestamp
	);

	// create function table
	fprintf(f, "const unsigned int %s::rpc_cmdListSize = %i;\n\n", className, int(fl.size()));
	fprintf(f, "const char *%s::rpc_cmdName[] =\n{\n", className);
	unsigned int cmd = 0;
	list<string>::iterator i;
	for (i = fl.begin(); i != fl.end(); i++)
	{
		if (cmd == 0)
			fprintf(f, "\t/* %5u */ \"%s\"", cmd, i->c_str());
		else
			fprintf(f, ",\n\t/* %5u */ \"%s\"", cmd, i->c_str());
		cmd++;
	}
	fprintf(f, "\n};\n\n");
}


bool GenerateClientCode(functList &fl, FILE *f)
{
	string name;
	string parameter;

	try
	{
		unsigned int cmd = 0;
		list<string>::iterator i;
		for (i = fl.begin(); i != fl.end(); i++)
		{
			unsigned int found = i->find_last_of('$');
			name = i->substr(0,found);
			parameter = i->substr(found+1);
			GenerateClientEntry(f, cmd, name.c_str(), parameter.c_str());
			cmd++;
		}
	}
	catch(const char*e) { printf("ERROR: %s\n", e); }

	return true;
}



void Help(const char *msg = 0)
{
	if (msg) printf("%s!\n", msg);
	printf("rpcgen <source> -h<host rpc> -d<dtb rpc>");
}


int main(int argc, char* argv[])
{
	char *srcFileName = 0;
	char *dtbFileName = 0;
	char *hstFileName = 0;

	// --- read command line parameter --------------------------------------
	if (argc < 2 || argc > 4) { Help("Wong number of arguments"); return 1; }
	srcFileName = argv[1];
	if (srcFileName == 0) { Help(); return 1; }
	for (int i=2; i<argc; i++)
	{
		if (argv[i][0] != '-') Help("Wrong argument");
		switch (argv[i][1])
		{
			case 'd': dtbFileName = &(argv[i][2]); break;
			case 'h': hstFileName = &(argv[i][2]); break;
			default: Help("Wong argument opti"); return 1;
		}
	}

	CreateTimeStamp();

	// --- analyze C++ file ------------------------------------------
	FILE *f;
	functList cmdList;

	CppParser parser;
	try
	{
		parser.Open(srcFileName);
		while(true)
		{
			parser.GetRpcExport();
			parser.GetFunctionDecl();
			cmdList.push_back(parser.GetFname() + '$' + parser.GetFparam());

			printf("%s$%s\n", parser.GetFname().c_str(), parser.GetFparam().c_str());
		}
	}
	catch (CPError e)
	{
		if (e.error != CPError::END_OF_FILE) { e.What(); return 1; }
	}
	parser.Close();

	// --- create server functions
	if (dtbFileName)
	{
		f = fopen(dtbFileName, "wt");
		if (!f) { printf("ERROR: could not create DTB code file\n"); return 1; }
		GenerateServerCodeHeader(f);
		GenerateServerCode(cmdList, f);
		GenerateServerCodeTrailer(cmdList, f);
		fclose(f);
	}

	// generate host functions
	if (hstFileName)
	{
		f = fopen(hstFileName, "wt");
		if (!f) { printf("ERROR: could not create host code file\n"); return 2; }
		GenerateClientCodeHeader(cmdList, f);
		GenerateClientCode(cmdList, f);
		fclose(f);
	}

	//	system("pause");
	return 0;
}
