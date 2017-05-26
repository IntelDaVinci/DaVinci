#ifndef __OBJECTIVECOMPARATOR__
#define __OBJECTIVECOMPARATOR__

#include "UiRectangle.h"
#include "UiStateObject.h"
#include "UiState.h"
#include <vector>
#include <memory>

/*
 * Intel confidential -- do not distribute further
 * This source code is distributed covered by "Internal License Agreement -- For Internal Open Source DaVinci Program"
 * Please read and accept license.txt distributed with this package before using this source code
 */

using namespace Emgu::CV;
using namespace Emgu::CV::Structure;

namespace DaVinci
{
    namespace Exploratory
    {
        /// <summary>
        /// Class ObjectiveComparator
        /// </summary>
        class ObjectiveComparator : public std::enable_shared_from_this<ObjectiveComparator>
        {
        private:
            int allowDistance = 15;

            /// <summary>
            /// Construct function
            /// </summary>
        public:
            ObjectiveComparator();

            /// <summary>
            /// Set allow distance
            /// </summary>
            /// <param name="d"></param>
            void setAllowDistance(int d);

            /// <summary>
            /// Check whether object recognized object rect center appears in the ui recognized rect center with allowed range
            /// </summary>
            /// <param name="rect"></param>
            /// <param name="objArray"></param>
            /// <returns></returns>
            bool isRectExisting(const std::shared_ptr<UiRectangle> &rect, std::vector<UiStateObject> &objArray);

            /// <summary>
            /// Compare the objects with ui recognized and object recognized 
            /// </summary>
            /// <param name="objState"></param>
            /// <param name="uiState"></param>
            /// <param name="total"></param>
            /// <param name="miss"></param>
            void compareObjects(const std::shared_ptr<UiState> &objState, const std::shared_ptr<UiState> &uiState, int &total, int &miss);

        };
    }
}


#endif	//#ifndef __OBJECTIVECOMPARATOR__
