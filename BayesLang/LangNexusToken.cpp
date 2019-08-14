#include "LangNexusToken.h"

LangNexusToken::LangNexusToken(istream &is, ostream &os)
:NxsToken(is),out(os)
{
}

void LangNexusToken::OutputComment(const NxsString &msg)
{
    cout << msg << endl;
    out << msg << endl;
}
