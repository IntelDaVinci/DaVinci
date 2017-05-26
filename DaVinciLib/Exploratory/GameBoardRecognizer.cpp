#include "GameBoardRecognizer.h"

using namespace Emgu::CV;
using namespace Emgu::CV::Structure;
using namespace Emgu::CV::Util;
using namespace Emgu::Util;
using namespace Emgu::CV::ML;
using namespace Emgu::CV::ML::Structure;
using namespace Emgu::CV::UI;
using namespace Emgu::CV::Features2D;
using namespace Emgu::CV::CvEnum;
using namespace Emgu::CV::ML::MlEnum;
using namespace Emgu::CV::OCR;
namespace DaVinci
{
    namespace Exploratory
    {

        GameBoardRecognizer::GameBoardRecognizer()
        {
            objUtil = std::make_shared<ObjectUtil>();
            gameBoardRect = Rectangle::Empty;
            cells = std::vector<int>(2);
        }

        int GameBoardRecognizer::findMostAggregateKeyFromLineTable(const std::shared_ptr<Hashtable> &table)
        {
            int maxKey = -1;
            int maxCount = -1;

            for (auto key : table)
            {
                std::vector<LineSegment2D> lineList = dynamic_cast<std::vector<LineSegment2D>>(table[key.first]);
                if (lineList.size() > maxCount)
                {
                    maxKey = key.first;
                    maxCount = lineList.size();
                }
            }

            return maxKey;
        }

        void GameBoardRecognizer::DrawCells(const cv::Mat &frame, Rectangle boardRectangle, int row, int column, std::vector<int> &cell)
        {
            for (int i = 1; i < column; i++)
            {
                CvInvoke::cvLine(frame, Point(boardRectangle.X + i * cell[0], boardRectangle.Y), Point(boardRectangle.X + i * cell[0], boardRectangle.Y + boardRectangle.Height), MCvScalar(0, 0, 0), 3, LINE_TYPE::CV_AA, 0);
            }
            for (int i = 1; i < row; i++)
            {
                CvInvoke::cvLine(frame, Point(boardRectangle.X, boardRectangle.Y + i * cell[1]), Point(boardRectangle.X + boardRectangle.Width, boardRectangle.Y + i * cell[1]), MCvScalar(0, 0, 0), 3, LINE_TYPE::CV_AA, 0);
            }
        }

