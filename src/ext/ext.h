/*
 * Copyright (c) 2019 Martin Jäger / Libre Solar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EXT_H_
#define EXT_H_

#include <vector>

/** @file
 *
 * @brief Classes to handle EXTensions and EXTernal communication interfaces
 */

class ExtInterface
{
public:
    ExtInterface();
    virtual void process_asap() {};
    virtual void process_1s() {};
    virtual void enable() {};
};

class ExtManager
{
public:
    ExtManager();

    /** UEXT interface process function
     *
     * This function is called in each main loop, as soon as all other tasks finished.
     */
    virtual void process_asap();

    /** UEXT interface process function
     *
     * This function is called every second, if no other task was blocking for a longer time.
     * It should be used for state machines, etc.
     */
    virtual void process_1s();

    /* UEXT interface initialization
    *
    * This function is called only once at startup.
    */
    virtual void enable_all();

    /**
     * Adds an ExtInterface object to the list of managed extensions,
     * automatically called by ExtInterface objects during construction
     */
    static void add_ext(ExtInterface*);

private:
    /**
     * Stores list of all ExtInterface objects
     */
    static std::vector<ExtInterface*>* interfaces;

    /**
     * Makes sure there is an internal interfacts vector created, does nothing if it exists
     */
    static void check_list();
};

extern ExtManager ext_mgr;

#endif /* EXT_H_ */
