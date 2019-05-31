/* LibreSolar Battery Management System firmware
 * Copyright (c) 2016-2019 Martin Jäger (www.libre.solar)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "config.h"

/** UEXT interface description template
 *
 * The UEXT port can be used to connect several different extension modules to the charge controller.
 *
 * Only one interface can be used at a time. The interfaces are selected via #define in config.h
 *
 * Use this file as a template for custom interfaces. If the UEXT port is not used, UEXT_DUMMY_ENABLED
 * has to be defined in config.h to implement the functions.
 */

// Only compile if this UEXT interface is enabled in config.h
#ifdef UEXT_DUMMY_ENABLED

void uext_init(void)
{
    // add you init functions here
}

void uext_process_asap(void)
{
    // add functions to be called as soon as possible during each loop in main function
}

void uext_process_1s(void)
{
    // add functions to be called every second here
}

#endif /* UEXT_DUMMY_ENABLED */
