/**
* @file include/llvmir2hll/semantics/semantics/libc_semantics.h
* @brief Semantics for the standard C library.
* @copyright (c) 2017 Avast Software, licensed under the MIT license
*/

#ifndef LLVMIR2HLL_SEMANTICS_SEMANTICS_LIBC_SEMANTICS_H
#define LLVMIR2HLL_SEMANTICS_SEMANTICS_LIBC_SEMANTICS_H

#include <string>

#include "llvmir2hll/semantics/semantics/default_semantics.h"
#include "llvmir2hll/support/smart_ptr.h"

namespace llvmir2hll {

/**
* @brief Semantics for the standard C library.
*
* This class provides the semantics for the standard C library.
*
* Instances of this class have reference object semantics.
*/
class LibcSemantics: public DefaultSemantics {
public:
	static ShPtr<Semantics> create();

	/// @name Semantics Interface
	/// @{
	virtual std::string getId() const override;
	virtual Maybe<std::string> getMainFuncName() const override;
	virtual Maybe<std::string> getCHeaderFileForFunc(
		const std::string &funcName) const override;
	virtual Maybe<bool> funcNeverReturns(
		const std::string &funcName) const override;
	virtual Maybe<std::string> getNameOfVarStoringResult(
		const std::string &funcName) const override;
	virtual Maybe<std::string> getNameOfParam(const std::string &funcName,
		unsigned paramPos) const override;
	virtual Maybe<IntStringMap> getSymbolicNamesForParam(
		const std::string &funcName, unsigned paramPos) const override;
	/// @}

protected:
	LibcSemantics();
};

} // namespace llvmir2hll

#endif
