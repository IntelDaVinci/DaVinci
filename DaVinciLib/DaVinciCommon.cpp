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

#include "boost/thread/mutex.hpp"
#include "boost/thread/condition_variable.hpp"
#include "boost/core/null_deleter.hpp"

#include "xercesc/util/XMLString.hpp"
#include "xercesc/framework/LocalFileFormatTarget.hpp"

#include "DaVinciCommon.hpp"
#include "DeviceControlCommon.hpp"
#include "TargetDevice.hpp"
#include "DeviceManager.hpp"
#include "opencv2/gpu/gpu.hpp"

#include <chrono>
#include <iomanip>

using boost::asio::ip::tcp;
using boost::asio::ip::address_v4;
using boost::interprocess::basic_ovectorstream;
using namespace boost::process;
using cv::Mat;
using cv::Vec2f;
using namespace boost::filesystem;

namespace DaVinci
{
    const cv::Scalar Green = cv::Scalar(0, 255, 0);
    const cv::Scalar Red = cv::Scalar(0, 0, 255);
    const cv::Scalar Yellow = cv::Scalar(0, 255, 255);
    const cv::Scalar Blue = cv::Scalar(255, 0, 0);
    const cv::Scalar White = cv::Scalar(255, 255, 255);
    const cv::Scalar Black = cv::Scalar(0, 0, 0);

    const Rect EmptyRect = Rect(0, 0, 0, 0);
    const Point EmptyPoint = Point(0, 0);
    const Point CenterPoint = Point(640, 360);
    const Mat EmptyMat = Mat::zeros(Size(1280, 720), CV_8UC3);
    const int maxWidthHeight = 4096;

    const string Characters[62] = {"a", "A", "b", "B", "c", "C", "D", "d", "e", "E", "f", "F", "g", "G",
        "h", "H", "i", "I", "j", "J", "k", "K", "l", "L" "m", "M", "n", "N", "o", "O", "p", "P", 
        "q", "Q", "r", "R", "s", "S", "t", "T", "u", "U", "v", "V",  "w", "W", "x", "X", "y", "Y",
        "z", "Z", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};

    LineSegment::LineSegment()
    {
    }

    LineSegment::LineSegment(Point p1, Point p2)
    {
        this->p1 = p1;
        this->p2 = p2;
    }

    LineSegmentD::LineSegmentD(Point2d p1, Point2d p2)
    {
        this->p1 = p1;
        this->p2 = p2;
    }

    AutoResetEvent::AutoResetEvent(bool initialSignalState) : signaled(initialSignalState)
    {
    }

    void AutoResetEvent::Reset()
    {
        boost::lock_guard<boost::mutex> lock(event_mutex);
        signaled = false;
    }

    void AutoResetEvent::Set()
    {
        boost::lock_guard<boost::mutex> lock(event_mutex);
        signaled = true;
        event_cond.notify_one();
    }

    bool AutoResetEvent::WaitOne(int timeout)
    {
        boost::unique_lock<boost::mutex> lock(event_mutex);
        if (timeout >= 0)
        {
            auto timeoutAbs = boost::chrono::steady_clock::now() + boost::chrono::milliseconds(timeout);
            while (!signaled)
            {
                if (event_cond.wait_until(lock, timeoutAbs) == boost::cv_status::timeout)
                {
                    break;
                }
            }
        }
        else
        {
            while (!signaled)
            {
                event_cond.wait(lock);
            }
        }
        signaled = false;
        return true;
    }

    GlobalNamedMutex::GlobalNamedMutex(const string & mutexName)
    {
        ghMutex = CreateMutex( 
                  NULL,
                  FALSE,
                  mutexName.c_str());

        if (ghMutex != NULL) 
        {
           if (WaitForSingleObject(ghMutex, INFINITE) != WAIT_OBJECT_0)
               DAVINCI_LOG_ERROR << "GlobalNamedMutex WaitForSingleObject failed, Error Code: " << GetLastError();
        }
        else
            DAVINCI_LOG_ERROR << "GlobalNamedMutex Creation Failure, Name: " << mutexName;
    }

    GlobalNamedMutex::~GlobalNamedMutex()
    {
        if (ghMutex != NULL) 
        {
            ReleaseMutex(ghMutex);
            CloseHandle(ghMutex);
        }
    }

    /// <summary> Splits command line arguments into a vector of args. </summary>
    ///
    /// <param name="args">    The argument string. </param>
    /// <param name="argList"> [in,out] List of arguments. </param>
    void SplitCommandLineArgs(const string &args, vector<string> &argList)
    {
        int beginPos = 0, endPos = 0;
        bool inQuote = false;
        while (endPos < static_cast<int>(args.size()))
        {
            if (args[endPos] == '\"')
            {
                inQuote = !inQuote;
                if (inQuote)
                {
                    beginPos++;
                }
                else
                {
                    argList.push_back(args.substr(beginPos, endPos - beginPos));
                    beginPos = endPos + 1;
                }
            }
            else if (args[endPos] == ' ' && !inQuote)
            {
                if (beginPos < endPos)
                {
                    argList.push_back(args.substr(beginPos, endPos - beginPos));
                }
                beginPos = endPos + 1;
            }
            endPos++;
        }
        if (beginPos < endPos)
        {
            argList.push_back(args.substr(beginPos, endPos - beginPos));
        }
    }

    void RunProcessWatcher(const child &watchingProcess, int timeout, boost::atomic<bool> &isProcessExited, boost::atomic<bool> &isTimeout, boost::condition_variable &processExited)
    {
        boost::mutex processExitMutex;
        boost::unique_lock<boost::mutex> lock(processExitMutex);
        auto timeoutAbs = boost::chrono::steady_clock::now() + boost::chrono::milliseconds(timeout);
        while (!isProcessExited)
        {
            if (processExited.wait_until(lock, timeoutAbs) == boost::cv_status::timeout)
            {
                try
                {
                    isTimeout = true;
                    watchingProcess.terminate(true);
                }
                catch (...)
                {
                    // swallow the exception
                }
                return;
            }
        }
    }

