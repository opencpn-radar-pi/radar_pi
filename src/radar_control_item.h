/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Radar Plugin
 * Author:   David Register
 *           Dave Cowell
 *           Kees Verruijt
 *           Douwe Fokkema
 *           Sean D'Epagnier
 ***************************************************************************
 *   Copyright (C) 2010 by David S. Register              bdbcat@yahoo.com *
 *   Copyright (C) 2012-2013 by Dave Cowell                                *
 *   Copyright (C) 2012-2016 by Kees Verruijt         canboat@verruijt.net *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************
 */

#ifndef _RADAR_CONTROL_ITEM_H_
#define _RADAR_CONTROL_ITEM_H_

#include "pi_common.h"

PLUGIN_BEGIN_NAMESPACE

class radar_pi;

struct RadarRange {
  int meters;
  int actual_meters;
};

class radar_control_item {
 public:
  // The copy constructor
  radar_control_item(const radar_control_item &other) {
    m_value = other.m_value;
    m_button = other.m_button;
    m_mod = other.m_mod;
  }

  // The assignment constructor
  radar_control_item &operator=(const radar_control_item &other) {
    if (this != &other) {  // self-assignment check expected
      m_value = other.m_value;
      m_button = other.m_button;
      m_mod = other.m_mod;
    }
    return *this;
  }

  // The assignment constructor to allow "item = value"
  radar_control_item &operator=(int v) {
    Update(v);
    return *this;
  }

  void Update(int v) {
    wxCriticalSectionLocker lock(m_exclusive);

    if (v != m_button) {
      m_mod = true;
      m_button = v;
    }
    m_value = v;
  };

  bool GetButton(int *value) {
    wxCriticalSectionLocker lock(m_exclusive);
    bool changed = m_mod;
    if (value) {
      *value = this->m_value;
    }

    m_mod = false;
    return changed;
  }

  int GetButton() {
    wxCriticalSectionLocker lock(m_exclusive);

    m_mod = false;
    return m_button;
  }

  int GetValue() {
    wxCriticalSectionLocker lock(m_exclusive);

    return m_value;
  }

  bool IsModified() {
    wxCriticalSectionLocker lock(m_exclusive);

    return m_mod;
  }

  radar_control_item() {
    m_value = 0;
    m_button = 0;
    m_mod = false;
  }

 protected:
  wxCriticalSection m_exclusive;
  int m_value;
  int m_button;
  bool m_mod;
};

class radar_range_control_item : public radar_control_item {
 public:
  // PersistentSettings *m_settings;

  void Update(int v);
  const RadarRange *GetRange() {
    wxCriticalSectionLocker lock(m_exclusive);

    return m_range;
  }

  radar_range_control_item(radar_pi *pi) {
    m_pi = pi;
    m_value = 0;
    m_button = 0;
    m_mod = false;
    m_range = 0;
    // m_settings = 0;
  }

 private:
  const RadarRange *m_range;
  radar_pi *m_pi;
};

PLUGIN_END_NAMESPACE

#endif
