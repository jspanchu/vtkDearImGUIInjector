// SPDX-FileCopyrightText: Copyright 2023 Jaswant Panchumarti
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

class vtkObject;

class OverlayUI
{
public:
  static void setup(vtkObject* caller, unsigned long eventId, void* clientData, void* callData);
  static void draw(vtkObject* caller, unsigned long eventId, void* clientData, void* callData);
};