/*
----------------------------------------------------------------------------------------

"Copyright 2014-2015 Intel Corporation.

The source code, information and material ("Material") contained herein is owned by Intel Corporation
or its suppliers or licensors, and title to such Material remains with Intel Corporation or its suppliers
or licensors. The Material contains proprietary information of Intel or its suppliers and licensors. 
The Material is protected by worldwide copyright laws and treaty provisions. No part of the Material may 
be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed or disclosed
in any way without Intel's prior express written permission. No license under any patent, copyright or 
other intellectual property rights in the Material is granted to or conferred upon you, either expressly, 
by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights 
must be express and approved by Intel in writing.

Unless otherwise agreed by Intel in writing, you may not remove or alter this notice or any other notice 
embedded in Materials by Intel or Intel's suppliers or licensors in any way."
-----------------------------------------------------------------------------------------
*/

#ifndef __DAVINCI_COMMON_HPP__
#define __DAVINCI_COMMON_HPP__

#include <vector>
#include <map>
#include <unordered_map>

#include "xercesc/dom/DOM.hpp"

#include "boost/smart_ptr/shared_ptr.hpp"
#include "boost/smart_ptr/enable_shared_from_this.hpp"
#include "boost/asio.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/process.hpp"
#include "boost/thread/thread.hpp"
#include "boost/chrono/chrono.hpp"
#include "boost/interprocess/streams/vectorstream.hpp"
#include "boost/atomic.hpp"
#include "boost/algorithm/string/trim.hpp"
#include "boost/process.hpp"

#include "opencv2/opencv.hpp"
#include <locale>
#include <codecvt>

#include "DaVinciDefs.hpp"
#include "DaVinciStatus.hpp"
#include "Shape.hpp"

namespace DaVinci
{
    using namespace std;
    using namespace cv;
    using namespace xercesc;

    // forward declaration for DeviceControl types
    class TargetDevice;
    enum class Orientation;

    extern const cv::Scalar Green;
    extern const cv::Scalar Red;
    extern const cv::Scalar Yellow;
    extern const cv::Scalar Blue;
    extern const cv::Scalar White;
    extern const cv::Scalar Black;

    extern const Rect EmptyRect;
    extern const Point EmptyPoint;
    extern const Point CenterPoint;
    extern const Mat EmptyMat;
    extern const int maxWidthHeight;
    extern const string Characters[62];

    enum UType
    {
        UTYPE_X = 0, // By X
        UTYPE_Y = 1, // BY Y
        UTYPE_W = 2, // By Width
        UTYPE_H = 3, // By Height
        UTYPE_L = 4, // By Length
        UTYPE_CX = 5, // By Center X
        UTYPE_CY = 6, // By Center Y
        UTYPE_CC = 7, // By Center Coordinate
        UTYPE_AREA = 8 // By area
    };

#define SIGLETON_CHILD_CLASS(classname) \
private:\
    classname();\
    friend class SingletonBase<classname>;

    template<typename T>
    class SingletonBase : public boost::enable_shared_from_this<SingletonBase<T>>, public boost::noncopyable
    {
    public:
        static T & Instance()
        {
            if (!initialized)
            {
                boost::lock_guard<boost::mutex> lock(singletonMutex);
                if (!initialized)
                {
                    assert(!instance);
                    instance = boost::shared_ptr<T>(new T());
                    initialized = true;
                }
            }
            return *instance;
        }

        virtual ~SingletonBase()
        {
        }

    private:
        static boost::atomic<bool> initialized;
        static boost::shared_ptr<T> instance;
        static boost::mutex singletonMutex;
    };

    template <typename T>
    boost::atomic<bool> SingletonBase<T>::initialized(false);

    template <typename T>
    boost::shared_ptr<T> SingletonBase<T>::instance;

    template <typename T>
    boost::mutex SingletonBase<T>::singletonMutex;

    /// <summary> Thread sleep. </summary>
    ///
    /// <param name="millisecond"> The millisecond. </param>
    inline void ThreadSleep(int millisecond)
    {
        boost::this_thread::sleep_for(boost::chrono::milliseconds(millisecond));
    }

    // SetThreadName http://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx

#if (defined WIN32)
    const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push, 8)
    typedef struct THREADNAME_INFO {
        DWORD dwType; // Must be 0x1000.
        LPCSTR szName; // Pointer to name (in user addr space).
        DWORD dwThreadID; // Thread ID (-1=caller thread).
        DWORD dwFlags; // Reserved for future use, must be zero.
    } THREADNAME_INFO;
