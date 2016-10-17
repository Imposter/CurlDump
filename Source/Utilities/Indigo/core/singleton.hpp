/*
*   This file is part of the Indigo library.
*
*   This program is licensed under the GNU General
*   Public License. To view the full license, check
*   LICENSE in the project root.
*/

#ifndef indigo_singleton_hpp_
#define indigo_singleton_hpp_

namespace indigo {
template <typename _TClass>
class Singleton {
public:
	static _TClass &GetInstance() {
		static _TClass instance;
		return instance;
	}

	static _TClass *GetInstancePtr() {
		return &GetInstance();
	}

	Singleton() { };
	Singleton(_TClass const &) = delete;
	void operator=(_TClass const &) = delete;
};
}

#endif // indigo_singleton_hpp_