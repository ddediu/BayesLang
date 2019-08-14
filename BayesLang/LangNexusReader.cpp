#include "LangNexusReader.h"


LangNexusReader::LangNexusReader(const char *infname)
:NxsReader()
{
    inf.open(infname, ios::binary);
}

LangNexusReader::~LangNexusReader()
{
    inf.close();
}

bool LangNexusReader::EnteringBlock(NxsString blockName)
{
    cout << "Reading \"" << blockName << "\" block..." << endl;

    // Returning true means it is ok to delete any data associated with
    // blocks of this type read in previously
    //
    return true;
}

void LangNexusReader::ExecuteStarting()
{
}

void LangNexusReader::ExecuteStopping()
{
}

void LangNexusReader::SkippingBlock(NxsString blockName)
{
    cout << "Skipping unknown block (" << blockName << ")..." << endl;
}

void LangNexusReader::SkippingDisabledBlock(NxsString )
{
}

void LangNexusReader::OutputComment(const NxsString &msg)
{
    cout << msg;
}

void LangNexusReader::NexusError(NxsString msg, file_pos pos, long line, long col)
{
    cerr << endl;
    cerr << "Error found at line " << line;
    cerr << ", column " << col;
    cerr << " (file position " << pos << "):" << endl;
    cerr << msg << endl;

    exit(0);
}

