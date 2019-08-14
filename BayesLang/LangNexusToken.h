#ifndef LANGNEXUSTOKEN_H
#define LANGNEXUSTOKEN_H

#include "ncl/ncl.h"

class LangNexusToken : public NxsToken
{
    public:
        LangNexusToken(istream &is, ostream &os);
        void OutputComment(const NxsString &msg);

    protected:
    private:
        ostream &out;
};

#endif // LANGNEXUSTOKEN_H
