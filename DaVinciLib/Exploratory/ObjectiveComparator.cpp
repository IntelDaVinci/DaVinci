#include "ObjectiveComparator.h"

using namespace Emgu::CV;
using namespace Emgu::CV::Structure;
namespace DaVinci
{
    namespace Exploratory
    {

        ObjectiveComparator::ObjectiveComparator()
        {
        }

        void ObjectiveComparator::setAllowDistance(int d)
        {
            this->allowDistance = d;
        }

        bool ObjectiveComparator::isRectExisting(const std::shared_ptr<UiRectangle> &rect, std::vector<UiStateObject> &objArray)
        {
            Rectangle geoRect = Rectangle(rect->GetLeftUpper().X, rect->GetLeftUpper().Y, rect->GetRightBottom().GetX() - rect->GetLeftUpper().X, rect->GetRightBottom().GetY() - rect->GetLeftUpper().Y);

            for (auto obj : objArray)
            {
                Point geoPoint = Point(obj->GetRect().Center()->X, obj->GetRect().Center()->Y);
                if (geoRect.Contains(geoPoint))
                {
                    return true;
                }
            }

            return false;
        }

        void ObjectiveComparator::compareObjects(const std::shared_ptr<UiState> &objState, const std::shared_ptr<UiState> &uiState, int &total, int &miss)
        {
            if (objState == nullptr || uiState == nullptr)
            {
                total = 0;
                miss = 0;
                return;
            }

            std::vector<UiStateObject> objList = objState->GetClickableObjects();

            std::vector<UiStateObject> uiList = uiState->GetClickableObjects();

            if (uiList.empty())
            {
                total = 0;
                miss = 0;
                return;
            }

            if (objList.empty())
            {
                total = uiList.size();
                miss = uiList.size();
                return;
            }

            total = uiList.size();

            std::vector<bool> mark = std::vector<bool>(uiList.size());

            for (int i = 0; i < mark.size(); i++)
            {
                mark[i] = false;
            }

            for (int i = 0; i < uiList.size(); i++)
            {
                std::shared_ptr<UiRectangle> rect = uiList[i]->GetRect();
                if (isRectExisting(rect, objList))
                {
                    mark[i] = true;
                }
            }

            miss = 0;
            for (int i = 0; i < mark.size(); i++)
            {
                if (mark[i] == false)
                {
                    miss++;
                }
            }
        }
    }
}