        bool GameBoardRecognizer::RecognizeGameBoard(const cv::Mat &frame)
        {
            int row = 0;
            int column = 0;

            int rectCenterX, rectCenterY;
            int frameCenterX = static_cast<int>(frame->Width / 2), frameCenterY = static_cast<int>(frame->Height / 2);
            int deltaInterval = 20;

            std::vector<Rectangle> rectList = std::vector<Rectangle>();
            std::vector<Rectangle> smallCellList = std::vector<Rectangle>();

//C# TO C++ CONVERTER NOTE: The following 'using' block is replaced by its C++ equivalent:
//            using (Image<Gray, byte> grayFrame = frame.Convert<Gray, byte>())
            cv::Mat grayFrame = frame->Convert<Gray, unsigned char>();
            try
            {
//C# TO C++ CONVERTER NOTE: The following 'using' block is replaced by its C++ equivalent:
//            using (Image<Gray, byte> cannyFrame = new Image<Gray, byte>(grayFrame.Size))
            cv::Mat cannyFrame = std::make_shared<Image<Gray, unsigned char>>(grayFrame->Size);
            try
            {
                CvInvoke::cvCanny(grayFrame, cannyFrame, 10, 50, 3);
                cannyFrame->Save("cannyFrame.png");

//C# TO C++ CONVERTER NOTE: The following 'using' block is replaced by its C++ equivalent:
//                using (MemStorage stor = new MemStorage())
                std::shared_ptr<MemStorage> stor = std::make_shared<MemStorage>();
                try
                {
                    std::shared_ptr<Contour<Point>> contours = cannyFrame->FindContours(CHAIN_APPROX_METHOD::CV_CHAIN_APPROX_NONE, RETR_TYPE::CV_RETR_TREE, stor);
                    for (; contours != nullptr; contours = contours->HNext)
                    {
                        Rectangle rect = contours->BoundingRectangle;
                        if (rect.Width * rect.Height >= frame->Width * frame->Height / 120) // for the existing games, 120 is good, but may not be suitalbe for other unknown games
                        {
                            rectCenterX = (rect.X + rect.Width / 2);
                            rectCenterY = (rect.Y + rect.Height / 2);

                            if (abs(rectCenterX - frameCenterX) < deltaInterval && abs(rectCenterY - frameCenterY) < deltaInterval) // get the whole big rectangle
                            {
                                frame->Draw(rect, Bgr(Color::Blue), 2); // blue marks the big rectangles
                                rectList.push_back(rect);
                            }
                            else // get the small cell rectangle
                            {
                                frame->Draw(rect, Bgr(Color::Red), 2); // red marks all recognized rectangles
                                smallCellList.push_back(rect);
                            }
                        }
                    }
                }
//C# TO C++ CONVERTER TODO TASK: There is no native C++ equivalent to the exception 'finally' clause:
                finally
                {
                    if (stor != nullptr)
                    {
                        stor.Dispose();
                    }
                }
            }
//C# TO C++ CONVERTER TODO TASK: There is no native C++ equivalent to the exception 'finally' clause:
            finally
            {
                if (cannyFrame != nullptr)
                {
                    cannyFrame.Dispose();
                }
            }
            }
//C# TO C++ CONVERTER TODO TASK: There is no native C++ equivalent to the exception 'finally' clause:
            finally
            {
                if (grayFrame != nullptr)
                {
                    grayFrame.Dispose();
                }
            }
            frame->Save("wholeFrame.png");

            // case 1: big rectangle is recognized
            // Sort rectangles according to rect area from small to big
            rectList.Sort([&] (Rectangle rect1, Rectangle rect2)
            {
                return (rect1->Width * rect1->Height).compare(rect2->Width * rect2->Height);
            });

            if (rectList.size() > 0)
            {
                gameBoardRect = rectList[0];

//C# TO C++ CONVERTER NOTE: The following 'using' block is replaced by its C++ equivalent:
//                using (Image<Bgr, byte> boardFrame = frame.Copy(gameBoardRect))
                cv::Mat boardFrame = frame->Copy(gameBoardRect);
                try
                {
//C# TO C++ CONVERTER NOTE: The following 'using' block is replaced by its C++ equivalent:
//                using (Image<Gray, byte> grayFrame = boardFrame.Convert<Gray, byte>())
                cv::Mat grayFrame = boardFrame->Convert<Gray, unsigned char>();
                try
                {
//C# TO C++ CONVERTER NOTE: The following 'using' block is replaced by its C++ equivalent:
//                using (Image<Gray, byte> edgeFrame = new Image<Gray, byte>(grayFrame.Size))
                cv::Mat edgeFrame = std::make_shared<Image<Gray, unsigned char>>(grayFrame->Size);
                try
                {
                    CvInvoke::cvCanny(grayFrame, edgeFrame, 20, 100, 3);
                    edgeFrame->Save("edgeFrame.png");

                    //LineSegment2D[] lines = edgeFrame.HoughLinesBinary(1.0, Math.PI / 180.0, 20, 100, 1.0)[0];
                    std::vector<LineSegment2D> lines = edgeFrame->HoughLinesBinary(1.0, M_PI / 180.0, 20, 65, 1.0)[0];
                    std::vector<LineSegment2D> horizontalLines = std::vector<LineSegment2D>();
                    std::vector<LineSegment2D> verticalLines = std::vector<LineSegment2D>();

                    for (auto line : lines)
                    {
                        if (line.P1.Y == line.P2.Y)
                        {
                            CvInvoke::cvLine(boardFrame, line.P1, line.P2, MCvScalar(255, 255, 0), 1, LINE_TYPE::CV_AA, 0); // greenblue marks horizontal lines
                            horizontalLines.push_back(line);
                        }
                        else if (line.P1.X == line.P2.X)
                        {
                            CvInvoke::cvLine(boardFrame, line.P1, line.P2, MCvScalar(0, 255, 255), 1, LINE_TYPE::CV_AA, 0); // yellow marks vertical lines
                            verticalLines.push_back(line);
                        }
                    }

                    boardFrame->Save("boardFrame.png");

                    // calculate width of a cell
                    std::shared_ptr<Hashtable> table = objUtil->HashLinesWithInterval(horizontalLines, ObjectUtil::HashLength);
                    int aggregateKey = findMostAggregateKeyFromLineTable(table);
                    std::vector<LineSegment2D> aggregateLineList = dynamic_cast<std::vector<LineSegment2D>>(table[aggregateKey]);

                    double sum = 0;
                    int calibration = 0;
                    double preciseCell = 0;

                    for (auto line : aggregateLineList)
                    {
                        sum += line.Length;
                    }

                    preciseCell = sum / aggregateLineList.size();
                    column = static_cast<int>(gameBoardRect.Width / preciseCell);
                    // there may be intervals between the board rectangle and the small cells, in this case calibration is needed
                    calibration = static_cast<int>((gameBoardRect.Width / preciseCell - column) * preciseCell / column);
                    cells[0] = static_cast<int>(preciseCell) + calibration;

                    // calculate weight of a cell
                    table = objUtil->HashLinesWithInterval(verticalLines, ObjectUtil::HashLength);
                    aggregateKey = findMostAggregateKeyFromLineTable(table);
                    aggregateLineList = dynamic_cast<std::vector<LineSegment2D>>(table[aggregateKey]);

                    sum = 0;
                    for (auto line : aggregateLineList)
                    {
                        sum += line.Length;
                    }

                    preciseCell = sum / aggregateLineList.size();
                    row = static_cast<int>(gameBoardRect.Height / preciseCell);
                    calibration = static_cast<int>((gameBoardRect.Height / preciseCell - row) * preciseCell / row);
                    cells[1] = static_cast<int>(preciseCell) + calibration;

                    // draw cells for debug
                    this->DrawCells(frame, gameBoardRect, row, column, cells);
                    frame->Save("wholeFrame2.png");
                }
//C# TO C++ CONVERTER TODO TASK: There is no native C++ equivalent to the exception 'finally' clause:
                finally
                {
                    if (edgeFrame != nullptr)
                    {
                        edgeFrame.Dispose();
                    }
                }
                }
//C# TO C++ CONVERTER TODO TASK: There is no native C++ equivalent to the exception 'finally' clause:
                finally
                {
                    if (grayFrame != nullptr)
                    {
                        grayFrame.Dispose();
                    }
                }
                }
//C# TO C++ CONVERTER TODO TASK: There is no native C++ equivalent to the exception 'finally' clause:
                finally
                {
                    if (boardFrame != nullptr)
                    {
                        boardFrame.Dispose();
                    }
                }

                return true;
            }
            else if (smallCellList.size() > 0) // case 2: small cell rectangles are recognized
            {
                std::shared_ptr<Hashtable> table = nullptr;
                std::vector<int> aggregateKey;
                std::vector<Rectangle> columnRectangleList;
                std::vector<Rectangle> rowRectangleList;

                int rowInterval = 0;
                int columnInterval = 0;
                int sum = 0;

                int rectangleX = 0;
                int rectangleY = 0;
                int rectangleWidth = 0;
                int rectangleHeight = 0;

                // get a column
                table = objUtil->HashRectanglesByXYWidthHeight(smallCellList, 0);
                aggregateKey = objUtil->FindMostAggregateKeyFromRectangleTable(table);
                columnRectangleList = dynamic_cast<std::vector<Rectangle>>(table[aggregateKey]);
                row = columnRectangleList.size();

                // sort rectangles according to rectangle's y from small to big
                columnRectangleList.Sort([&] (Rectangle rect1, Rectangle rect2)
                {
                    return rect1::Y->compare(rect2::Y);
                });

                // get a row
                table = objUtil->HashRectanglesByXYWidthHeight(smallCellList, 1);
                aggregateKey = objUtil->FindMostAggregateKeyFromRectangleTable(table);
                rowRectangleList = dynamic_cast<std::vector<Rectangle>>(table[aggregateKey]);
                column = rowRectangleList.size();

                // sort rectangles according to rectangle's x from small to big
                rowRectangleList.Sort([&] (Rectangle rect1, Rectangle rect2)
                {
                    return rect1::X->compare(rect2::X);
                });

                // calculate width of a cell, and expand it to make no column interval
                for (int i = 0; i < column; i++)
                {
                    if (i + 1 < column)
                    {
                        columnInterval += rowRectangleList[i + 1].X - rowRectangleList[i].X - rowRectangleList[i].Width;
                    }
                    sum += rowRectangleList[i].Width;
                }
                columnInterval /= (column - 1);
                cells[0] = static_cast<int>(sum) / column + columnInterval;

                sum = 0;
                for (int i = 0; i < row; i++)
                {
                    if (i + 1 < row)
                    {
                        rowInterval += columnRectangleList[i + 1].Y - columnRectangleList[i].Y - columnRectangleList[i].Height;
                    }
                    sum += columnRectangleList[i].Height;
                }
                rowInterval /= (row - 1);
                cells[1] = static_cast<int>(sum) / row + rowInterval;

                rectangleX = rowRectangleList[0].X - columnInterval / 2;
                rectangleY = columnRectangleList[0].Y - rowInterval / 2;
                rectangleWidth = rowRectangleList.Last()->X - rowRectangleList.First()->X + rowRectangleList.Last()->Width + columnInterval;
                rectangleHeight = columnRectangleList.Last()->Y - columnRectangleList.First()->Y + columnRectangleList.Last()->Height + rowInterval;

                gameBoardRect = Rectangle(rectangleX, rectangleY, rectangleWidth, rectangleHeight);

                // draw cells for debug
                frame->Draw(gameBoardRect, Bgr(Color::Black), 3);
                this->DrawCells(frame, gameBoardRect, row, column, cells);
                frame->Save("wholeFrame2.png");

                return true;
            }


            return false;
        }

        bool GameBoardRecognizer::BelongsTo(const std::shared_ptr<ObjectAttributes> &attributes)
        {
            cv::Mat image = attributes->image;
            return RecognizeGameBoard(image);
        }

        std::string GameBoardRecognizer::GetCategoryName()
        {
            return objectCategory;
        }

        Rectangle GameBoardRecognizer::GetGameBoardRect()
        {
            return gameBoardRect;
        }
    }
}
