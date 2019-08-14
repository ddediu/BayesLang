#ifndef LANGNEXUSREADER_H
#define LANGNEXUSREADER_H


#include "ncl/ncl.h"

class LangNexusReader : public NxsReader
{
    public:
		ifstream inf;
		ofstream outf;

        LangNexusReader(const char *infname);
        virtual ~LangNexusReader();

        void ExecuteStarting();
        void ExecuteStopping();

        bool EnteringBlock(NxsString blockName);

        void SkippingBlock(NxsString blockName);
        void SkippingDisabledBlock(NxsString );
        void OutputComment(const NxsString &msg);
        void NexusError(NxsString msg, file_pos pos, long line, long col);

    protected:
    private:
};

#endif // LANGNEXUSREADER_H
