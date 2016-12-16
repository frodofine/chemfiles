/* Chemfiles, an efficient IO library for chemistry file formats
* Copyright (C) 2015 Guillaume Fraux
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/
*/
// clang-format off

#include <cstring>

#include "chemfiles.h"
#include "chemfiles/Residue.hpp"
#include "chemfiles/Topology.hpp"
#include "chemfiles/capi.hpp"
using namespace chemfiles;

extern "C" CHFL_RESIDUE* chfl_residue(const char* name, uint64_t resid) {
    assert(name != nullptr);
    CHFL_RESIDUE* residue = nullptr;
    CHFL_ERROR_GOTO(
        residue = new Residue(std::string(name), resid);
    )
    return residue;
error:
    delete residue;
    return nullptr;
}

extern "C" CHFL_RESIDUE* chfl_residue_from_topology(const CHFL_TOPOLOGY* const topology, uint64_t i) {
    assert(topology != nullptr);
    CHFL_RESIDUE* residue = nullptr;
    CHFL_ERROR_GOTO(
        // Return NULL if the index is out of bounds
        if (i < topology->residues().size()) {
            residue = new Residue(topology->residues()[checked_cast(i)]);
        }

    )
    return residue;
error:
    delete residue;
    return nullptr;
}

extern "C" CHFL_RESIDUE* chfl_residue_for_atom(const CHFL_TOPOLOGY* const topology, uint64_t i) {
    assert(topology != nullptr);
    CHFL_RESIDUE* residue = nullptr;
    CHFL_ERROR_GOTO(
        auto res = topology->residue(checked_cast(i));
        if (res) {
            residue = new Residue(*res);
        }
    )
    return residue;
error:
    delete residue;
    return nullptr;
}

extern "C" chfl_status chfl_residue_atoms_count(const CHFL_RESIDUE* const residue, uint64_t* size) {
    assert(residue != nullptr);
    assert(size != nullptr);
    CHFL_ERROR_CATCH(
        *size = residue->size();
    )
}


extern "C" chfl_status chfl_residue_id(const CHFL_RESIDUE* const residue, uint64_t* id) {
    assert(residue != nullptr);
    assert(id != nullptr);
    CHFL_ERROR_CATCH(
        *id = residue->id();
    )
}

extern "C" chfl_status chfl_residue_name(const CHFL_RESIDUE* const residue, char* name, uint64_t buffsize) {
    assert(residue != nullptr);
    assert(name != nullptr);
    CHFL_ERROR_CATCH(
        strncpy(name, residue->name().c_str(), checked_cast(buffsize) - 1);
        name[buffsize - 1] = '\0';
    )
}

extern "C" chfl_status chfl_residue_add_atom(CHFL_RESIDUE* const residue, uint64_t i) {
    assert(residue != nullptr);
    CHFL_ERROR_CATCH(
        residue->add_atom(checked_cast(i));
    )
}

extern "C" chfl_status chfl_residue_contains(const CHFL_RESIDUE* const residue, uint64_t i, bool* result) {
    assert(residue != nullptr);
    assert(result != nullptr);
    CHFL_ERROR_CATCH(
        *result = residue->contains(checked_cast(i));
    )
}

extern "C" chfl_status chfl_residue_free(CHFL_RESIDUE* const residue) {
    delete residue;
    return CHFL_SUCCESS;
}