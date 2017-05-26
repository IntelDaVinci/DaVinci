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

#ifndef __SEEDRANDOM__HPP__
#define __SEEDRANDOM__HPP__

#include "boost/random.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"

namespace DaVinci
{
    using namespace std;
    using namespace boost::posix_time;

    class SeedRandom
    {
    public:
        typedef boost::uniform_int<> NumberDistribution; 
        typedef boost::mt19937 RandomNumberGenerator; 
        typedef boost::variate_generator<RandomNumberGenerator&, NumberDistribution> Generator; 

        SeedRandom();
        SeedRandom(int seed);
        int GetRandomWithRange(int start, int end);

    private:
        RandomNumberGenerator generator;

    };

}

#endif	//#ifndef __SEEDRANDOM__HPP__