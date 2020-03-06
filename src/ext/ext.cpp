/*
 * Copyright (c) 2019 Martin Jäger / Libre Solar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ext.h"

#include <algorithm>

// necessary for PlatformIO debug session
void std::__throw_bad_alloc()
{
    while (1) {}
}

// necessary to compile Zephyr v2.2
void std::__throw_length_error(char const*)
{
    while (1) {}
}

// we use code to self register objects at construction time.
// It relies on the fact that initially this pointer will be initialized
// with NULL ! This is only true for global variables or static members of classes
// so this must remain a static class member
std::vector<ExtInterface*>* ExtManager::interfaces;

// run the respective function on all objects in the "devices" list
// use the c++11 lambda expressions here for the for_each loop, keeps things compact

void ExtManager::process_asap()
{
    for_each(std::begin(*interfaces),std::end(*interfaces), [](ExtInterface* tsif) {
        tsif->process_asap();
    });
}

void ExtManager::enable_all()
{
    for_each(std::begin(*interfaces),std::end(*interfaces), [](ExtInterface* tsif) {
        tsif->enable();
    });
}

void ExtManager::process_1s()
{
    for_each(std::begin(*interfaces),std::end(*interfaces), [](ExtInterface* tsif) {
        tsif->process_1s();
    });
}

void ExtManager::check_list()
{
    if (ExtManager::interfaces == NULL) {
        ExtManager::interfaces = new std::vector<ExtInterface*>;
    }
}

void ExtManager::add_ext(ExtInterface* member)
{
    check_list();
    interfaces->push_back(member);
}

ExtManager::ExtManager()
{
    check_list();
}

ExtInterface::ExtInterface()
{
    ext_mgr.add_ext(this);
}

ExtManager ext_mgr;
