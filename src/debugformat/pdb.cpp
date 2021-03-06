/**
 * @file src/debugformat/pdb.cpp
 * @brief Common (DWARF and PDB) debug information representation library.
 * @copyright (c) 2017 Avast Software, licensed under the MIT license
 */

#define LOG_ENABLED false

#include <iostream>

#include "tl-cpputils/debug.h"
#include "tl-cpputils/string.h"
#include "debugformat/debugformat.h"

namespace debugformat {

void DebugFormat::loadPdb()
{
	if (!_pdbFile)
		return;

	loadPdbTypes();
	loadPdbGlobalVariables();
	loadPdbFunctions();
}

void DebugFormat::loadPdbTypes()
{
	auto* ts = _pdbFile->get_types_container();
	for (auto& item : ts->types)
	{
		if (item.second)
		{
			types.insert(loadPdbType(item.second));
		}
	}
	for (auto& item : ts->types_fully_defined)
	{
		if (item.second)
		{
			types.insert(loadPdbType(item.second));
		}
	}
}

void DebugFormat::loadPdbGlobalVariables()
{
	auto* pdbGlobalVars = _pdbFile->get_global_variables();
	if (pdbGlobalVars == nullptr)
		return;

	for (auto& gvItem : *pdbGlobalVars)
	{
		auto& pgv = gvItem.second;

		std::string n = pgv.name;
		std::string name = n.empty() ? "glob_var_" + tl_cpputils::toHexString(pgv.address) : n;
		auto addr = tl_cpputils::Address(pgv.address);
		if (!addr.isDefined())
			continue;
		retdec_config::Object gv(name, retdec_config::Storage::inMemory(addr));
		gv.type = loadPdbType(pgv.type_def);
		if (gv.type.getLlvmIr() == "void")
			gv.type.setLlvmIr("i32");
		globals.insert(gv);
	}
}

void DebugFormat::loadPdbFunctions()
{
	auto* pdbFunctions = _pdbFile->get_functions();
	if (pdbFunctions == nullptr)
		return;

	for (auto& f : *pdbFunctions)
	{
		if (f.second == nullptr)
			continue;

		pdbparser::PDBFunction *pfnc = f.second;

		retdec_config::Function fnc(pfnc->getNameWithOverloadIndex());

		fnc.setStart(pfnc->address);
		fnc.setEnd(pfnc->address + pfnc->length);
		fnc.setSourceFileName(_pdbFile->get_module_name(pfnc->module_index));

		auto* sym = _inFile->getFileFormat()->getSymbol(pfnc->address + 1);
		fnc.setIsThumb(sym && sym->isThumbSymbol());

		fnc.setIsVariadic(pfnc->type_def->func_is_variadic);

		if (!pfnc->lines.empty())
		{
			fnc.setStartLine(pfnc->lines.front().line);
			fnc.setEndLine(pfnc->lines.back().line);
		}

		if (pfnc->type_def != nullptr)
		{
			fnc.returnType = loadPdbType(pfnc->type_def->func_rettype_def);
		}

		unsigned argCntr = 0;
		for (auto& a : pfnc->arguments)
		{
			retdec_config::Storage storage;
			if (a.location == pdbparser::PDBLVLOC_REGISTER) // in register
			{
				storage = retdec_config::Storage::inRegister(a.register_num);
			}
			else // in register-relative stack
			{
				auto num = a.location == pdbparser::PDBLVLOC_BPREL32 ? 22 /*CV_REG_EBP*/ : a.register_num;
				storage = retdec_config::Storage::onStack(a.offset, num);
			}

			std::string n = a.name;
			std::string name = n.empty() ? std::string("arg") + std::to_string(argCntr) : n;
			retdec_config::Object newArg(name, storage);
			newArg.type = loadPdbType(a.type_def);
			fnc.parameters.insert(newArg);
			++argCntr;
		}

		for (auto& lv : pfnc->loc_variables)
		{
			std::string name = lv.name;
			if (name.empty())
			{
				continue;
			}

			retdec_config::Storage storage;
			if (lv.location == pdbparser::PDBLVLOC_REGISTER) // in register
			{
				storage = retdec_config::Storage::inRegister(lv.register_num);
			}
			else  // in register-relative stack
			{
				auto num = lv.location == pdbparser::PDBLVLOC_BPREL32 ? 22 /*CV_REG_EBP*/ : lv.register_num;
				storage = retdec_config::Storage::onStack(lv.offset, num);
			}

			retdec_config::Object newLocalVar(name, storage);
			newLocalVar.type = loadPdbType(lv.type_def);
			fnc.locals.insert(newLocalVar);
		}

		fnc.setIsFromDebug(true);
		functions.insert( {fnc.getStart(), fnc} );
	}
}

/**
 * Convert PDB type representation into common type representation.
 * @param type PDB type.
 * @return Common type representation.
 */
retdec_config::Type DebugFormat::loadPdbType(pdbparser::PDBTypeDef* type)
{
	if (type == nullptr)
	{
		return retdec_config::Type("i32");
	}
	auto t = retdec_config::Type(type->to_llvm());
	return t.isDefined() ? t : retdec_config::Type("i32");
}

} // namespace debugformat
