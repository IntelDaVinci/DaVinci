#ifndef _FRAME_INDEX_FILE_H_
#define _FRAME_INDEX_FILE_H_

#include <sys/types.h>

namespace android {

class FrameIndexFile {
public:
    FrameIndexFile() : m_frame_index_file_fd(-1), m_is_open(false) {};
    ~FrameIndexFile();
    void Write(uint64_t frame_index);

private:
    bool IsOpen();
    bool Open();
    void Close();

private:
    int  m_frame_index_file_fd;
    bool m_is_open;
    static const char *FRAME_INDEX_FILE_PATH;
};

}

#endif