    DaVinciStatus RunProcessSync(const string &executable, const string &args, const boost::shared_ptr<ostringstream> &outStr, const string &workingDir, int timeout)
    {
        boost::shared_ptr<basic_ovectorstream<vector<char>>> outStream;
        if (outStr != nullptr)
        {
            outStream = boost::shared_ptr<basic_ovectorstream<vector<char>>>(new basic_ovectorstream<vector<char>>());
        }
        DaVinciStatus status = RunProcessSyncRaw(executable, args, outStream, workingDir, timeout);
        if (DaVinciSuccess(status) && outStr != nullptr && outStream != nullptr && outStream->vector().size() > 0)
        {
            const vector<char> &v = outStream->vector();
            outStr->write(&v[0], v.size());
        }
        return status;
    }

    DaVinciStatus RunProcessSyncRaw(const string &executable, const string &args, const boost::shared_ptr<basic_ovectorstream<vector<char>>> &outStream, const string &workingDir, int timeout)
    {
        DAVINCI_LOG_DEBUG << "Run Process (Sync): \"" << executable << args << "\"";
        vector<string> argList;

        if (executable.empty())
            return errc::invalid_argument;

        argList.push_back(executable);
        SplitCommandLineArgs(args, argList);
        context ctxout;
        if (outStream != nullptr)
        {
            ctxout.stderr_behavior = redirect_stream_to_stdout();
            ctxout.stdout_behavior = capture_stream();
        }
        ctxout.work_directory = workingDir;
        try
        {
            child c = launch(executable, argList, ctxout);
            boost::condition_variable processExited;
            boost::shared_ptr<boost::thread> watchdog;
            boost::atomic<bool> isTimeout(false);
            boost::atomic<bool> isProcessExited(false);
            if (timeout > 0)
            {
                watchdog = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(RunProcessWatcher, boost::ref(c), timeout, boost::ref(isProcessExited), boost::ref(isTimeout), boost::ref(processExited))));
            }
            if (outStream != nullptr)
            {
                // jgong5: pistream does not support buffered read on Windows, so have to read in bytes.
                // I didn't tested it on Posix systems though.
                pistream& is = c.get_stdout();
                if (is)
                {
                    while (!isTimeout && is.rdbuf()->sgetc() != EOF)
                    {
                        *outStream << static_cast<char>(is.rdbuf()->sbumpc());
                    }
                }
            }
            c.wait();
            isProcessExited = true;
            processExited.notify_one();
            if (watchdog != nullptr)
            {
                watchdog->join();
            }
            if (!isTimeout)
            {
                return DaVinciStatusSuccess;
            }
            else
            {
                return DaVinciStatus(errc::timed_out);
            }
        }
        catch (...)
        {
            // TODO: provide more accurate error code per exception type
            return DaVinciStatus(errc::operation_not_permitted);
        }
    }

    DaVinciStatus RunShellCommand(const string &shellCommand, const boost::shared_ptr<ostringstream> &outStr,
        const string &workingDir, int timeout, boost::process::environment env)
    {
        if (shellCommand.empty())
            return DaVinciStatus(errc::invalid_argument);

        DAVINCI_LOG_DEBUG << "Run Process (Sync): \"" << shellCommand << "\"";

        context ctx;
        ctx.environment = env;
        if (!workingDir.empty())
            ctx.work_directory = workingDir;
        if (outStr != nullptr)
        {
            ctx.stderr_behavior = redirect_stream_to_stdout();
            ctx.stdout_behavior = capture_stream();
        }       

        auto child = launch_shell(shellCommand, ctx);

        boost::condition_variable processExited;
        boost::shared_ptr<boost::thread> watchdog;
        boost::atomic<bool> isTimeout(false);
        boost::atomic<bool> isProcessExited(false);
        if (timeout > 0)
        {
            watchdog = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(RunProcessWatcher, boost::ref(child), timeout,
                boost::ref(isProcessExited), boost::ref(isTimeout), boost::ref(processExited))));
        }

        if (outStr != nullptr)
        {
            auto& is = child.get_stdout();
            *outStr << is.rdbuf();
            DAVINCI_LOG_DEBUG << outStr->str();
        }

        auto s = child.wait();
        isProcessExited = true;
        processExited.notify_one();
        if (watchdog != nullptr)
        {
            watchdog->join();
        }
        if (!isTimeout)
        {
#if defined(BOOST_POSIX_API)
            if (s.exited())
            {
                DAVINCI_LOG_DEBUG << s.exit_status();
                return DaVinciStatusSuccess;
            }
            else
                return DaVinciStatus(errc::no_child_process);
#elif defined(BOOST_WINDOWS_API)
            DAVINCI_LOG_DEBUG << s.exit_status();
            return DaVinciStatusSuccess;
#endif
        }
        else
        {
            return DaVinciStatus(errc::timed_out);
        }
    }

    DaVinciStatus RunProcessAsync(const string &executable, const string &args, boost::shared_ptr<boost::process::process> &p, const string &workingDir)
    {
        DAVINCI_LOG_DEBUG << "Run Process (Async): \"" << executable << args;
        vector<string> argList;
        argList.push_back(executable);
        SplitCommandLineArgs(args, argList);
        context ctxout;
        ctxout.work_directory = workingDir;
        try
        {
            p = boost::shared_ptr<boost::process::process>(new boost::process::process(launch(executable, argList, ctxout)));
            return DaVinciStatusSuccess;
        }
        catch (...)
        {
            // TODO: provide more accurate error code per exception type
            return DaVinciStatus(errc::operation_not_permitted);
        }
    }

    unsigned short GetAvailableLocalPort(unsigned short searchBase)
    {
        boost::asio::io_service io_service;
        unsigned short port = searchBase;
        while (port != 0)
        {
            try
            {
                tcp::acceptor acceptor(io_service);
                tcp::endpoint endpoint(address_v4::loopback(), port);
                acceptor.open(tcp::v4());
                acceptor.bind(endpoint);
                return port;
            }
            catch (...)
            {
                port++;
            }
        }
        return 0;
    }

    DaVinciStatus ReadAllLines(const string& fileName, vector<std::string>& lines)
    {
        ifstream ifile;
        string line;
        ifile.open(fileName);
        if (ifile.fail())
        {
            DAVINCI_LOG_WARNING << (std::string("Warning: can't open file - ") + fileName);
            return errc::invalid_argument;
        }
        while (getline(ifile,line))
        {
            if (!line.empty() && line.at(line.length() - 1) == '\r')
            {
                line.erase(line.length() - 1);
            }
            lines.push_back(line);
        }
        ifile.close();
        return DaVinciStatusSuccess;
    }

    DaVinciStatus ReadAllLinesFromStr(const string &str, vector<string> &lines)
    {
        istringstream iss(str);
        string line;
        while (getline(iss, line))
        {
            if (!line.empty() && line.at(line.length() - 1) == '\r')
            {
                line.erase(line.length() - 1);
            }
            lines.push_back(line);
        }
        return DaVinciStatusSuccess;
    }

    void ProjectPoints(vector<Point2f>& points,const Mat& H )
    {
        Mat ptMat = Mat((int)points.size(),1,CV_32FC2);

        int pSize = (int)points.size();
        for(int i=0;i<pSize;++i)
        {
            ptMat.at<Vec2f>(i,0)[0]=points[i].x;
            ptMat.at<Vec2f>(i,0)[1]=points[i].y;
        }
        perspectiveTransform(ptMat,ptMat,H);
        for(int i=0;i<pSize;++i)
        {
            points[i].x= ptMat.at<Vec2f>(i,0)[0];
            points[i].y=ptMat.at<Vec2f>(i,0)[1];
        }
    }

    Mat LogSharpen(const Mat& image)
    {
        float k[3][3] = {{0.0, -1.0, 0.0}, {-1.0, 5.0f, -1.0}, {0.0, -1.0f, 0.0}};
        Mat kernel = Mat(3, 3, CV_32F, k);
        Mat sharpenImage =Mat(image.rows, image.cols,CV_8UC3);
        filter2D(image, sharpenImage,image.depth(), kernel);
        return sharpenImage;          
    }

    void VoteForUniqueness(const vector<vector<cv::DMatch>>& matchers, double uniquenessThreshold, Mat& mask)
    {
        assert(mask.rows==1||mask.cols==1);
        bool isRow = false;
        if(mask.rows==1)
        {
            isRow = true;
        }

#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
        int matchSize = (int)matchers.size();
        for(int i=0;i<matchSize;++i)
        {
            if(matchers[i][0].distance/matchers[i][1].distance>uniquenessThreshold)
            {
                if(isRow)
                {
                    mask.at<uchar>(0,i)=0;
                }
                else
                {
                    mask.at<uchar>(i,0)=0;
                }
            }
        }
    }

    void XmlStringDeleter(char *p)
    {
        XMLString::release(&p);
    }

    string XMLStrToStr(const XMLCh *xmlStr)
    {
        if (xmlStr == nullptr)
        {
            return string("");
        }
        else
        {
            return string((boost::shared_ptr<char>(XMLString::transcode(xmlStr), &XmlStringDeleter)).get());
        }
    }

    boost::shared_ptr<XMLCh> StrToXMLStr(const string str)
    {
        return boost::shared_ptr<XMLCh>(XMLString::transcode(str.c_str()));
    }

    DaVinciStatus WriteDOMDocumentToXML(const boost::shared_ptr<DOMImplementation> domImp, const boost::shared_ptr<DOMDocument> domDocument, const string xmlFileName)
    {
        if (domImp == nullptr || domDocument == nullptr || xmlFileName.empty())
        {
            return errc::invalid_argument;
        }

        boost::shared_ptr<DOMLSOutput> output = boost::shared_ptr<DOMLSOutput>((boost::shared_ptr<DOMImplementationLS>(domImp))->createLSOutput(), &XmlNonDocumentPtrDeleter<DOMLSOutput>);
        boost::shared_ptr<DOMLSSerializer> serial = boost::shared_ptr<DOMLSSerializer>((boost::shared_ptr<DOMImplementationLS>(domImp))->createLSSerializer(), &XmlNonDocumentPtrDeleter<DOMLSSerializer>);
        boost::shared_ptr<DOMConfiguration> domConfiguration = boost::shared_ptr<DOMConfiguration>(serial->getDomConfig(), boost::null_deleter());

        if (domConfiguration->canSetParameter(StrToXMLStr("format-pretty-print").get(), true))
        {
            domConfiguration->setParameter(StrToXMLStr("format-pretty-print").get(), true);
        }

        boost::shared_ptr<XMLFormatTarget> target = boost::shared_ptr<XMLFormatTarget>(new LocalFileFormatTarget(xmlFileName.c_str()));
        output->setEncoding(StrToXMLStr("UTF-8").get());
        output->setByteStream(target.get());
        if (serial->write(domDocument.get(), output.get()))
        {
            return DaVinciStatusSuccess;
        }
        else
        {
            return errc::io_error;
        }
    }

    std::vector<boost::shared_ptr<DOMNode>> GetNodesFromXPath(const boost::shared_ptr<DOMDocument> domDoc, const string xpath)
    {
        boost::shared_ptr<DOMElement> root = boost::shared_ptr<DOMElement>(domDoc->getDocumentElement(), &XmlDocumentPtrDeleter<DOMElement>);

        boost::shared_ptr<DOMXPathResult> result = boost::shared_ptr<DOMXPathResult>(
            domDoc->evaluate(
            XMLString::transcode(xpath.c_str()),
            root.get(),
            NULL,
            DOMXPathResult::ORDERED_NODE_SNAPSHOT_TYPE,
            NULL), &XmlNonDocumentPtrDeleter<DOMXPathResult>);

        size_t nodeLength = result->getSnapshotLength();
        vector<boost::shared_ptr<DOMNode>> nodeList =  vector<boost::shared_ptr<DOMNode>>();
        for (size_t i = 0; i < nodeLength; ++i )
        {
            result->snapshotItem(i);
            boost::shared_ptr<DOMNode> node = boost::shared_ptr<DOMNode>(result->getNodeValue(), &XmlDocumentPtrDeleter<DOMNode>);
            nodeList.push_back(node);
        }

        return nodeList;
    }     

    std::vector<boost::shared_ptr<DOMNode>> GetNodesFromXPath(const boost::shared_ptr<DOMDocument> domDoc, const string xpath, const XMLCh *attributeName, const std::string attributeValue)
    {
        boost::shared_ptr<DOMElement> root = boost::shared_ptr<DOMElement>(domDoc->getDocumentElement(), &XmlDocumentPtrDeleter<DOMElement>);

        boost::shared_ptr<DOMXPathResult> result = boost::shared_ptr<DOMXPathResult>(
            domDoc->evaluate(
            StrToXMLStr(xpath).get(),
            root.get(),
            NULL,
            DOMXPathResult::ORDERED_NODE_SNAPSHOT_TYPE,
            NULL), &XmlNonDocumentPtrDeleter<DOMXPathResult>);

        size_t nodeLength = result->getSnapshotLength();
        vector<boost::shared_ptr<DOMNode>> nodeList =  vector<boost::shared_ptr<DOMNode>>();
        for (size_t i = 0; i < nodeLength; ++i )
        {
            result->snapshotItem(i);
            boost::shared_ptr<DOMNode> node = boost::shared_ptr<DOMNode>(result->getNodeValue(), &XmlDocumentPtrDeleter<DOMNode>);
            boost::shared_ptr<DOMNode> attributes = boost::shared_ptr<DOMNode>(node->getAttributes()->getNamedItem(attributeName),  &XmlDocumentPtrDeleter<DOMNode>);
            if(attributes == nullptr)
            {
                DAVINCI_LOG_WARNING << (std::string("No DOMNode found, please check the path ends with the node for attribute."));
                continue;
            }
            if(boost::equals(XMLStrToStr(attributes->getNodeValue()), attributeValue))
            {
                nodeList.push_back(node);
            }
        }

        return nodeList;
    }


    boost::shared_ptr<DOMElement> CreateDOMElementNullDeleter(const boost::shared_ptr<DOMDocument> &doc, const string &elementName)
    {
        return boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr(elementName).get()), boost::null_deleter());
    }

    LineSegment Vec4iToLineSegment(const Vec4i &line)
    {
        return LineSegment(Point(line[0], line[1]), Point(line[2], line[3]));
    }

    vector<LineSegment> Vec4iLinesToLineSegments(const vector<Vec4i> &lines)
    {
        vector<LineSegment> lineSegmentList;
        LineSegment lineSegment;
        for(auto line : lines)
        {
            lineSegment = Vec4iToLineSegment(line);
            lineSegmentList.push_back(lineSegment);
        }
        return lineSegmentList;
    }

    vector<LineSegment> GetHorizontalLineSegments(const vector<LineSegment> &lines)
    {
        vector<LineSegment> hLineSegmentList;
        for(auto line : lines)
        {
            if(line.p1.y == line.p2.y)
                hLineSegmentList.push_back(line);
        }

        return hLineSegmentList;
    }

    bool SortLineSegmentXGreater (const LineSegment &i, const LineSegment &j)
    {
        if(i.p1.x < j.p1.x)
            return true;
        else
            return false;
    }

    bool SortLineSegmentYGreater (const LineSegment &i, const LineSegment &j)
    {
        if(i.p1.y < j.p1.y)
            return true;
        else
            return false;
    }

    bool SortLineSegmentXSmaller (const LineSegment &i, const LineSegment &j)
    {
        if(i.p1.x > j.p1.x)
            return true;
        else
            return false;
    }

    bool SortLineSegmentYSmaller (const LineSegment &i, const LineSegment &j)
    {
        if(i.p1.y > j.p1.y)
            return true;
        else
            return false;
    }

    bool SortRectXGreater (const Rect &i, const Rect &j)
    {
        if(i.x < j.x)
            return true;
        else
            return false;
    }

    bool SortRectYGreater (const Rect &i, const Rect &j)
    {
        if(i.y < j.y)
            return true;
        else
            return false;
    }

    bool SortRectHGreater (const Rect &i, const Rect &j)
    {
        if(i.height < j.height)
            return true;
        else
            return false;
    }

    bool SortRectAreaGreater(const Rect &i, const Rect &j)
    {
        if(i.area() < j.area())
            return true;
        else
            return false;
    }

    bool SortRectCCGreater (const Rect &i, const Rect &j)
    {
        int centerX1 = i.x + i.width / 2;
        int centerY1 = i.y + i.height / 2;
        int centerX2 = j.x + j.width / 2;
        int centerY2 = j.y + j.height / 2;

        if (centerX1 < centerX2)
        {
            return true;
        }
        else if (centerX1 > centerX2)
        {
            return false;
        }
        else if (centerY1 < centerY2)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool SortRectXSmaller (const Rect &i, const Rect &j)
    {
        if(i.x > j.x)
            return true;
        else
            return false;
    }

    bool SortRectYSmaller (const Rect &i, const Rect &j)
    {
        if(i.y > j.y)
            return true;
        else
            return false;
    }

    bool SortRectHSmaller (const Rect &i, const Rect &j)
    {
        if(i.height > j.height)
            return true;
        else
            return false;
    }

    bool SortRectAreaSmaller(const Rect &i, const Rect &j)
    {
        if(i.area() > j.area())
            return true;
        else
            return false;
    }

    bool SortRectCCSmaller (const Rect &i, const Rect &j)
    {
        int centerX1 = 2 * i.x + i.width;
        int centerY1 = 2 * i.y + i.height;
        int centerX2 = 2 * j.x + j.width;
        int centerY2 = 2* j.y + j.height;

        if (centerX1 > centerX2)
        {
            return true;
        }
        else if (centerX1 < centerX2)
        {
            return false;
        }
        else if (centerY1 > centerY2)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    DaVinciStatus SortLineSegments(vector<LineSegment> &lineVector, enum UType t, bool isGreater)
    {
        if(t == UTYPE_X)
        {
            if(isGreater)
                std::sort (lineVector.begin(), lineVector.end(), SortLineSegmentXGreater); 
            else
                std::sort (lineVector.begin(), lineVector.end(), SortLineSegmentXSmaller); 
        }
        else if(t == UTYPE_Y)
        {
            if(isGreater)
                std::sort (lineVector.begin(), lineVector.end(), SortLineSegmentYGreater); 
            else
                std::sort (lineVector.begin(), lineVector.end(), SortLineSegmentYSmaller); 
        }
        else
        {
            DAVINCI_LOG_ERROR << "Unsupported sort type: " << (int)t;
            return errc::function_not_supported;
        }

        return DaVinciStatusSuccess;
    }

    DaVinciStatus SortRects(vector<Rect> &rectVector, enum UType t, bool isGreater)
    {
        if(t == UTYPE_X)
        {
            if(isGreater)
                std::sort (rectVector.begin(), rectVector.end(), SortRectXGreater); 
            else
                std::sort (rectVector.begin(), rectVector.end(), SortRectXSmaller); 
        }
        else if(t == UTYPE_Y)
        {
            if(isGreater)
                std::sort (rectVector.begin(), rectVector.end(), SortRectYGreater); 
            else
                std::sort (rectVector.begin(), rectVector.end(), SortRectYSmaller); 
        }
        else if (t == UTYPE_H)
        {
            if (isGreater)
            {
                std::sort (rectVector.begin(), rectVector.end(), SortRectHGreater);
            }
            else
            {
                std::sort (rectVector.begin(), rectVector.end(), SortRectHSmaller); 
            }
        }
        else if (t == UTYPE_CC)
        {
            if (isGreater)
            {
                std::sort (rectVector.begin(), rectVector.end(), SortRectCCGreater);
            }
            else
            {
                std::sort (rectVector.begin(), rectVector.end(), SortRectCCSmaller);
            }
        }
        else if (t == UTYPE_AREA)
        {
            if (isGreater)
            {
                std::sort (rectVector.begin(), rectVector.end(), SortRectAreaGreater);
            }
            else
            {
                std::sort (rectVector.begin(), rectVector.end(), SortRectAreaSmaller);
            }
        }
        else
        {
            DAVINCI_LOG_ERROR << "Unsupported sort type: " << (int)t;
            return errc::operation_not_supported;
        }

        return DaVinciStatusSuccess;
    }

    bool IntersectWith(const Rect &rect1, const Rect &rect2)
    {
        bool isIntersectant = false;

        if ((rect1 & rect2) != EmptyRect)
        {
            isIntersectant = true;
        }

        return isIntersectant;
    }

    bool ContainRect(const Rect &rect1, const Rect &rect2, int edge)
    {
        return (rect1.x <= rect2.x
            && rect1.x + rect1.width + edge >= rect2.x + rect2.width 
            && rect1.y <= rect2.y
            && rect1.y + rect1.height + edge >= rect2.y + rect2.height);
    }

    Point2d FrameToNormCoord(Size frameSize, Point2d frameCoord, Orientation orientation)
    {
        Point2d result;
        double normX = maxWidthHeight * frameCoord.x / frameSize.width;
        double normY = maxWidthHeight * frameCoord.y / frameSize.height;
        if (IsHorizontalOrientation(orientation))
        {
            result.x = normX;
            result.y = normY;
        }
        else
        {
            result.x = maxWidthHeight - normY;
            result.y = normX;
        }
        if (IsReverseOrientation(orientation))
        {
            result.x = maxWidthHeight - result.x;
            result.y = maxWidthHeight - result.y;
        }
        return result;
    }

    Point2d NormToFrameCoord(Size frameSize, Point2d norm, Orientation orientation)
    {
        Point2d result;
        double ratioX = static_cast<double>(frameSize.width) / maxWidthHeight;
        double ratioY = static_cast<double>(frameSize.height) / maxWidthHeight;
        if (IsHorizontalOrientation(orientation))
        {
            result.x = norm.x * ratioX;
            result.y = norm.y * ratioY;
        }
        else
        {
            result.x = norm.y * ratioX;
            result.y = frameSize.height - norm.x * ratioY;
        }
        if (IsReverseOrientation(orientation))
        {
            result.x = frameSize.width - result.x;
            result.y = frameSize.height - result.y;
        }
        return result;
    }

    Size NormToFrameOffset(Size frameSize, Size normOffset, Orientation orientation)
    {
        Size result;
        if (IsHorizontalOrientation(orientation))
        {
            result.width = normOffset.width * frameSize.width / maxWidthHeight;
            result.height = normOffset.height * frameSize.height / maxWidthHeight;
        }
        else
        {
            result.width = normOffset.height * frameSize.width / maxWidthHeight;
            result.height = -normOffset.width * frameSize.height / maxWidthHeight;
        }
        if (IsReverseOrientation(orientation))
        {
            result.width = -result.width;
            result.height = -result.height;
        }
        return result;
    }

    DaVinciStatus ClickOnFrame(boost::shared_ptr<TargetDevice> device, Size frameSize, int x, int y, Orientation o, bool tapmode)
    {
        if (device == nullptr)
        {
            return errc::invalid_argument;
        }
        Point norm = FrameToNormCoord(frameSize, Point(x, y), o);
        DAVINCI_LOG_DEBUG << "Send Click to MAgent @(" << norm.x << ","<< norm.y << ")";
        return device->Click(norm.x, norm.y, -1, o, tapmode);
    }

    DaVinciStatus TouchDownOnFrame(boost::shared_ptr<TargetDevice> device, Size frameSize, int x, int y, Orientation orientation)
    {
        if (device == nullptr)
        {
            return errc::invalid_argument;
        }
        Point norm = FrameToNormCoord(frameSize, Point(x, y), orientation);
        DAVINCI_LOG_DEBUG << "Send TouchDown to MAgent @(" << norm.x << norm.y << ")";
        return device->TouchDown(norm.x, norm.y, -1, orientation);
    }

    Point TransformDevicePoint(Point sPoint, Orientation orientation, Size source, Size target)
    {
        Point tPoint;
        switch(orientation)
        {
        case Orientation::Portrait:
        case Orientation::ReversePortrait:
            {
                tPoint.x = sPoint.x * target.width / source.width;
                tPoint.y = sPoint.y * target.height / source.height;
                break;
            }
        case Orientation::Landscape:
        case Orientation::ReverseLandscape:
            {
                tPoint.x = sPoint.x * target.height/ source.height;
                tPoint.y = sPoint.y * target.width / source.width;
                break;
            }
        default:
            {
                tPoint.x = sPoint.x * target.width / source.width;
                tPoint.y = sPoint.y * target.height / source.height;
                break;
            }
        }

        return tPoint;
    }

    DaVinciStatus TouchOnDevice(boost::shared_ptr<TargetDevice> device, Size deviceSize, int x, int y, Orientation orientation, string action)
    {
        Size orientationDeviceSize;
        switch(orientation)
        {
        case Orientation::Portrait:
        case Orientation::ReversePortrait:
            {
                orientationDeviceSize = Size(deviceSize.width, deviceSize.height);
                break;
            }
        case Orientation::Landscape:
        case Orientation::ReverseLandscape:
            {
                orientationDeviceSize = Size(deviceSize.height, deviceSize.width);
                break;
            }
        default:
            {
                orientationDeviceSize = Size(deviceSize.width, deviceSize.height);
            }
        }

        Size frameSize;
        Point newPoint = TransformRotatedFramePointToFramePoint(orientationDeviceSize, Point(x, y), orientation, frameSize);

        if(boost::to_lower_copy(action) == "down")
            return TouchDownOnFrame(device, frameSize, newPoint.x, newPoint.y, orientation);
        else if(boost::to_lower_copy(action) == "up")
            return TouchUpOnFrame(device, frameSize, newPoint.x, newPoint.y, orientation);
        else
            return TouchMoveOnFrame(device, frameSize, newPoint.x, newPoint.y, orientation);
    }

    DaVinciStatus TouchUpOnFrame(boost::shared_ptr<TargetDevice> device, Size frameSize, int x, int y, Orientation orientation)
    {
        if (device == nullptr)
        {
            return errc::invalid_argument;
        }
        Point norm = FrameToNormCoord(frameSize, Point(x, y), orientation);
        DAVINCI_LOG_DEBUG << "Send TouchUp to MAgent @(" << norm.x << "," << norm.y << ")";
        return device->TouchUp(norm.x, norm.y, -1, orientation);
    }

    DaVinciStatus TouchMoveOnFrame(boost::shared_ptr<TargetDevice> device, Size frameSize, int x, int y, Orientation orientation)
    {
        if (device == nullptr)
        {
            return errc::invalid_argument;
        }
        Point norm = FrameToNormCoord(frameSize, Point(x, y), orientation);
        DAVINCI_LOG_DEBUG << "Send TouchMove to MAgent @(" << norm.x <<"," << norm.y << ")";
        return device->TouchMove(norm.x, norm.y, -1, orientation);
    }

    inline cv::Mat RotateImageInternal(const cv::Mat &image, int type)
    {
        cv::Mat rotated(image.t());
        if (DeviceManager::Instance().GpuAccelerationEnabled())
        {
            // gpu::transpose only support 1, 4, 8 element size
            // we uses cpu version as element size of image is 3.
            cv::gpu::GpuMat gpuImageTranspose(rotated);
            cv::gpu::GpuMat rotatedGpu;
            // source and destination must not be the same matrix
            cv::gpu::flip(gpuImageTranspose, rotatedGpu, type);
            rotatedGpu.download(rotated);
        }
        else
        {
            cv::flip(rotated, rotated, type);
        }
        return rotated;
    }
    /// <summary> Rotate image 270 degree (reverse-clockwise) </summary>
    ///
    /// <param name="image"> The image. </param>
    ///
    /// <returns> A cv::Mat. </returns>
    inline cv::Mat RotateImage270(const cv::Mat &image)
    {
        return RotateImageInternal(image, 0);
    }

    /// <summary> Rotate image 90 degree (reverse-clockwise) </summary>
    ///
    /// <param name="image"> The image. </param>
    ///
    /// <returns> A cv::Mat. </returns>
    inline cv::Mat RotateImage90(const cv::Mat &image)
    {
        return RotateImageInternal(image, 1);
    }

    Rect RotateROIUp(Rect frameROI, Size frameSize, Orientation orientation)
    {
        Rect rotatedFrameROI = EmptyRect;
        switch (orientation)
        {
        case Orientation::Portrait:
            rotatedFrameROI = Rect(frameSize.height - frameROI.y - frameROI.height, frameROI.x, frameROI.height, frameROI.width);
            break;
        case Orientation::Landscape:
            rotatedFrameROI = frameROI;
            break;
        case Orientation::ReversePortrait:
            rotatedFrameROI = Rect(frameROI.y,  frameSize.width - frameROI.x - frameROI.width, frameROI.height, frameROI.width);
            break;
        case Orientation::ReverseLandscape:
            rotatedFrameROI = Rect(frameSize.width - frameROI.x - frameROI.width, frameSize.height - frameROI.y - frameROI.height, frameROI.width, frameROI.height);
            break;
        default:
            rotatedFrameROI = Rect(frameSize.height - frameROI.y - frameROI.height, frameROI.x, frameROI.height, frameROI.width);
            break;
        }
        return rotatedFrameROI;
    }

    cv::Mat RotateFrameUp(const Mat &frame, Orientation orientation)
    {
        if (frame.empty())
            return frame;

        cv::Mat rotatedFrame;
        switch (orientation)
        {
        case Orientation::Portrait:
            rotatedFrame = RotateImage90(frame);
            break;
        case Orientation::Landscape:
            rotatedFrame = frame;
            break;
        case Orientation::ReversePortrait:
            rotatedFrame = RotateImage270(frame);
            break;
        case Orientation::ReverseLandscape:
            rotatedFrame = RotateImage180(frame);
            break;
        default:
            rotatedFrame = RotateImage90(frame);
            break;
        }
        return rotatedFrame;
    }

    cv::Mat RotateFrameUpForCommandScreenCapture(const Mat &frame, Orientation orientation)
    {
        cv::Mat rotatedFrame;

        if (DeviceManager::Instance().GetDeviceDefaultOrientation() == Orientation::Landscape)
        {
            switch (orientation)
            {
            case Orientation::ReverseLandscape:
                rotatedFrame = RotateImage180(frame);
                break;
            case Orientation::ReversePortrait:
                rotatedFrame = RotateImage270(frame);
                break;
            case Orientation::Portrait:
                rotatedFrame = RotateImage90(frame);
                break;
            case Orientation::Unknown:
            case Orientation::Landscape:
            default:
                rotatedFrame = frame;
                break;
            }
        }
        else
        {
            switch (orientation)
            {
            case Orientation::ReverseLandscape:
                rotatedFrame = RotateImage90(frame);
                break;
            case Orientation::ReversePortrait:
                rotatedFrame = RotateImage180(frame);
                break;
            case Orientation::Landscape:
                rotatedFrame = RotateImage270(frame);
                break;
            case Orientation::Unknown:
            case Orientation::Portrait:
            default:
                rotatedFrame = frame;
                break;
            }
        }

        return rotatedFrame;
    }

    cv::Mat RotateFrameUp(const Mat &frame)
    {
        cv::Mat frameBeforeRotate = frame;
        cv::Mat frameAfterRotate = frame;
        boost::shared_ptr<TargetDevice> device = DeviceManager::Instance().GetCurrentTargetDevice();
        if(DeviceManager::Instance().IsAndroidDevice(device))
        {
            if (!DeviceManager::Instance().UsingHyperSoftCam())
            {
                frameBeforeRotate = device->GetScreenCapture();
            }
            frameAfterRotate = RotateFrameUp(frameBeforeRotate, device->GetCurrentOrientation());
        }
        return frameAfterRotate;
    }

    cv::Mat RotateFrameClockwise(const cv::Mat &frame, int rotateAngle)
    {
        cv::Mat rotatedFrame;

        switch(rotateAngle)
        {
        case 90:
            rotatedFrame = RotateImage90(frame);
            break;
        case 180:
            rotatedFrame = RotateImage180(frame);
            break;
        case 270:
            rotatedFrame = RotateImage270(frame);
            break;
        default:
            rotatedFrame = frame;
            break;
        }

        return rotatedFrame;
    }

    /// <summary>
    /// Transform rotated frame point to frame point
    /// </summary>
    /// <param name="rotatedFrameSize">rotatedFrameSize</param>
    /// <param name="rotatedPoint">rotatedPoint</param>
    /// <param name="orientation"></param>
    /// <param name="frameSize">frameSize</param>
    /// <returns></returns>
    Point TransformRotatedFramePointToFramePoint(Size rotatedFrameSize, Point rotatedPoint, Orientation orientation, Size &frameSize)
    {
        Point newPoint;
        switch (orientation)
        {
        case Orientation::Portrait:
            newPoint = Point(rotatedPoint.y, rotatedFrameSize.width - rotatedPoint.x);
            frameSize = Size(rotatedFrameSize.height, rotatedFrameSize.width);
            break;
        case Orientation::Landscape:
            newPoint = rotatedPoint;
            frameSize = rotatedFrameSize;
            break;
        case Orientation::ReversePortrait:
            newPoint = Point(rotatedFrameSize.height - rotatedPoint.y, rotatedPoint.x);
            frameSize = Size(rotatedFrameSize.height, rotatedFrameSize.width);
            break;
        case Orientation::ReverseLandscape:
            newPoint = Point(rotatedFrameSize.width - rotatedPoint.x, rotatedFrameSize.height - rotatedPoint.y);
            frameSize = rotatedFrameSize;
            break;
        default:
            newPoint = Point(rotatedPoint.y, rotatedFrameSize.height - rotatedPoint.x);
            frameSize = Size(rotatedFrameSize.height, rotatedFrameSize.width);
            break;
        }

        return newPoint;
    }

    /// <summary>
    /// Transform frame point to rotated frame point
    /// </summary>
    /// <param name="rotatedFrameSize">rotatedFrameSize</param>
    /// <param name="point">Frame point</param>
    /// <param name="orientation"></param>
    /// <returns></returns>
    Point TransformFramePointToRotatedFramePoint(Size rotatedFrameSize, Point point, Orientation orientation)
    {
        Point newPoint;
        switch (orientation)
        {
        case Orientation::Portrait:
            newPoint = Point(rotatedFrameSize.width - point.y, point.x);
            break;
        case Orientation::Landscape:
            newPoint = point;
            break;
        case Orientation::ReversePortrait:
            newPoint = Point(point.y, rotatedFrameSize.width - point.x);
            break;
        case Orientation::ReverseLandscape:
            newPoint = Point(rotatedFrameSize.width - point.x, rotatedFrameSize.height - point.y);
            break;
        default:
            newPoint = Point(rotatedFrameSize.width - point.y, point.x);
            break;
        }

        return newPoint;
    }

    /// <summary>
    /// Click on rotatedFrame
    /// </summary>
    /// <param name="rotatedFrameSize">Rotated frame size</param>
    /// <param name="rotatedPoint">Point on rotated frame</param>
    /// <param name="tapMode">For tap mode</param>
    void ClickOnRotatedFrame(const Size &rotatedFrameSize, const Point &rotatedPoint, bool tapMode)
    {
        auto device = DeviceManager::Instance().GetCurrentTargetDevice();
        if(device != nullptr)
        {
            Orientation orientation = device ->GetCurrentOrientation();
            Size frameSize;
            Point newPoint = TransformRotatedFramePointToFramePoint(rotatedFrameSize, rotatedPoint, orientation, frameSize);
            DaVinciStatus status = ClickOnFrame(device, frameSize, newPoint.x, newPoint.y, orientation, tapMode);

            if (!DaVinciSuccess(status))
                DAVINCI_LOG_ERROR << "ClickOnRotatedFrame failed - wrong status";
            else
                DAVINCI_LOG_DEBUG << "ClickOnRotatedFrame success";
        }
        else
        {
            DAVINCI_LOG_ERROR << "ClickOnRotatedFrame failed - device not connected";
        }
    }

    wstring StrToWstr(const char* str)
    {
        int wcsLen = ::MultiByteToWideChar(CP_UTF8, NULL, str, (int)strlen(str), NULL, 0);
        wchar_t* wszString = new wchar_t[wcsLen + 1];
        ::MultiByteToWideChar(CP_UTF8, NULL, str, (int)strlen(str), wszString, wcsLen);
        wszString[wcsLen] = '\0';
        wstring ret = wstring(wszString);
        delete [] wszString;
        return ret;
    }

    string WstrToStr(wstring wstr)
    {
        string str = "";
        int wstrLen = (int)wstr.length();
        int strLen = wstrLen * 2;
        str.resize(strLen,' ');

        ::WideCharToMultiByte(CP_ACP,0,(LPCWSTR)wstr.c_str(),wstrLen,(LPSTR)str.c_str(),strLen,NULL,NULL);

        return str;
    }

    std::wstring AnsiToWChar(LPCSTR pszSrc, int len)
    {
        int nSize = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pszSrc, len, 0, 0);
        if (nSize <= 0)
        {
            return std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes("");
        }

        WCHAR *pwszDst = new WCHAR[nSize + 1];

        MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pszSrc, len, pwszDst, nSize);
        pwszDst[nSize] = 0;

        if (pwszDst[0] == 0xFEFF) // Skip Oxfeff
        {
            for (int i = 0; i < nSize; i++)
            {
                pwszDst[i] = pwszDst[i + 1];
            }
        }

        wstring wcharString(pwszDst);
        delete [] pwszDst;

        return wcharString;
    }

    std::string WCharToAnsi(LPCWSTR pwszSrc)
    {
        int nLen = WideCharToMultiByte(CP_ACP, 0, pwszSrc, -1, NULL, 0, NULL, NULL);
        if (nLen<= 0) return std::string("");
        char* pszDst = new char[nLen];
        if (NULL == pszDst) return std::string("");
        WideCharToMultiByte(CP_ACP, 0, pwszSrc, -1, pszDst, nLen, NULL, NULL);
        pszDst[nLen -1] = 0;
        std::string strTemp(pszDst);
        delete [] pszDst;
        return strTemp;
    }

    std::wstring StrToWstrAnsi(const std::string &str)
    {
        return AnsiToWChar(str.c_str(), (int)str.size());
    }

    string WstrToStrAnsi(wstring& inputws)
    { 
        return WCharToAnsi(inputws.c_str()); 
    }

    bool AreEqual(double a, double b, double absoluteError, double relativeError)
    {
        bool areEqual = false;

        if (a == b)
        {
            areEqual = true;
        }
        else if (fabs(a - b) <= absoluteError)
        {
            areEqual = true;
        }
        else
        {
            double max = fabs(a) > fabs(b) ? fabs(a) : fabs(b);
            if (fabs(a - b) / max <= relativeError)
            {
                areEqual = true;
            }
        }

        return areEqual;
    }

    bool ContainNoneASCIIStr(wstring str)
    {
        bool flag = false;
        int len = (int)str.size();
        for (int i = 0; i < len; i++)
        {
            if ((unsigned int)str[i]>0x007Fu)
            {
                flag = true;
                return flag;
            }
        }
        return flag;
    }

    bool ContainSpecialChar(wstring str)
    {
        bool flag = false;
        int len = (int)str.size();
        for (int i = 0; i < len; i++)
        {
            if ( (unsigned int)str[i] == (unsigned int)('\'') || (unsigned int)str[i] == (unsigned int)('&') )
            {
                flag = true;
                return flag;
            }
        }
        return flag;
    }

    void ParseURI(vector<string>& sections, const string& uri)
    {
        /// TODO escaping uri automatically
        string scheme, user, pwd, host, path, query, fragment;
        string remainder = uri;
        string::size_type idx = remainder.find("://");

        if (idx != string::npos)
        {
            scheme = remainder.substr(0, idx);
            remainder = uri.substr(idx+3);
        }
        else
        {
            scheme = "file";
        }           
        idx = remainder.find("#");
        if (idx != string::npos)
        {
            fragment = remainder.substr(idx+1);
            remainder = remainder.substr(0, idx);        
        }
        else
        {
            fragment = "";    
        }            

        idx = remainder.find("?");
        if (idx != string::npos)
        {
            query = remainder.substr(idx+1);
            remainder = remainder.substr(0, idx);
        }
        else
        {
            query = "";
        }
        idx = remainder.find("@");
        if (idx != string::npos)
        {
            string left =  remainder.substr(0, idx);
            remainder = remainder.substr(idx+1);
            idx = left.find(":");
            if (idx != string::npos)
            {
                user = left.substr(0, idx);
                pwd = left.substr(idx+1);
            }
            else
            {
                user = left;
                pwd = "";
            }
        }
        else
        {
            user = "";
            pwd = "";
        }

        idx =  remainder.find("/");
        if (idx != string::npos)
        {
            host = remainder.substr(0, idx);
            path = remainder.substr(idx+1);
        }
        else
        {
            host = "";
            path = "";
        }

        if (sections.size() < 7)
        {
            sections.resize(7);
        }
        sections[0] = scheme; 
        sections[1] = user; 
        sections[2] = pwd; 
        sections[3] = host; 
        sections[4] = path; 
        sections[5] = query; 
        sections[6] = fragment; 
        //std::cout << sections[0] << "|" << sections[1] << "|" << sections[2] << "|" << sections[3] << "|" << sections[4] << "|" << sections[5] << "|" << sections[6] << endl;
    }

    void ParseQuery(std::unordered_map<string, string>& map, const string& query)
    {
        string remainder = query;
        string::size_type idx;
        do
        { 
            string left = remainder;
            idx = remainder.find("&");
            if (idx != string::npos)
            {
                left = remainder.substr(0, idx);
                remainder = remainder.substr(idx+1);
            }
            string::size_type idxI = left.find("=");
            if (idxI != string::npos)
            {
                string name = left.substr(0, idxI);
                string value =  left.substr(idxI+1);
                if (!name.empty())
                    map[name] = value; 
            }
            else
            {
                if (!left.empty())
                    map[left] = "";
            }
        } while (idx != string::npos);
    }

    string GetCurrentTimeString()
    {
        using namespace std::chrono;
        auto now = system_clock::now();
        auto unixNow = now.time_since_epoch();
        auto usNow = duration_cast<milliseconds>(unixNow);

        auto sNow = duration_cast<seconds>(unixNow);
        auto now2 = system_clock::time_point(sNow);
        auto unixNow2 = now2.time_since_epoch();
        auto usNow2 = duration_cast<milliseconds>(unixNow2);

        auto msTail = usNow - usNow2;
        auto tmNow = system_clock::to_time_t(now2);
        struct tm tm;
        localtime_s(&tm, &tmNow);
        stringstream ss;
        ss << put_time(&tm, "%Y%m%dT%H%M%S") << "." << msTail.count();
        ss.flush();

        return ss.str();
    }

    string ConcatPath(string path1, string path2)
    {
        path p(path1);
        p /= path2;
        return p.string();
    }

    string EscapePath(const string& srcPath)
    {
        auto path = boost::filesystem::path(srcPath);
        boost::filesystem::path newPath;
        for (auto it = path.begin(); it != path.end(); ++ it)
        {
            string tmpStr(it->string());
            if (tmpStr.find(" ") != string::npos)
            {                        
                tmpStr.insert(0, "\"");
                tmpStr.append("\"");
            }
            newPath /= tmpStr;
        }
        return newPath.string();
    }

    string ProcessPathSpaces(const string &processPathStr)
    {
        if (processPathStr.find(" ") != string::npos)
        {
            string processedPath = processPathStr;
            processedPath.insert(0, "\"");
            processedPath.append("\"");
            return processedPath;
        }
        return processPathStr;
    }

    int CalPxLengthOfSide(int longerSidePxLength, int deviceWidth, int deviceHeight)
    {
        int otherSidePxLength = -1;
        if (deviceWidth <= 0 || deviceHeight <= 0)
        {
            return otherSidePxLength;
        }

        double resolutionRatio = (double)deviceWidth / (double)deviceHeight;
        if (resolutionRatio > 1.0)
        {
            otherSidePxLength = (int)(longerSidePxLength / resolutionRatio);
        }
        else
        {
            otherSidePxLength = (int)(longerSidePxLength * resolutionRatio);
        }
        return otherSidePxLength;
    }
}
