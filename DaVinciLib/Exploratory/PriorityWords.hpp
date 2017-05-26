#ifndef __PRIORITYWORDS__
#define __PRIORITYWORDS__

#include <string>
#include <unordered_map>
#include <vector>
#include <regex>

#include "UiStateObject.hpp"

namespace DaVinci
{
	/// <summary>
	/// Class PriorityWords
	/// </summary>
	class PriorityWords
	{
		/// <summary>
		/// Construct
		/// </summary>
		/// <param name="patternStrings"></param>
	public:
		PriorityWords(std::vector<std::string> &patternStrings);

		/// <summary>
		/// Try get priority object
		/// </summary>
		/// <param name="ls"></param>
		/// <param name="o"></param>
		/// <returns></returns>
		bool TryGetPriorityObject(std::unordered_map<UiStateObject, bool> &ls, UiStateObject &o);

		/// <summary>
		/// ToString
		/// </summary>
		/// <returns></returns>
		std::string ToString();

	private:
		std::vector<std::regex> patterns;
		std::vector<string> stringVector;
	};
}


#endif	//#ifndef __PRIORITYWORDS__
