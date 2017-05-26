#ifndef VIDEO_STREAM_PARSER_H
#define VIDEO_STREAM_PARSER_H

#include <mfxcommon.h>

#define MSDK_ZERO_MEMORY(VAR)                    {memset(&VAR, 0, sizeof(VAR));}

class CVideoParser;

class CVideoParserObserver
{
public:
    virtual void OnNewFrame(CVideoParser* parser, mfxU64 frame_num) = 0;
    virtual void OnFrameSize(CVideoParser* parser, mfxU32 width, mfxU32 height) = 0;
};

class CVideoParser
{
public:
    CVideoParser() { m_nIFts = 0; }
    CVideoParser(CVideoParserObserver* observer, int phase = 0) : m_oObserver(observer), m_uPhase(phase) { m_iValue = 0; m_nBitsInValue = 0; m_nIFts = 0; }
    virtual ~CVideoParser() {}
    virtual inline void Input(mfxBitstream* mfxBS, mfxU32 offset, mfxU32 length) { m_uData = mfxBS->Data + offset; m_nDataLength = length; parse(); }
    virtual inline void Input(mfxU8* data, mfxU32 length) { m_uData = data; m_nDataLength = length; parse(); }
    virtual inline mfxI64 value() { mfxI64 value = m_iValue; m_iValue = 0; m_nBitsInValue = 0;  return value; }
    inline int Phase() { return (int)m_uPhase; }

protected:
    virtual void parse() = 0;

    mfxU8                   m_uPhase;           // used for receive or feed phase
    mfxU8*                  m_uData;
    mfxU32                  m_nDataLength;
    CVideoParserObserver*   m_oObserver;
    mfxU64                  m_nOffsetInStream;  // first unread bit position

    mfxI64                  m_iValue;
    mfxU8                   m_nBitsInValue;     // first unwrite bit position
    mfxU32                  m_nIFts;
};

#endif