#pragma pack(pop)

    inline void _SetThreadName(DWORD threadId, const char* threadName) 
    {
        THREADNAME_INFO info;
        info.dwType = 0x1000;
        info.szName = threadName;
        info.dwThreadID = threadId;
        info.dwFlags = 0;
        __try 
        {
            RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
        }
        __except(EXCEPTION_EXECUTE_HANDLER) 
        {
        }
    }
#endif

    inline void SetThreadName(boost::thread::id threadId, std::string threadName) 
    {
#if (defined WIN32)
        // convert string to char*
        const char* cchar = threadName.c_str();
        // convert HEX string to DWORD
        unsigned int dwThreadId;
        std::stringstream ss;
        ss << std::hex << threadId;
        ss >> dwThreadId;

        _SetThreadName((DWORD)dwThreadId, cchar);
#else
        DAVINCI_LOG_WARNING << "Not supported now." << endl;
#endif
    }


    /// <summary> Attempts to parse an integral from the given string </summary>
    ///
    /// <typeparam name="Target"> Type of the integral. </typeparam>
    /// <param name="str">    The string representing the integral. </param>
    /// <param name="target"> [in,out] The parsed integral. </param>
    ///
    /// <returns> true if it succeeds, false if it fails. On failure, the target is set to zero. </returns>
    template <typename Target>
    inline bool TryParse(const string &str, Target &target)
    {
        if (str.empty())
            return false;

        try
        {
            target = boost::lexical_cast<Target>(boost::algorithm::trim_copy(str));
            return true;
        }
        catch (const boost::bad_lexical_cast &)
        {
            target = 0;
            return false;
        }
    }

    /// <summary> Gets bytes from int (little endian). </summary>
    ///
    /// <param name="value"> The value. </param>
    ///
    /// <returns> The bytes from int. </returns>
    inline vector<unsigned char> GetBytesFromUInt(unsigned int value)
    {
        vector<unsigned char> bytes(4);
        bytes[3] = static_cast<unsigned char>(value >> 24);
        bytes[2] = static_cast<unsigned char>(value >> 16);
        bytes[1] = static_cast<unsigned char>(value >> 8);
        bytes[0] = static_cast<unsigned char>(value);
        return bytes;
    }

    template <typename Target>
    bool FEquals(Target l, Target r, Target e = DBL_EPSILON)
    { 
        return fabs(l - r) < e;
    }

    template <typename Target>
    bool FSmaller(Target l, Target r, Target e = DBL_EPSILON)
    {
        return r - l >= e;
    }

    template <typename Target>
    bool FSmallerE(Target l, Target r, Target e = DBL_EPSILON) 
    {
        return r - l > -e;
    }

    template <typename Target1, typename Target2>
    bool MapContainKey(Target1 key, std::unordered_map<Target1, Target2> map) 
    {
        for (auto pair : map)
        {
            if(pair.first == key)
            {
                return true;
            }
        }

        return false;
    }

    class LineSegment
    {
    public:
        LineSegment();
        LineSegment(Point p1, Point p2);

        Point p1;
        Point p2;
    };

    class LineSegmentD
    {
    public:
        LineSegmentD(Point2d p1, Point2d p2);

        Point2d p1;
        Point2d p2;
    };

    /// <summary> 
    /// Gets available local port.
    /// 
    /// Note that this function does not handle race condition when the available
    /// port is found but then acquired later by other process.
    /// </summary>
    ///
    /// <param name="searchBase"> The base port to start searching. </param>
    ///
    /// <returns> The available local port or -1 if no local port is available. </returns>
    unsigned short GetAvailableLocalPort(unsigned short searchBase);

    /// <summary> Executes the process synchronously. </summary>
    ///
    /// <param name="executable"> The executable. </param>
    /// <param name="args">       The arguments. </param>
    /// <param name="outStr">  [in,out] The output string (stderr redirected to stdout). </param>
    /// <param name="workingDir"> (Optional) the working dir where the process starts </param>
    /// <param name="timeout">    (Optional) The timeout in milli-seconds. </param> <param name="readOutput"> (Optional) Whether we want to read stdout and stderr. </param>
    ///
    /// <returns> The DaVinciStatus. </returns>
    DaVinciStatus RunProcessSync(const string &executable, const string &args, const boost::shared_ptr<ostringstream> &outStr = nullptr, const string &workingDir = ".", int timeout = 0);

    /// <summary> Executes the process synchronously and delivers raw output stream if needed. </summary>
    ///
    /// <param name="executable"> The executable. </param>
    /// <param name="args">       The arguments. </param>
    /// <param name="outStream">  (Optional) The output stream (stderr redirected to stdout). </param>
    /// <param name="workingDir"> (Optional) the working dir. </param>
    /// <param name="timeout">    (Optional) the timeout. </param>
    ///
    /// <returns> The DaVinciStatus. </returns>
    DaVinciStatus RunProcessSyncRaw(const string &executable, const string &args, const boost::shared_ptr<boost::interprocess::basic_ovectorstream<vector<char>>> &outStream = nullptr, const string &workingDir = ".", int timeout = 0);

    /// <summary> Executes the shell command operation. </summary>
    ///
    /// <param name="shellCommand"> The shell command. </param>
    /// <param name="outStream">    (Optional) stream to write data to. </param>
    /// <param name="workingDir">   (Optional) the working dir. </param>
    /// <param name="timeout">      (Optional) the timeout. </param>
    ///
    /// <returns> The DaVinciStatus. </returns>
    DaVinciStatus RunShellCommand(const string &shellCommand, const boost::shared_ptr<ostringstream> &outStr = nullptr, const string &workingDir = ".", int timeout = 0,
        boost::process::environment env = boost::process::environment());

    /// <summary> Executes the process asynchronously </summary>
    ///
    /// <param name="executable"> The executable. </param>
    /// <param name="args">       The arguments. </param>
    /// <param name="p">
    /// [in,out] The pointer to the process
    /// </param>
    /// <param name="workingDir"> (Optional) the working dir. </param>
    ///
    /// <returns> The DaVinciStatus. </returns>
    DaVinciStatus RunProcessAsync(const string &executable, const string &args, boost::shared_ptr<boost::process::process> &p, const string &workingDir = ".");

    inline cv::Mat RotateImageInternal(const cv::Mat &image, int type);
    inline cv::Mat RotateImage270(const cv::Mat &image);
    inline cv::Mat RotateImage90(const cv::Mat &image);

    inline double GetCurrentMillisecond()
    {
        return (double)cv::getTickCount() * 1000.0 / cv::getTickFrequency();
    }

    /// <summary> Rotate image 180 degree </summary>
    ///
    /// <param name="image"> The image. </param>
    ///
    /// <returns> A cv::Mat. </returns>
    inline cv::Mat RotateImage180(const cv::Mat &image)
    {
        cv::Mat rotated(image.rows, image.cols, image.type());
        cv::flip(image, rotated, -1); // vertical & horizontal
        return rotated;
    }

    inline void CpuPause()
    {
#ifdef WIN32
        _mm_pause();
#else
        __asm__ __volatile__( "rep; nop" : : : "memory" );
#endif
    }

    cv::Mat RotateFrameUp(const Mat &frame, Orientation orientation);

    Rect RotateROIUp(Rect frameROI, Size frameSize, Orientation orientation);

    cv::Mat RotateFrameUpForCommandScreenCapture(const Mat &frame, Orientation orientation);

    cv::Mat RotateFrameUp(const Mat &frame);

    cv::Mat RotateFrameClockwise(const cv::Mat &frame, int rotateAngle);

    Point TransformDevicePoint(Point sPoint, Orientation orientation, Size source, Size target);

    DaVinciStatus ReadAllLines(const string& fileName,vector<std::string>& lines);

    DaVinciStatus ReadAllLinesFromStr(const string &str, vector<string> &lines);

    void ProjectPoints(std::vector<cv::Point2f>& points,const cv::Mat& H );
    cv::Mat LogSharpen(const cv::Mat& image);
    void VoteForUniqueness(const std::vector<std::vector<cv::DMatch>>& matchers, double uniquenessThreshold, cv::Mat& mask);

    template <typename Target>
    void XmlDocumentPtrDeleter(Target* tp)
    {
        if (0 == tp->getOwnerDocument())
            tp->release();
    }

    template <typename Target>
    void XmlNonDocumentPtrDeleter(Target* tp)
    {
        tp->release();
    }

    /// <summary> Convert XML string to string. </summary>
    ///
    /// <param name="xmlStr"> The XML string. </param>
    ///
    /// <returns> A string. </returns>
    string XMLStrToStr(const XMLCh *xmlStr);

    /// <summary> Convert string to XMLstring. </summary>
    ///
    /// <param name="xmlStr"> The XML string. </param>
    ///
    /// <returns> A xml str. </returns>
    boost::shared_ptr<XMLCh> StrToXMLStr(const string str);

    /// <summary> Write dom document into xml. </summary>
    ///
    /// <param name="domImp"> The DOM implementation. </param>
    ///
    /// <param name="domDocument"> The DOM document. </param>
    ///
    /// <param name="xmlFileName"> The xml file. </param>
    DaVinciStatus WriteDOMDocumentToXML(const boost::shared_ptr<DOMImplementation> domImp, const boost::shared_ptr<xercesc::DOMDocument> domDocument, const string xmlFileName);

    /// <summary> Creates dom element with null deleter. </summary>
    ///
    /// <param name="doc"> The document. </param>
    ///
    /// <returns> The new dom element null deleter. </returns>
    boost::shared_ptr<DOMElement> CreateDOMElementNullDeleter(const boost::shared_ptr<DOMDocument> &doc, const string &elementName);

    class AutoResetEvent
    {
    public:
        AutoResetEvent(bool initialSignalState = false);
        void Set();
        void Reset();
        bool WaitOne(int timeout = -1);
    private:
        boost::mutex event_mutex;
        boost::condition_variable event_cond;
        bool signaled;
    };

    /// <summary> Creates named interprocess mutex  </summary>
    /// Boost named mutex cannot release when process crash during mutex protected area, which will cause deadlock.
    /// While windows OS can release native mutex in this case.
    /// <param name="mutexname"> The mutex name. </param>
    class GlobalNamedMutex
    {
    public:
        GlobalNamedMutex(const string & mutexName);
        ~GlobalNamedMutex();
    private:
        HANDLE ghMutex;
    };

    /// <summary> Convert Vec2f to linesegment. </summary>
    ///
    /// <param name="line"> The houghline. </param>
    ///
    /// <returns> Linesegment. </returns>
    LineSegment Vec4iToLineSegment(const Vec4i &line);

    /// <summary> Convert Vec2f vector to linesegment vector. </summary>
    ///
    /// <param name="lines"> The houghlines. </param>
    ///
    /// <returns> Linesegment vector. </returns>
    vector<LineSegment> Vec4iLinesToLineSegments(const vector<Vec4i> &lines);

    /// <summary> Get horizontal lines. </summary>
    ///
    /// <param name="lines"> The line segments. </param>
    ///
    /// <returns> Horizontal lines. </returns>
    vector<LineSegment> GetHorizontalLineSegments(const vector<LineSegment> &lines);

    /// <summary> Provide sort functions. </summary>
    ///
    /// <param name="i"> The first line segment. </param>
    ///
    /// <param name="j"> The second line segment. </param>
    ///
    /// <returns> True for sorted. </returns>
    bool SortLineSegmentXGreater (const LineSegment &i, const LineSegment &j);

    /// <summary> Provide sort functions. </summary>
    ///
    /// <param name="i"> The first line segment. </param>
    ///
    /// <param name="j"> The second line segment. </param>
    ///
    /// <returns> True for sorted. </returns>
    bool SortLineSegmentYGreater (const LineSegment &i, const LineSegment &j);

    /// <summary> Provide sort functions. </summary>
    ///
    /// <param name="i"> The first line segment. </param>
    ///
    /// <param name="j"> The second line segment. </param>
    ///
    /// <returns> True for sorted. </returns>
    bool SortLineSegmentXSmaller (const LineSegment &i, const LineSegment &j);

    /// <summary> Provide sort functions. </summary>
    ///
    /// <param name="i"> The first line segment. </param>
    ///
    /// <param name="j"> The second line segment. </param>
    ///
    /// <returns> True for sorted. </returns>
    bool SortLineSegmentYSmaller (const LineSegment &i, const LineSegment &j);

    /// <summary> Provide sort functions. </summary>
    ///
    /// <param name="i"> The first rect. </param>
    ///
    /// <param name="j"> The second rect. </param>
    ///
    /// <returns> True for sorted. </returns>
    bool SortRectXGreater (const Rect &i, const Rect &j);

    /// <summary> Provide sort functions. </summary>
    ///
    /// <param name="i"> The first rect. </param>
    ///
    /// <param name="j"> The second rect. </param>
    ///
    /// <returns> True for sorted. </returns>
    bool SortRectYGreater (const Rect &i, const Rect &j);

    /// <summary> Provide sort functions. </summary>
    ///
    /// <param name="i"> The first rect. </param>
    ///
    /// <param name="j"> The second rect. </param>
    ///
    /// <returns> True for sorted. </returns>
    bool SortRectHGreater (const Rect &i, const Rect &j);

    /// <summary> Provide sort functions. </summary>
    ///
    /// <param name="i"> The first rect. </param>
    ///
    /// <param name="j"> The second rect. </param>
    ///
    /// <returns> True for sorted. </returns>
    bool SortRectCCGreater (const Rect &i, const Rect &j);

    /// <summary> Provide sort functions. </summary>
    ///
    /// <param name="i"> The first rect. </param>
    ///
    /// <param name="j"> The second rect. </param>
    ///
    /// <returns> True for sorted. </returns>
    bool SortRectXSmaller (const Rect &i, const Rect &j);

    /// <summary> Provide sort functions. </summary>
    ///
    /// <param name="i"> The first rect. </param>
    ///
    /// <param name="j"> The second rect. </param>
    ///
    /// <returns> True for sorted. </returns>
    bool SortRectYSmaller (const Rect &i, const Rect &j);

    /// <summary> Provide sort functions. </summary>
    ///
    /// <param name="i"> The first rect. </param>
    ///
    /// <param name="j"> The second rect. </param>
    ///
    /// <returns> True for sorted. </returns>
    bool SortRectHSmaller (const Rect &i, const Rect &j);

    /// <summary> Provide sort functions. </summary>
    ///
    /// <param name="i"> The first rect. </param>
    ///
    /// <param name="j"> The second rect. </param>
    ///
    /// <returns> True for sorted. </returns>
    bool SortRectCCSmaller (const Rect &i, const Rect &j);

    bool SortRectAreaGreater(const Rect &i, const Rect &j);

    bool SortRectAreaSmaller(const Rect &i, const Rect &j);

    /// <summary> Sort line segments. </summary>
    ///
    /// <param name="lineVector"> The line segments. </param>
    ///
    /// <param name="type"> The sort type. </param>
    ///
    /// <param name="isGreater"> Sort by greater. </param>
    ///
    /// <returns> return DaVinciStatusSuccess if the sort type is supported otherwise return error value. </returns>
    DaVinciStatus SortLineSegments(vector<LineSegment> &lineVector, enum UType type, bool isGreater);

    /// <summary> Sort rectangles. </summary>
    ///
    /// <param name="rects"> The rects. </param>
    ///
    /// <param name="type"> The sort type. </param>
    ///
    /// <param name="isGreater"> Sort by greater. </param>
    ///
    /// <returns> return DaVinciStatusSuccess if the sort type is supported otherwise return error value. </returns>
    DaVinciStatus SortRects(vector<Rect> &rectVector,  enum UType type, bool isGreater);

    /// <summary> Intersect two rectangles. </summary>
    ///
    /// <param name="rect1"> The first rect. </param>
    ///
    /// <param name="rect2"> The second rect. </param>
    ///
    /// <returns> True for intersection. </returns>
    bool IntersectWith(const Rect &rect1, const Rect &rect2);

    /// <summary> Contain rect2 in rect1. </summary>
    ///
    /// <param name="rect1"> The first rect. </param>
    ///
    /// <param name="rect2"> The second rect. </param>
    ///
    /// <returns> True for containing. </returns>
    bool ContainRect(const Rect &rect1, const Rect &rect2, int edge = 0);

    /// <summary> Query DOM tree by XPath. </summary>
    ///
    /// <param name="domDoc"> The dom document. </param>
    ///
    /// <param name="xpath"> The xpath. </param>
    ///
    /// <returns> DOMNode list. </returns>
    std::vector<boost::shared_ptr<DOMNode>> GetNodesFromXPath(const boost::shared_ptr<xercesc::DOMDocument> domDoc, const string xpath);

    /// <summary> Query DOM tree by XPath and specified attribute. </summary>
    ///
    /// <param name="domDoc"> The dom document. </param>
    ///
    /// <param name="xpath"> The xpath. </param>
    ///
    /// <param name="attributeName"> The attribute name. </param>
    ///
    /// <param name="attributeValue"> The attribute value. </param>
    ///
    /// <returns> DOMNode list. </returns>
    std::vector<boost::shared_ptr<DOMNode>> GetNodesFromXPath(const boost::shared_ptr<xercesc::DOMDocument> domDoc, const string xpath, const XMLCh *attributeName, const std::string attributeValue);

    /// <summary> Sort pair by integer key </summary>
    ///
    /// <typeparam name="Target"> Other type. </typeparam>
    /// <param name="p1"> The first pair. </param>
    /// <param name="p2"> The second pair. </param>
    ///
    /// <returns> true for sortetd. </returns>
    template <typename Target>
    inline bool SortPairByIntKeyGreaterT(pair<int, Target> &p1, pair<int, Target> &p2)
    {
        if(p1.first < p2.first)
            return true;
        else
            return false;
    }

    inline bool SortPairByIntKeyGreater(pair<int, int> &p1, pair<int, int> &p2)
    {
        return SortPairByIntKeyGreaterT(p1, p2);
    }

    /// <summary> Sort pair by integer key </summary>
    ///
    /// <typeparam name="Target"> Other type. </typeparam>
    /// <param name="p1"> The first pair. </param>
    /// <param name="p2"> The second pair. </param>
    ///
    /// <returns> true for sortetd. </returns>
    template <typename Target>
    inline bool SortPairByIntKeySmallerT(pair<int, Target> &p1, pair<int, Target> &p2)
    {
        if(p1.first > p2.first)
            return true;
        else
            return false;
    }

    inline bool SortPairByIntKeySmaller(pair<int, int> &p1, pair<int, int> &p2)
    {
        return SortPairByIntKeySmallerT(p1, p2);
    }


    /// <summary> Sort pair by integer value </summary>
    ///
    /// <typeparam name="Target"> Other type. </typeparam>
    /// <param name="p1"> The first pair. </param>
    /// <param name="p2"> The second pair. </param>
    ///
    /// <returns> true for sortetd. </returns>
    template <typename Target>
    inline bool SortPairByIntValueSmallerT(pair<Target, int> &p1, pair<Target, int> &p2)
    {
        if(p1.second > p2.second)
            return true;
        else
            return false;
    }

    inline bool SortPairByIntValueSmaller(pair<int, int> &p1, pair<int, int> &p2)
    {
        return SortPairByIntValueSmallerT(p1, p2);
    }

    /// <summary> Compare vector </summary>
    ///
    /// <typeparam name="Target"> Other type. </typeparam>
    /// <param name="v1"> The first vector. </param>
    /// <param name="v2"> The second vector. </param>
    ///
    /// <returns> true for same vector (unordered). </returns>
    template <typename Target>
    bool HasSameElements(const std::vector<Target>& v1, const std::vector<Target>& v2) 
    {
        return std::is_permutation(v1.begin(), v1.end(), v2.begin());
    }

    /// <summary> Remove element from vector </summary>
    ///
    /// <typeparam name="Target"> Other type. </typeparam>
    /// <param name="v"> The vector. </param>
    /// <param name="e"> The element. </param>
    template <typename Target>
    void RemoveElementFromVector(std::vector<Target>& v, const Target& e) 
    {
        v.erase(std::remove(v.begin(), v.end(), e), v.end());
    }

    /// <summary> Remove repeated element from vector </summary>
    /// <summary> Attention! Before removing, vector should be sorted </summary>
    ///
    /// <typeparam name="Target"> Other type. </typeparam>
    /// <param name="v"> The vector. </param>
    template <typename Target>
    void RemoveRepeatedElementFromVector(std::vector<Target>& v) 
    {
        v.erase(std::unique(v.begin(), v.end()), v.end());
    }

    /// <summary> Add vector to vector </summary>
    ///
    /// <typeparam name="Target"> Other type. </typeparam>
    /// <param name="v1"> The first vector. </param>
    /// <param name="v2"> The second vector. </param>
    template <typename Target>
    void AddVectorToVector(std::vector<Target>& v1, const std::vector<Target>& v2) 
    {
        v1.insert(v1.end(), v2.begin(), v2.end());
    }

    template <typename Target>
    std::vector<Target> MergeVectors(std::vector<Target> v1, const std::vector<Target> v2)
    {
        std::vector<Target> v3;
        v3.insert(v3.end(), v1.begin(), v1.end());
        v3.insert(v3.end(), v2.begin(), v2.end());
        return v3;
    }

    /// <summary> Find element from vector </summary>
    ///
    /// <typeparam name="Target"> Other type. </typeparam>
    /// <param name="v"> The vector. </param>
    /// <param name="e"> The element. </param>
    template <typename Target>
    typename vector<Target>::iterator FindElementIterFromVector(std::vector<Target>& v, const Target& e) 
    {
        return std::find(v.begin(), v.end(), e);
    }

    template <typename Target>
    typename bool ContainVectorElement(std::vector<Target>& v, const Target& e) 
    {
        return FindElementIterFromVector(v, e) != v.end();
    }

    template <typename TargetK, typename TargetV>
    vector<TargetK> GetMapKeys(std::map<TargetK, TargetV> v)
    { 
        vector<TargetK> keys;
        std::transform(
            v.begin(),
            v.end(),
            std::back_inserter(keys),
            [](const std::map<TargetK,TargetV>::value_type &pair){return pair.first;});
        return keys;
    }

    /// <summary>
    /// Transform frame coordinate into normalized coordinate (4096x4096).
    /// Frame coordinate is always in Landscape.
    /// </summary>
    /// <param name="frameSize">Size of the frame</param>
    /// <param name="frameCoord">Point in frame coordinate</param>
    /// <param name="orientation">Orientation of normalized coordinate</param>
    /// <returns>Normalized coordinate</returns>
    Point2d FrameToNormCoord(Size frameSize, Point2d frameCoord, Orientation orientation);

    /// <summary>
    /// Transform the normalized coordinate (4096x4096) into frame coordinate.
    /// Frame coordinate is always Landscape.
    /// </summary>
    /// <param name="frameSize">Size of the frame.</param>
    /// <param name="norm">Normalized point</param>
    /// <param name="orientation">Orientation of normalized coordinate</param>
    /// <returns>Point in frame coordinate</returns>
    Point2d NormToFrameCoord(Size frameSize, Point2d norm, Orientation orientation);

    /// <summary>
    /// Transform offset in normalized coordinate (4096x4096) into frame coordinate.
    /// Frame coordinate is always in Landscape.
    /// </summary>
    /// <param name="frameSize">Size of the frame.</param>
    /// <param name="normOffset">Offset in normalized coordinate</param>
    /// <param name="orientation">Orientation of normalized coordinate</param>
    /// <returns>Offset in frame coordinate</returns>
    Size NormToFrameOffset(Size frameSize, Size normOffset, Orientation orientation);

    /// <summary>
    /// When clicking with device coordinates in vertical mode directly, normalize the coordinates and then send clicking event
    /// The layout is decided from the device if rooted, otherwise from the horizontal flag set in the script.
    /// </summary>
    /// <param name="device"></param>
    /// <param name="frame"></param>
    /// <param name="x"></param>
    /// <param name="y"></param>
    /// <param name="o"></param>
    DaVinciStatus ClickOnFrame(boost::shared_ptr<TargetDevice> device, Size frameSize, int x, int y, Orientation o, bool tapmode = false);

    /// <summary>
    /// Touch down on the device via the image frame it presents.
    /// </summary>
    /// <param name="frame"></param>
    /// <param name="x"></param>
    /// <param name="y"></param>
    /// <param name="orientation"></param>
    DaVinciStatus TouchDownOnFrame(boost::shared_ptr<TargetDevice> device, Size frameSize, int x, int y, Orientation orientation);

    /// <summary>
    /// Touch up on the device via the image frame it presents.
    /// </summary>
    /// <param name="frame"></param>
    /// <param name="x"></param>
    /// <param name="y"></param>
    /// <param name="orientation"></param>
    DaVinciStatus TouchUpOnFrame(boost::shared_ptr<TargetDevice> device, Size frameSize, int x, int y, Orientation orientation);

    /// <summary>
    /// Touch up on the device
    /// </summary>
    /// <param name="deviceSize"></param>
    /// <param name="x"></param>
    /// <param name="y"></param>
    /// <param name="orientation"></param>
    /// <param name="action"></param>
    DaVinciStatus TouchOnDevice(boost::shared_ptr<TargetDevice> device, Size deviceSize, int x, int y, Orientation orientation, string action);

    /// <summary>
    /// Touch move on the device via the image frame it presents.
    /// </summary>
    /// <param name="frame"></param>
    /// <param name="x"></param>
    /// <param name="y"></param>
    /// <param name="orientation"></param>
    DaVinciStatus TouchMoveOnFrame(boost::shared_ptr<TargetDevice> device, Size frameSize, int x, int y, Orientation orientation);

    Point TransformRotatedFramePointToFramePoint(Size rotatedFrameSize, Point rotatedPoint, Orientation orientation, Size& frameSize);

    Point TransformFramePointToRotatedFramePoint(Size rotatedFrameSize, Point point, Orientation orientation);


    void ClickOnRotatedFrame(const Size &rotatedFrameSize, const Point &rotatedPoint, bool tapMode = false);

    /// <summary>
    /// Transfrom string to wstring.
    /// </summary>
    /// <param name="str">string</param>
    /// <returns>wstring</returns>
    wstring StrToWstr(const char* str);

    /// <summary>
    /// Transfrom wstring to string.
    /// </summary>
    /// <param name="wstr">wstring</param>
    /// <returns>string</returns>
    string WstrToStr(wstring wstr);

    std::wstring AnsiToWChar(LPCSTR pszSrc, int len);

    std::string WCharToAnsi(LPCWSTR pwszSrc);

    std::wstring StrToWstrAnsi(const std::string &str);

    string WstrToStrAnsi(wstring& inputws);

    /// <summary>
    /// Check whether two double numbers are equal
    /// </summary>
    /// <param name="a">first double number</param>
    /// <param name="b">second double number</param>
    /// <param name="absoluteError">absolute error</param>
    /// <param name="relativeError">relative error</param>
    /// <returns>true for yes</returns>
    bool AreEqual(double a, double b, double absoluteError, double relativeError);

    /// <summary>
    /// Determines whether [is chinese apk] [the specified apk name].
    /// </summary>
    /// <param name="apkName">Name of the apk.</param>
    /// <returns></returns>
    bool ContainNoneASCIIStr(wstring str);

    bool ContainSpecialChar(wstring str);

    /// <summary>
    /// Parse a string as a URI.
    /// </summary>
    /// <param name="sections">The results, they are scheme, user, password, host, path, query, fragment from index 0 to 6.</param>
    /// <param name="uri">The string needs to be parsed</param>
    /// <returns></returns>    
    void ParseURI(vector<string>& sections, const string& uri);

    /// <summary>
    /// Parse a string as a URI query section.
    /// </summary>
    /// <param name="sections">The result map, first element is name, second is value.</param>
    /// <param name="uri">The string needs to be parsed</param>
    /// <returns></returns> 
    void ParseQuery(unordered_map<string, string>& map, const string& query);

    /// <summary>
    /// Return a string contains unix time stamp from epoch, precision is milli sencond. 
    /// <returns></returns>
    string GetCurrentTimeString();

    /// <summary>
    /// Concatenate the two paths into one path
    /// </summary>
    /// <param name="path1">the first path</param>
    /// <param name="path2">the second path</param>
    /// <returns>the path concatenated</returns>
    string ConcatPath(string, string);

    /// <summary>
    /// Escape the space character in filesytem path
    /// </summary>
    /// <param name="srcPath">the path needs to be escaped</param>
    /// <returns>the path escaped</returns>    
    string EscapePath(const string& srcPath);

    /// <summary>
    /// Add double quotations both at beginning and ending of the path string
    /// </summary>
    /// <param name="processPathStr">the path needs to be processed</param>
    /// <returns>the path escaped</returns> 
    string ProcessPathSpaces(const string &processPathStr);

    /// <summary>
    /// Calculate the pixel length of other side according to device resolution
    /// </summary>
    /// <param name="longerSidePxLength">the pixel length of the device longer side</param>
    /// <param name="deviceWidth">the pixel length of device width</param>
    /// <param name="deviceHeight">the pixel length of the device height</param>
    /// <returns>the path escaped</returns>
    int CalPxLengthOfSide(int longerSidePxLength, int deviceWidth, int deviceHeight);
}

#endif