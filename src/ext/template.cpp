/*
 * Copyright (c) 2018 Martin Jäger / Libre Solar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "config.h"

/** EXT interface description template
 *
 * The UEXT port can be used to connect several different extension modules to the charge controller.
 *
 * Use this file as a template for custom interfaces. If the UEXT port is not used, UEXT_DUMMY_ENABLED
 * has to be defined in config.h to implement the functions.
 */

// Only compile if this EXT interface is enabled in config.h
#ifdef CONFIG_EXT_TEMPLATE_ENABLED

// implement specific extension inherited from ExtInterface
class ExtTemplate: public ExtInterface
{
public:
    ExtTemplate();
    void enable();
    void process_asap();
    void process_1s();
};

static ExtTemplate ext_template;    // local instance, will self register itself

/**
 * Constructor, place basic initialization here, if necessary
 */
ExtTemplate::ExtTemplate() {}

/**
 * Enable operation, place start of use code here, if necessary
 */
void ExtTemplate::enable() {
    // add you init functions here
}

void ExtTemplate::process_asap(void) {
    // add functions to be called as soon as possible during each loop in main function
}

void ExtTemplate::process_1s(void) {
    // add functions to be called every second here
}

#endif /* EXT_TEMPLATE_ENABLED */
