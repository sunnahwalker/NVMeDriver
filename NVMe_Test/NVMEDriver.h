#pragma once

#ifndef __NVME_DRIVER_H__
#define __NVME_DRIVER_H__

#include <iostream>
#include <stdio.h>
#include <Windows.h>

BOOL find_port();
BOOL Do_IDFY();
BOOL Do_SMART();
BOOL Close_Handle();

#endif