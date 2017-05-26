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

#ifndef __UISTATEOBJECT__
#define __UISTATEOBJECT__

#include <vector>

#include "DaVinciDefs.hpp"
#include "DaVinciStatus.hpp"
#include "UiRectangle.hpp"
#include "UiAction.hpp"

#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMDocumentType.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationLS.hpp>
#include <xercesc/dom/DOMNodeIterator.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMText.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>

namespace DaVinci
{
    using namespace std;
    using namespace xercesc;

    /// <summary>
    /// Class UiStateObject
    /// </summary>
    class UiStateObject : public boost::enable_shared_from_this<UiStateObject>
    {
        /// <summary>
        /// default constructor to be compatiable for UiStateMenu UiStateHome UiStateBack
        /// </summary>
    public:
        UiStateObject();

        /// <summary>
        /// Construct
        /// </summary>
        /// <param name="xmlNode"></param>
        UiStateObject(const DOMNode &xmlNode);

        const std::string GetText() const;
        void SetText(const std::string &value);
        const UiRectangle GetRect() const;
        void SetRect(const UiRectangle &value);
        const bool GetClickable() const;
        void SetClickable(const bool clickable);
        const bool GetControlState() const;
        void SetControlState(const bool control);
        void SetIDInfo(const std::string &value);
        const std::string GetIDInfo() const;

        virtual std::string takeAction(const boost::shared_ptr<UiAction> &action);

        /// <summary>
        /// Get Hashcode
        /// </summary>
        /// <returns></returns>
        const int GetHashCode() const;

        bool operator==(const UiStateObject &o) const
        { 
            int sHashCode = this->GetHashCode();
            int tHashCode = o.GetHashCode();
            return (sHashCode == tHashCode);
        }

    private:
        /// <summary>
        /// Text
        /// </summary>
        std::string text;
        UiRectangle rect;
        bool isClickable;
        bool isControl;
        std::string idInfo;
    };
}


#endif	//#ifndef __UISTATEOBJECT__
