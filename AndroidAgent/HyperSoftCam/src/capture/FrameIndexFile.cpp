#include "capture/FrameIndexFile.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "utils/Logger.h"
#include "utils/ConvUtils.h"

namespace android {

const char *FrameIndexFile::FRAME_INDEX_FILE_PATH = "/data/local/tmp/frame_timestap_info";

FrameIndexFile::~FrameIndexFile()
{
    AUTO_LOG();
    if (IsOpen())
        Close();
}

bool FrameIndexFile::IsOpen()
{
    if (m_frame_index_file_fd < 0)
        return false;
    else
        return true;
}

bool FrameIndexFile::Open()
{
    if (!IsOpen()) {
        m_frame_index_file_fd = open(FRAME_INDEX_FILE_PATH,
                                     O_CREAT | O_RDWR | O_SYNC | O_TRUNC | O_APPEND,
                                     S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        if (m_frame_index_file_fd == -1) {
            DWLOGE("Cannot open file - %s.(%s)", FRAME_INDEX_FILE_PATH, strerror(errno));
            return false;
        }
    }

    return true;
}

void FrameIndexFile::Close()
{
    if (IsOpen())
        close(m_frame_index_file_fd);
}

void FrameIndexFile::Write(uint64_t frame_index)
{
    if (!IsOpen())
        if (!Open())
            return;

    char output_buffer[256] = {0};
    sprintf(output_buffer, "%llu#%llu\n", frame_index, GetCurrentTimeStamp() / 1000);
    ssize_t ret = write(m_frame_index_file_fd, output_buffer, strlen(output_buffer));
    if (ret == -1)
        DWLOGE("Failed in writing buffer to file.(%s)", strerror(errno));
}

}
