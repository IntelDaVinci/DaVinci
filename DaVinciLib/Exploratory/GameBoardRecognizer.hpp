#ifndef __GAMEBOARDRECOGNIZER__
#define __GAMEBOARDRECOGNIZER__

#define _USE_MATH_DEFINES
#include "BaseObjectCategory.h"
#include "ObjectUtil.h"
#include <string>
#include <vector>
#include <cmath>
#include <memory>

/*
 * Intel confidential -- do not distribute further
 * This source code is distributed covered by "Internal License Agreement -- For Internal Open Source DaVinci Program"
 * Please read and accept license.txt distributed with this package before using this source code
 */

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
        /// <summary>
        /// Game board recognizer
        /// </summary>
        class GameBoardRecognizer : public BaseObjectCategory
        {
        private:
            std::string objectCategory = "Game Board Page";

            std::shared_ptr<ObjectUtil> objUtil;

            Rectangle gameBoardRect;

            std::vector<int> cells;

            /// <summary>
            /// Game board page
            /// </summary>
        public:
            GameBoardRecognizer();

            /// <summary>
            /// Find max aggregate key
            /// </summary>
            /// <param name="table">table</param>
            /// <returns>maxKey</returns>
        private:
            int findMostAggregateKeyFromLineTable(const std::shared_ptr<Hashtable> &table);

            /// <summary>
            /// draw cells in the frame for debug
            /// </summary>
            /// <param name="frame">frame</param>
            /// <param name="boardRectangle">boardRectangle</param>
            /// <param name="column">column</param>
            /// <param name="row">row</param>
            /// <param name="cell">cell[0] for width; cell[1] for height</param>
            void DrawCells(const cv::Mat &frame, Rectangle boardRectangle, int row, int column, std::vector<int> &cell);

            /// <summary>
            /// Recognize board game board
            /// </summary>
            /// <param name="frame">frame</param>
            /// <returns>true for success; false for fail</returns>
        public:
            bool RecognizeGameBoard(const cv::Mat &frame);


            /// <summary>
            /// Check wheteher the object attributes satisfies the category
            /// </summary>
            /// <param name="attributes"></param>
            /// <returns></returns>
            virtual bool BelongsTo(const std::shared_ptr<ObjectAttributes> &attributes) override;

            /// <summary>
            /// Get object category name
            /// </summary>
            /// <returns></returns>
            virtual std::string GetCategoryName() override;

            /// <summary>
            /// Get game board rectangle
            /// </summary>
            /// <returns></returns>
            Rectangle GetGameBoardRect();

public:
            std::shared_ptr<GameBoardRecognizer> shared_from_this()
            {
                return std::static_pointer_cast<GameBoardRecognizer>(BaseObjectCategory::shared_from_this());
            }
        };
    }
}


#endif	//#ifndef __GAMEBOARDRECOGNIZER__
