#include <string>
#include <unordered_map>
#include <vector>

#include "boost/algorithm/string/join.hpp"

#include "UiStateObject.hpp"
#include "PriorityWords.hpp"

using namespace std;

namespace DaVinci
{
	PriorityWords::PriorityWords(std::vector<std::string> &patternStrings)
	{
		stringVector = patternStrings;
		for(auto patternString : stringVector)
		{
			std::tr1::regex regexStr(patternString);
			patterns.push_back(regexStr);
		}
	}

	bool PriorityWords::TryGetPriorityObject(std::unordered_map<UiStateObject, bool> &ls, UiStateObject &o)
	{
		for (auto pair : ls)
		{
			std::string word = pair.first.GetText();

			if (word == "" || word == "")
			{
				continue;
			}
			for (int i = 0; i < patterns.size(); ++i)
			{
				// 'i' is the priority index
				if (patterns.at[i]->Match(word).Success)
				{
					o = pair.first;
					patterns.erase(patterns.begin() + i);
					return true;
				}
			}
		}

		return false;
	}

	std::string PriorityWords::ToString()
	{
		return boost::algorithm::join(stringVector, " | ");
	}
}
